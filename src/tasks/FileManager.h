#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "../Task.h"
#include "../Display.h"
#include "../Keyboard.h"
#include "../ui/UI.h"
#include "../ui/elements/All.h"
#include "../MemoryFree.h"
#include "../Debug.h"

namespace nsShell
{
    bool requestOpenFile(const char* path, TaskHandle_t callerHandle);
}

namespace nsFileManager
{
    enum FileCommand : uint8_t
    {
        fileCommandOpen,
        fileCommandCut,
        fileCommandCopy,
        fileCommandPaste,
        fileCommandNewDir,
        fileCommandRename,
        fileCommandDelete,
        fileCommandInfo
    };

    static const char menuOpen[] PROGMEM = "Open";
    static const char menuCut[] PROGMEM = "Cut";
    static const char menuCopy[] PROGMEM = "Copy";
    static const char menuPaste[] PROGMEM = "Paste";
    static const char menuNewDir[] PROGMEM = "New dir";
    static const char menuRename[] PROGMEM = "Rename";
    static const char menuDelete[] PROGMEM = "Delete";
    static const char menuInfo[] PROGMEM = "Info";

    static PGM_P const fileMenuItems[] PROGMEM = {
        menuOpen, menuCut, menuCopy, menuPaste,
        menuNewDir, menuRename, menuDelete, menuInfo};

    class FileManager : public Task
    {
    private:
        inline static const char caption[] PROGMEM = "FileManager";
        inline static const uint8_t icon[] PROGMEM = {
            0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0, 0x7C, 0x00, 0xF0, 0xFE, 0x00, 0xF0,
            0xFE, 0x01, 0xF0, 0xFE, 0xFF, 0xF0, 0xFE, 0xFF, 0xF1, 0xFE, 0xFF, 0xF1,
            0x1E, 0x00, 0xF0, 0x0E, 0x00, 0xF0, 0xE6, 0xFF, 0xF3, 0xF6, 0xFF, 0xF7,
            0xF2, 0xFF, 0xF7, 0xFA, 0xFF, 0xF3, 0xF8, 0xFF, 0xF3, 0xFC, 0xFF, 0xF1,
            0xFE, 0xFF, 0xF1, 0xFE, 0xFF, 0xF0, 0x00, 0x00, 0xF0, 0x00, 0x00, 0xF0};
        Form *form = nullptr;
        FileList *files = nullptr;
        char *clipboardPath = nullptr;
        char *operationPath = nullptr;
        bool clipboardValid = false;
        bool clipboardCut = false;
        bool refreshPending = false;
        bool preserveRefreshSelection = false;
        Menu *contextMenu = nullptr;

        bool appendPath(char *path, uint8_t pathSize, const char *part)
        {
            uint8_t length = strlen(path);
            uint8_t partLength = strlen(part);
            if (partLength == 0)
                return length < pathSize;
            if ((uint16_t)length + partLength + 2 > pathSize)
                return false;
            if (length > 0 && path[length - 1] != '/')
                path[length++] = '/';
            memcpy(path + length, part, partLength + 1);
            return true;
        }

        bool removeLastPathComponent(char *path)
        {
            uint8_t length = strlen(path);
            while (length > 1 && path[length - 1] == '/')
                path[--length] = '\0';
            while (length > 1 && path[length - 1] != '/')
                path[--length] = '\0';
            return true;
        }

