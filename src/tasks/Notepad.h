#ifndef NOTEPAD_H
#define NOTEPAD_H

#include "../Task.h"
#include "../Display.h"
#include "../Keyboard.h"
#include "../Sdcard.h"
#include "../ui/UI.h"
#include "../ui/elements/All.h"
#include <string.h>

namespace nsNotepad
{
    static const char menuNew[] PROGMEM = "New";
    static const char menuOpen[] PROGMEM = "Open";
    static const char menuSave[] PROGMEM = "Save";
    static const char menuSaveAs[] PROGMEM = "Save As";
    static const char menuExit[] PROGMEM = "Exit";

    static PGM_P const menuItems[] PROGMEM = {
        menuNew,
        menuOpen,
        menuSave,
        menuSaveAs,
        menuExit};

    class Notepad : public Task
    {
    private:
        inline static const char caption[] PROGMEM = "Notepad";
        inline static const uint8_t icon[] PROGMEM = {
            0x00, 0x00, 0xF0, 0xFE, 0xFF, 0xF7, 0x02, 0x80, 0xF4, 0x02, 0x80, 0xF4,
            0xF2, 0x9F, 0xF7, 0x02, 0x80, 0xF7, 0xF2, 0x83, 0xF7, 0x02, 0x80, 0xF4,
            0xF2, 0x9F, 0xF4, 0x02, 0x80, 0xF4, 0xF2, 0x87, 0xF4, 0x02, 0x80, 0xF4,
            0xF2, 0x8F, 0xF4, 0x02, 0x80, 0xF4, 0xF2, 0x83, 0xF4, 0x02, 0x80, 0xF4,
            0xF2, 0x8F, 0xF4, 0x02, 0x80, 0xF4, 0xFE, 0xFF, 0xF7, 0x00, 0x00, 0xF0};
        
        Form *form = nullptr;
        Memo *memo = nullptr;
        Menu *menu = nullptr;
        uint8_t pendingCommand = 0xFF;
        char currentPath[MAX_PATH_LEN] = {};

        void ensureExtension(char *path, const char *ext)
        {
            uint8_t len = strlen(path);
            if (len > 0 && len < 60)
            {
                const char *dot = strrchr(path, '.');
                const char *slash = strrchr(path, '/');
                if (!dot || (slash && dot < slash))
                {
                    strcat(path, ext);
                }
            }
        }

        void onEvent(UIElement *sender, uint8_t event, uint8_t value) override
        {
            (void)value;
            
            if (sender == menu && (event == EVENT_CLICK || event == EVENT_CLOSE))
            {
                int8_t selected = -1;
                if (event == EVENT_CLICK)
                    selected = menu->getResultIndex();
                
                if (form && menu)
                {
                    form->removeElement(menu);
                    menu = nullptr;
                }
                // OnEvent - метод с уже глубокой вложенностью
                // Отложим обработку выбранной команды для loop, где вложенность минимальна
                if (selected != -1)
                    pendingCommand = selected;
            }
        }

        bool checkSave()
        {
            if (memo && memo->isModified())
            {
                DialogResult result = ui.messageBox(F("Save changes?"), messageBoxYesNo);
                if (result == dialogYes)
                {
                    if (!currentPath[0])
                        return saveAs();
                    if (memo->save(currentPath))
                        return true;
                    ui.messageBox(F("Cannot save file"));
                    return false;
                }
                else if (result == dialogNo)
                    return true;
                return false;
            }
            return true;
        }

        bool saveAs()
        {
            char *path = (char *)pvPortMalloc(64);
            if (!path)
            {
                ui.messageBox(F("Out of memory"));
                return false;
            }

            path[0] = '\0';
            bool saved = false;
            if (ui.saveDialog(path, 64, TextRef(".txt")) == dialogOk)
            {
                ensureExtension(path, ".txt");
                saved = memo && memo->save(path);
                if (saved)
                    strcpy(currentPath, path);
                else
                    ui.messageBox(F("Cannot save file"));
            }
            vPortFree(path);
            return saved;
        }