        // Копирование дерева файлов/папок с обходом в FileManager
        bool copyTreeLocal(const char *sourcePath, const char *destinationPath)
        {
            // Проверяем что источник существует
            FsFile root = sdCard.open(sourcePath, O_RDONLY);
            if (!root)
            {
                return false;
            }
            
            // Если это файл, просто копируем
            if (root.isFile())
            {
                root.close();
                return sdCard.copySingleFile(sourcePath, destinationPath);
            }
            root.close();
            
            // Это директория - создаём её в destination
            if (!sdCard.mkdir(destinationPath))
            {
                return false;
            }
            
            // Выделяем буферы для путей
            char *currentSrc = static_cast<char *>(pvPortMalloc(MAX_PATH_LEN));
            char *currentDst = static_cast<char *>(pvPortMalloc(MAX_PATH_LEN));
            
            if (!currentSrc || !currentDst)
            {
                vPortFree(currentSrc);
                vPortFree(currentDst);
                return false;
            }
            
            strncpy(currentSrc, sourcePath, MAX_PATH_LEN - 1);
            currentSrc[MAX_PATH_LEN - 1] = '\0';
            strncpy(currentDst, destinationPath, MAX_PATH_LEN - 1);
            currentDst[MAX_PATH_LEN - 1] = '\0';
            
            if (!root.open(currentSrc, O_RDONLY))
            {
                vPortFree(currentSrc);
                vPortFree(currentDst);
                return false;
            }
            
            // Стек для навигации (позиции в директориях)
            const uint8_t maxDepth = 8;
            uint32_t positions[maxDepth];
            uint8_t depth = 0;
            bool success = true;
            
            while (success)
            {
                FsFile child = root.openNextFile(O_RDONLY);
                
                if (!child)
                {
                    // Конец текущей директории
                    root.close();
                    if (depth == 0)
                    {
                        break; // Всё обошли
                    }
                    
                    // Возвращаемся на уровень вверх
                    --depth;
                    if (!removeLastPathComponent(currentSrc) ||
                        !removeLastPathComponent(currentDst) ||
                        !root.open(currentSrc, O_RDONLY))
                    {
                        success = false;
                        break;
                    }
                    root.seek(positions[depth]);
                    continue;
                }
                
                // Получаем имя файла/папки
                char name[28];
                child.getName(name, sizeof(name));
                
                if (!appendPath(currentSrc, MAX_PATH_LEN, name) ||
                    !appendPath(currentDst, MAX_PATH_LEN, name))
                {
                    child.close();
                    success = false;
                    break;
                }
                
                bool descended = false;
                
                if (child.isFile())
                {
                    // Копируем файл через задачу
                    child.close();
                    success = sdCard.copySingleFile(currentSrc, currentDst);
                    
                    // Даём idle task время освободить память завершённой задачи
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                else if (depth >= maxDepth)
                {
                    // Слишком глубокая вложенность
                    child.close();
                    success = false;
                    break;
                }
                else
                {
                    // Это директория - создаём и входим в неё
                    if (!sdCard.mkdir(currentDst))
                    {
                        child.close();
                        success = false;
                    }
                    else
                    {
                        positions[depth++] = root.curPosition();
                        child.close();
                        root.close();
                        success = root.open(currentSrc, O_RDONLY);
                        descended = success;
                    }
                }
                
                if (!descended)
                {
                    // Убираем добавленное имя из путей
                    if (!removeLastPathComponent(currentSrc) ||
                        !removeLastPathComponent(currentDst))
                    {
                        success = false;
                    }
                }
            }
            
            root.close();
            vPortFree(currentSrc);
            vPortFree(currentDst);
            
            return success;
        }

        bool isPasteDestinationValid(const char *destination, const char *clipPath) const
        {
            uint8_t sourceLength = strlen(clipPath);
            if (strcmp(clipPath, destination) == 0)
                return false;
            return !(strncmp(clipPath, destination, sourceLength) == 0 &&
                     destination[sourceLength] == '/');
        }

        bool preparePasteDestination()
        {
            operationPath = static_cast<char *>(pvPortMalloc(MAX_PATH_LEN));
            if (!operationPath)
                return false;

            const char *slash = strrchr(clipboardPath, '/');
            strncpy(operationPath, files->getCurrentPath(), MAX_PATH_LEN - 1);
            operationPath[MAX_PATH_LEN - 1] = '\0';
            if (appendPath(operationPath, MAX_PATH_LEN, slash ? slash + 1 : clipboardPath) &&
                isPasteDestinationValid(operationPath, clipboardPath))
                return true;

            vPortFree(operationPath);
            operationPath = nullptr;
            return false;
        }

        void refreshList(bool preserveSelection = false)
        {
            if (!files)
                return;
            uint16_t savedIndex = preserveSelection ? files->getSelectedIndex() : 0;
            files->openDir(files->getCurrentPath());
            if (preserveSelection)
            {
                uint16_t itemCount = files->getItemCount();
                if (itemCount == 0)
                    savedIndex = 0;
                else if (savedIndex >= itemCount)
                    savedIndex = itemCount - 1;
                files->setSelectedIndex(savedIndex);
                files->draw();
            }
        }

        void requestListRefresh(bool preserveSelection = false)
        {
            refreshPending = true;
            preserveRefreshSelection = preserveRefreshSelection || preserveSelection;
        }

        void showContextMenu()
        {
            if (!form || contextMenu)
                return;
            contextMenu = form->addMenu(80, 6, 46, 38);
            if (!contextMenu)
                return;
            uint8_t count = 8;
            contextMenu->setProgmemItems(fileMenuItems, count);
            contextMenu->setEventMask(EVENT_CLICK | EVENT_CLOSE);
            contextMenu->show();
        }

        void closeContextMenu()
        {
            if (contextMenu)
            {
                form->removeElement(contextMenu);
                contextMenu = nullptr;
            }
        }

        void createDirectory()
        {
            char name[LIST_TEXT_BUF_SIZE] = {};
            char path[MAX_PATH_LEN];
            if (ui.inputBox(F("Folder name:"), name, sizeof(name)) != dialogOk || !name[0])
                return;

            strncpy(path, files->getCurrentPath(), sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
            if (!appendPath(path, sizeof(path), name))
                ui.messageBox(F("Path is too long"));
            else if (!sdCard.mkdir(path))
                ui.messageBox(F("Cannot create folder"));
            else
            {
                refreshList();
                files->selectByName(name);
            }
        }

        void copyToClipboard(bool cut)
        {
            char path[MAX_PATH_LEN];
            if (!files->getSelectedPath(path, sizeof(path)))
                return;

            char *newClipboardPath = static_cast<char *>(pvPortMalloc(MAX_PATH_LEN));
            if (newClipboardPath && strlen(path) < MAX_PATH_LEN)
            {
                strcpy(newClipboardPath, path);
                vPortFree(clipboardPath);
                clipboardPath = newClipboardPath;
                clipboardValid = true;
                clipboardCut = cut;
            }
            else
            {
                if (newClipboardPath)
                    vPortFree(newClipboardPath);
                if (clipboardPath)
                    vPortFree(clipboardPath);
                clipboardPath = nullptr;
                clipboardValid = false;
            }
        }

        void pasteFromClipboard()
        {
            if (!clipboardValid || !clipboardPath ||
                ui.messageBox(F("Paste here?"), messageBoxYesNo) != dialogYes)
                return;

            if (!preparePasteDestination())
            {
                ui.messageBox(F("Invalid paste destination"));
                return; // operationPath уже освобождён в preparePasteDestination()
            }

            // operationPath теперь валиден, нужно освободить в любом случае
            
            // СТАРЫЙ КОД (копирование с рекурсией в отдельной задаче):
            // bool copySuccess = sdCard.copyTree(clipboardPath, operationPath);
            
            // ТЕСТ 1 (копирование одного файла):
            // bool copySuccess = sdCard.copySingleFile(clipboardPath, operationPath);
            
            // ТЕСТ 2 (обход в FileManager, копирование файлов через задачи):
            DEBUG_PRINT_F("FileManager before copyTreeLocal, stack free = ");
            DEBUG_PRINTLN(getStackFreeBytes(xTaskGetCurrentTaskHandle()));
            
            bool copySuccess = copyTreeLocal(clipboardPath, operationPath);
            
            DEBUG_PRINT_F("FileManager after copyTreeLocal, stack free = ");
            DEBUG_PRINTLN(getStackFreeBytes(xTaskGetCurrentTaskHandle()));
            
            vPortFree(operationPath);
            operationPath = nullptr;

            if (!copySuccess)
            {
                ui.messageBox(F("Cannot paste"));
                return;
            }

            if (clipboardCut && !sdCard.removeTree(clipboardPath))
            {
                ui.messageBox(F("Cannot remove source item"));
                requestListRefresh(true);
                return;
            }

            clipboardValid = false;
            vPortFree(clipboardPath);
            clipboardPath = nullptr;
            requestListRefresh(true);
        }

        bool getSelectedItemPath(char *path)
        {
            char name[LIST_TEXT_BUF_SIZE];
            return files->getSelectedName(name, sizeof(name)) &&
                   strcmp(name, "..") != 0 &&
                    files->getSelectedPath(path, MAX_PATH_LEN);
        }

        void deleteSelectedItem()
        {
            if (ui.messageBox(F("Delete selected item?"), messageBoxYesNo) != dialogYes)
                return;

            operationPath = static_cast<char *>(pvPortMalloc(MAX_PATH_LEN));
            if (!operationPath || !getSelectedItemPath(operationPath))
            {
                if (operationPath)
                    vPortFree(operationPath);
                operationPath = nullptr;
                return;
            }
            if (!sdCard.removeTree(operationPath))
                ui.messageBox(F("Cannot delete item"));
            else
                requestListRefresh(true);
            vPortFree(operationPath);
            operationPath = nullptr;
        }

        void renameSelectedItem()
        {
            char name[LIST_TEXT_BUF_SIZE];
            char sourcePath[MAX_PATH_LEN];
            char destinationPath[MAX_PATH_LEN];
            if (!files->getSelectedName(name, sizeof(name)) || strcmp(name, "..") == 0 ||
                !files->getSelectedPath(sourcePath, sizeof(sourcePath)) ||
                ui.inputBox(F("New name:"), name, sizeof(name)) != dialogOk || !name[0])
                return;

            strncpy(destinationPath, files->getCurrentPath(), sizeof(destinationPath) - 1);
            destinationPath[sizeof(destinationPath) - 1] = '\0';
            if (!appendPath(destinationPath, sizeof(destinationPath), name) ||
                !sdCard.rename(sourcePath, destinationPath))
                ui.messageBox(F("Cannot rename item"));
            else
                requestListRefresh(true);
        }

        void showSelectedItemInfo()
        {
            char path[MAX_PATH_LEN];
            if (!getSelectedItemPath(path))
                return;

            FsFile info = sdCard.open(path, O_RDONLY);
            if (!info)
                ui.messageBox(F("Cannot open item"));
            else if (info.isDir())
            {
                info.close();
                ui.messageBox(F("Directory"));
            }
            else
            {
                char text[LIST_TEXT_BUF_SIZE];
                snprintf(text, sizeof(text), "Size: %lu", (unsigned long)info.size());
                info.close();
                ui.messageBox(text);
            }
        }

        void executeCommand(uint8_t command)
        {
            switch (command)
            {
            case fileCommandOpen:
                files->openSelected();
                break;
            case fileCommandNewDir:
                createDirectory();
                break;
            case fileCommandCut:
                copyToClipboard(true);
                break;
            case fileCommandCopy:
                copyToClipboard(false);
                break;
            case fileCommandPaste:
                pasteFromClipboard();
                break;
            case fileCommandDelete:
                deleteSelectedItem();
                break;
            case fileCommandRename:
                renameSelectedItem();
                break;
            case fileCommandInfo:
                showSelectedItemInfo();
                break;
            }
        }

        void onEvent(UIElement *sender, uint8_t event, uint8_t value) override
        {
            if (sender == contextMenu && (event == EVENT_CLICK || event == EVENT_CLOSE))
            {
                uint8_t command = (event == EVENT_CLICK) ? contextMenu->getResultIndex() : 255;
                closeContextMenu();
                if (command != 255)
                    executeCommand(command);
                return;
            }

            FileList *list = static_cast<FileList *>(sender);
            if (event == EVENT_CLICK)
            {
                char selectedPath[MAX_PATH_LEN];
                if (list->getSelectedPath(selectedPath, sizeof(selectedPath)) &&
                    !nsShell::requestOpenFile(selectedPath, xTaskGetCurrentTaskHandle()))
                    ui.messageBox(F("Cannot open file"));
            }
            else if (event == EVENT_KEY && value == keyEsc)
            {
                requestClose();
            }
            else if (event == EVENT_KEY && value == keySpace)
            {
                showContextMenu();
            }
        }

    public:
        static const __FlashStringHelper *getCaption() { return reinterpret_cast<const __FlashStringHelper*>(caption); }
        static const uint8_t *getIcon() { return icon; }
        const __FlashStringHelper *getTaskCaption() const override { return getCaption(); }

        uint16_t getStackSize() const override { return 840; }

        ~FileManager() override
        {
            if (form)
            {
                ui.closeForm(form);
                form = nullptr;
            }
            files = nullptr;
            if (clipboardPath)
                vPortFree(clipboardPath);
            if (operationPath)
                vPortFree(operationPath);
        }

    protected:
        void setup() override
        {
            form = new Form();
            form->setCaption(reinterpret_cast<const __FlashStringHelper*>(caption));
            form->setDrawBorder(true);

            files = form->addFileList(1, 1, 124, 50);
            files->setDrawBorder(true);
            files->setEventMask(EVENT_CLICK | EVENT_KEY);
            files->openDir("/");

            ui.showForm(form);
        }

        void loop() override
        {
            if (refreshPending)
            {
                bool preserveSelection = preserveRefreshSelection;
                refreshPending = false;
                preserveRefreshSelection = false;
                refreshList(preserveSelection);
            }
            vTaskDelay(pdMS_TO_TICKS(25));
        }
    };
}

#endif // FILEMANAGER_H