    public:
        static const __FlashStringHelper *getCaption() { return reinterpret_cast<const __FlashStringHelper*>(caption); }
        static const uint8_t *getIcon() { return icon; }
        const __FlashStringHelper *getTaskCaption() const override { return getCaption(); }

        uint16_t getStackSize() const override { return 600; }

        ~Notepad() override
        {
            if (form)
            {
                form->close();
                form = nullptr;
            }
            memo = nullptr;
            menu = nullptr;
        }

        static bool canOpen(const char *extension)
        {
            if (!extension || strlen(extension) != 4 || extension[0] != '.')
                return false;

            for (uint8_t i = 1; i < 4; ++i)
            {
                char ch = extension[i];
                if (ch >= 'A' && ch <= 'Z')
                    ch += 'a' - 'A';
                if (ch != "txt"[i - 1])
                    return false;
            }
            return true;
        }

        bool openFile(const char *path) override
        {
            if (!path || !path[0] || strlen(path) >= sizeof(currentPath))
                return false;

            if (memo)
            {
                if (memo->isModified())
                    return false;
                if (!memo->openFile(path))
                    return false;
            }
            else
            {
                if (!sdCard.exists(path))
                    return false;
            }

            strcpy(currentPath, path);
            return true;
        }

    protected:
        void setup() override
        {
            form = new Form();
            if (!form)
            {
                requestClose();
                return;
            }
            form->setCaption(reinterpret_cast<const __FlashStringHelper*>(caption));
            form->setDrawBorder(true);

            memo = form->addMemo(0, 1, 126, 53);
            if (!memo)
            {
                requestClose();
                return;
            }
            memo->setEditMode(true);
            memo->clear();

            ui.showForm(form);
            if (currentPath[0] && !memo->openFile(currentPath))
            {
                currentPath[0] = '\0';
                memo->clear();
                ui.messageBox(F("Cannot open file"));
            }
        }

        void loop() override
        {
            if (pendingCommand != 0xFF)
            {
                // Отложенные команды меню
                uint8_t cmd = pendingCommand;
                pendingCommand = 0xFF;
                
                if (cmd == 0) // New
                {
                    if (checkSave())
                    {
                        memo->clear();
                        currentPath[0] = '\0';
                    }
                }
                else if (cmd == 1) // Open
                {
                    if (checkSave())
                    {
                        char *path = (char *)pvPortMalloc(MAX_PATH_LEN);
                        if (path)
                        {
                            path[0] = '\0';
                            Serial.println(F("open dialog"));
                            Serial.flush();
                            if (ui.openDialog(path, MAX_PATH_LEN, TextRef(".txt")) == dialogOk)
                            {
                                if (!openFile(path))
                                    ui.messageBox(F("Cannot open file"));
                            }
                            vPortFree(path);
                        }
                    }
                }
                else if (cmd == 2) // Save
                {
                    if (!currentPath[0])
                    {
                        saveAs();
                    }
                    else if (!memo || !memo->save(currentPath))
                        ui.messageBox(F("Cannot save file"));
                }
                else if (cmd == 3) // Save As
                {
                    saveAs();
                }
                else if (cmd == 4) // Exit
                {
                    requestClose();
                }
            }

            if (ui.getActiveForm() == form && !menu && keyboard.take(keyEsc))
            {
                keyboard.consumeKBState();
                requestClose();
            }
            else if (ui.getActiveForm() == form && !menu && keyboard.take(keySpace, true))
            {
                keyboard.consumeKBState();
                menu = form->addMenu(0, 0, 50, 47);
                menu->setProgmemItems(menuItems, 5);
                menu->setEventMask(EVENT_CLICK | EVENT_CLOSE);
                menu->show();
            }

            taskYIELD();
        }

        CloseDecision onCloseRequest() override
        {
            return checkSave() ? closeAccept : closeReject;
        }
    };
}

#endif // NOTEPAD_H
