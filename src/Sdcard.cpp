#include "Sdcard.h"
#include "Debug.h"
#include "MemoryFree.h"
#include "Shell.h"
#include "Common.h"
#include <string.h>

SDCard sdCard;

bool SDCard::begin(uint8_t chipSelectPin, uint32_t maxSck)
{
    return filesystem.begin(chipSelectPin, maxSck);
}

FsFile SDCard::open(const char *path, oflag_t flags)
{
    return filesystem.open(path, flags);
}

bool SDCard::exists(const char *path)
{
    return filesystem.exists(path);
}

bool SDCard::mkdir(const char *path, bool createParents)
{
    return filesystem.mkdir(path, createParents);
}

bool SDCard::remove(const char *path)
{
    return filesystem.remove(path);
}

bool SDCard::rmdir(const char *path)
{
    return filesystem.rmdir(path);
}

bool SDCard::rename(const char *oldPath, const char *newPath)
{
    return filesystem.rename(oldPath, newPath);
}

uint8_t SDCard::sdErrorCode() const
{
    return const_cast<SdFs &>(filesystem).sdErrorCode();
}

uint32_t SDCard::sdErrorData() const
{
    return const_cast<SdFs &>(filesystem).sdErrorData();
}

bool SDCard::appendPath(char *path, const char *part) const
{
    if (!path || !part)
        return false;

    uint8_t length = strlen(path);
    uint8_t partLength = strlen(part);
    if ((uint16_t)length + partLength + 2 > MAX_PATH_LEN)
        return false;
    if (length && path[length - 1] != '/')
        path[length++] = '/';
    memcpy(path + length, part, partLength + 1);
    return true;
}

bool SDCard::removeLastPathComponent(char *path) const
{
    if (!path)
        return false;

    uint8_t length = strlen(path);
    while (length > 1 && path[length - 1] == '/')
        path[--length] = '\0';
    while (length > 1 && path[length - 1] != '/')
        path[--length] = '\0';
    return true;
}

bool SDCard::copyFile(const char *sourcePath, const char *destinationPath)
{
    FsFile src;
    FsFile dst;
    if (!src.open(sourcePath, O_RDONLY) ||
        !dst.open(destinationPath, O_WRONLY | O_CREAT | O_TRUNC))
    {
        src.close();
        dst.close();
        return false;
    }

    uint8_t buffer[64];
    int bytesRead;
    bool success = true;
    while ((bytesRead = src.read(buffer, sizeof(buffer))) > 0)
    {
        if (dst.write(buffer, bytesRead) != (size_t)bytesRead)
        {
            success = false;
            break;
        }
    }
    src.close();
    dst.close();
    return success && bytesRead >= 0;
}

bool SDCard::copyTreeInternal()
{
    FsFile root;
    if (!root.open(operationSourcePath, O_RDONLY))
        return false;
    if (root.isFile())
    {
        root.close();
        return copyFile(operationSourcePath, operationDestinationPath);
    }
    root.close();

    if (!filesystem.mkdir(operationDestinationPath))
        return false;

    char *sourcePath = static_cast<char *>(pvPortMalloc(MAX_PATH_LEN));
    char *destinationPath = static_cast<char *>(pvPortMalloc(MAX_PATH_LEN));

    if (!sourcePath || !destinationPath)
    {
        vPortFree(sourcePath);
        vPortFree(destinationPath);
        return false;
    }

    uint8_t srcLen = strlen(operationSourcePath);
    if (srcLen >= MAX_PATH_LEN)
    {
        vPortFree(sourcePath);
        vPortFree(destinationPath);
        return false;
    }
    memcpy(sourcePath, operationSourcePath, srcLen + 1);

    uint8_t destLen = strlen(operationDestinationPath);
    if (destLen >= MAX_PATH_LEN)
    {
        vPortFree(sourcePath);
        vPortFree(destinationPath);
        return false;
    }
    memcpy(destinationPath, operationDestinationPath, destLen + 1);

    if (!root.open(sourcePath, O_RDONLY))
    {
        vPortFree(sourcePath);
        vPortFree(destinationPath);
        return false;
    }

    uint32_t positions[navigationDepth];
    uint8_t depth = 0;
    bool success = true;
    while (success)
    {
        FsFile child = root.openNextFile(O_RDONLY);
        if (!child)
        {
            root.close();
            if (depth == 0)
                break;
            --depth;
            if (!removeLastPathComponent(sourcePath) ||
                !removeLastPathComponent(destinationPath) ||
                !root.open(sourcePath, O_RDONLY))
            {
                success = false;
                break;
            }
            root.seek(positions[depth]);
            continue;
        }

        char name[fileNameCapacity];
        child.getName(name, sizeof(name));
        if (!appendPath(sourcePath, name) || !appendPath(destinationPath, name))
        {
            child.close();
            success = false;
            break;
        }

        bool descended = false;
        if (child.isFile())
        {
            success = copyFile(sourcePath, destinationPath);
        }
        else if (depth >= navigationDepth)
            success = false;
        else
        {
            if (!filesystem.mkdir(destinationPath))
                success = false;
            else
            {
                positions[depth++] = root.curPosition();
                child.close();
                root.close();
                success = root.open(sourcePath, O_RDONLY);
                descended = success;
            }
        }
        child.close();
        if (!descended &&
            (!removeLastPathComponent(sourcePath) || !removeLastPathComponent(destinationPath)))
            success = false;
    }
    root.close();
    vPortFree(sourcePath);
    vPortFree(destinationPath);
    return success;
}

bool SDCard::removeTreeInternal()
{
    FsFile item;
    if (!item.open(operationSourcePath, O_RDONLY))
        return false;
    if (item.isFile())
    {
        item.close();
        return filesystem.remove(operationSourcePath);
    }
    item.close();

    char *currentPath = static_cast<char *>(pvPortMalloc(MAX_PATH_LEN));
    if (!currentPath)
    {
        return false;
    }

    // Inline storePath
    uint8_t pathLen = strlen(operationSourcePath);
    if (pathLen >= MAX_PATH_LEN)
    {
        vPortFree(currentPath);
        return false;
    }
    memcpy(currentPath, operationSourcePath, pathLen + 1);

    if (!item.open(currentPath, O_RDONLY))
    {
        vPortFree(currentPath);
        return false;
    }

    uint32_t positions[navigationDepth];
    uint8_t depth = 0;
    while (true)
    {
        FsFile child = item.openNextFile(O_RDONLY);
        if (!child)
        {
            item.close();
            if (depth == 0)
                break;
            --depth;
            if (!removeLastPathComponent(currentPath) || !item.open(currentPath, O_RDONLY))
            {
                vPortFree(currentPath);
                return false;
            }
            item.seek(positions[depth]);
            continue;
        }

        char name[fileNameCapacity];
        child.getName(name, sizeof(name));
        if (!appendPath(currentPath, name))
        {
            child.close();
            item.close();
            vPortFree(currentPath);
            return false;
        }

        if (child.isFile())
        {
            child.close();
            if (!filesystem.remove(currentPath) || !removeLastPathComponent(currentPath))
            {
                item.close();
                vPortFree(currentPath);
                return false;
            }
        }
        else if (depth >= navigationDepth)
        {
            child.close();
            item.close();
            vPortFree(currentPath);
            return false;
        }
        else
        {
            FsFile check = child.openNextFile(O_RDONLY);
            if (!check)
            {
                child.close();
                if (!filesystem.rmdir(currentPath) || !removeLastPathComponent(currentPath))
                {
                    item.close();
                    vPortFree(currentPath);
                    return false;
                }
            }
            else
            {
                check.close();
                child.close();
                positions[depth++] = item.curPosition();
                item.close();
                if (!item.open(currentPath, O_RDONLY))
                {
                    vPortFree(currentPath);
                    return false;
                }
            }
        }
    }
    bool success = filesystem.rmdir(operationSourcePath);
    vPortFree(currentPath);
    return success;
}

void SDCard::operationTask(void *parameter)
{
    SDCard *card = static_cast<SDCard *>(parameter);

    if (card->operation == operationCopy)
        card->operationResult = card->copyTreeInternal();
    else if (card->operation == operationRemove)
        card->operationResult = card->removeTreeInternal();
    else if (card->operation == operationCopySingleFile)
        card->operationResult = card->copySingleFileInternal();

    card->operationStackFreeBytes = getStackFreeBytes(nullptr);
    card->operationSourcePath = nullptr;
    card->operationDestinationPath = nullptr;

    xTaskNotify(card->operationCaller, 0, eNoAction);
    vTaskDelete(nullptr);
}

bool SDCard::copyTree(const char *sourcePath, const char *destinationPath)
{
    if (operationBusy || !sourcePath || !destinationPath)
        return false;

    operation = operationCopy;
    operationResult = false;
    operationStackFreeBytes = 0;
    operationSourcePath = sourcePath;
    operationDestinationPath = destinationPath;
    operationCaller = xTaskGetCurrentTaskHandle();
    operationBusy = true;

    if (xTaskCreate(operationTask, "SDOp", treeStackSize, this, 4, nullptr) != pdPASS)
    {
        operationBusy = false;
        operationCaller = nullptr;
        operationSourcePath = nullptr;
        operationDestinationPath = nullptr;
        return false;
    }

    xTaskNotifyWait(0, UINT32_MAX, nullptr, portMAX_DELAY);
    operationCaller = nullptr;
    operationBusy = false;

    return operationResult;
}

bool SDCard::removeTree(const char *path)
{
    if (operationBusy || !path)
        return false;

    operation = operationRemove;
    operationResult = false;
    operationStackFreeBytes = 0;
    operationSourcePath = path;
    operationDestinationPath = nullptr;
    operationCaller = xTaskGetCurrentTaskHandle();
    operationBusy = true;

    if (xTaskCreate(operationTask, "SDOp", treeStackSize, this, 4, nullptr) != pdPASS)
    {
        operationBusy = false;
        operationCaller = nullptr;
        operationSourcePath = nullptr;
        operationDestinationPath = nullptr;
        return false;
    }

    xTaskNotifyWait(0, UINT32_MAX, nullptr, portMAX_DELAY);
    operationCaller = nullptr;
    operationBusy = false;

    return operationResult;
}

bool SDCard::copyFileAsync(const char *sourcePath, const char *destinationPath, TaskHandle_t caller)
{
    if (operationBusy || !sourcePath || !destinationPath || !caller)
        return false;

    operation = operationCopy;
    operationResult = false;
    operationStackFreeBytes = 0;
    operationSourcePath = sourcePath;
    operationDestinationPath = destinationPath;
    operationCaller = caller;
    operationBusy = true;

    if (xTaskCreate(operationTask, "SDOp", treeStackSize, this, 4, nullptr) != pdPASS)
    {
        operationBusy = false;
        operationCaller = nullptr;
        operationSourcePath = nullptr;
        operationDestinationPath = nullptr;
        return false;
    }

    // Не ждём завершения — возвращаемся сразу
    return true;
}

bool SDCard::checkAsyncOperationComplete(bool *result)
{
    if (!operationCaller || !operationBusy)
        return false;

    // Проверяем, что вызов происходит из той же задачи, которая запустила операцию
    if (xTaskGetCurrentTaskHandle() != operationCaller)
        return false;

    // Проверяем уведомление без блокировки (timeout = 0)
    if (xTaskNotifyWait(0, UINT32_MAX, nullptr, 0) == pdTRUE)
    {
        // Операция завершилась
        if (result)
            *result = operationResult;

        operationCaller = nullptr;
        operationBusy = false;

        return true;
    }

    // Операция ещё выполняется
    return false;
}

// Простое копирование одного файла - inline всё в одной функции для минимального стека
bool SDCard::copySingleFileInternal()
{
    DEBUG_PRINT("Copy: opening src ");
    DEBUG_PRINTLN(operationSourcePath);

    // Открываем source и destination напрямую, без вызовов вспомогательных функций
    FsFile src;
    if (!src.open(operationSourcePath, O_RDONLY))
    {
        DEBUG_PRINTLN("Copy: src open FAILED");
        return false;
    }

    uint32_t fileSize = src.fileSize();
    DEBUG_PRINT("Copy: src size=");
    DEBUG_PRINTLN(fileSize);
    DEBUG_PRINT("Copy: opening dst ");
    DEBUG_PRINTLN(operationDestinationPath);

    FsFile dst;
    if (!dst.open(operationDestinationPath, O_WRONLY | O_CREAT | O_TRUNC))
    {
        DEBUG_PRINTLN("Copy: dst open FAILED");
        src.close();
        return false;
    }

    DEBUG_PRINTLN("Copy: files opened, starting copy loop");

    // Локальный буфер для fallback
    uint8_t localBuffer[64];
    int bytesRead;
    bool success = true;
    uint32_t totalCopied = 0;

    // Внешние объявления
    extern volatile uint8_t *frameBuffer;
    extern volatile uint8_t bytesToSend;
    extern TaskHandle_t activeTask; // Из Common.h - текущая активная задача

    bool useVideoBuffer = (operationCaller == activeTask);

    if (useVideoBuffer)
    {
        DEBUG_PRINTLN("Copy: FileManager active, using video buffer (512)");
    }
    else
    {
        DEBUG_PRINTLN("Copy: FileManager inactive, using local buffer (64)");
    }

    while (true)
    {
        uint8_t *buffer;
        size_t bufferSize;

        if (useVideoBuffer)
        {
            // FileManager в фокусе - используем видеобуфер (512 байт)
            SchedulerSuspender lock; // Блокируем планировщик

            // Ждём завершения передачи на дисплей (если идёт)
            while (TCCR5B != 0)
            {
            }

            buffer = (uint8_t *)frameBuffer;
            bufferSize = 512;

            bytesRead = src.read(buffer, bufferSize);

            if (bytesRead <= 0)
                break;

            int written = dst.write(buffer, bytesRead);

            if (written != bytesRead)
            {
                DEBUG_PRINTLN("Copy: write MISMATCH!");
                success = false;
                break;
            }

            totalCopied += bytesRead;
        }
        else
        {
            // Другая задача активна - используем маленький локальный буфер
            buffer = localBuffer;
            bufferSize = sizeof(localBuffer);

            bytesRead = src.read(buffer, bufferSize);

            if (bytesRead <= 0)
                break;

            int written = dst.write(buffer, bytesRead);

            if (written != bytesRead)
            {
                DEBUG_PRINTLN("Copy: write MISMATCH!");
                success = false;
                break;
            }

            totalCopied += bytesRead;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    DEBUG_PRINT("Copy: loop done, total copied=");
    DEBUG_PRINT(totalCopied);
    DEBUG_PRINT(" success=");
    DEBUG_PRINTLN(success);

    src.close();
    dst.close();

    DEBUG_PRINTLN("Copy: files closed");

    return success && bytesRead >= 0;
}

bool SDCard::copySingleFile(const char *sourcePath, const char *destinationPath)
{
    if (operationBusy || !sourcePath || !destinationPath)
        return false;

    operation = operationCopySingleFile;
    operationResult = false;
    operationStackFreeBytes = 0;
    operationSourcePath = sourcePath;
    operationDestinationPath = destinationPath;
    operationCaller = xTaskGetCurrentTaskHandle();
    operationBusy = true;

    if (xTaskCreate(operationTask, "SDOp", singleFileStackSize, this, 4, nullptr) != pdPASS)
    {
        operationBusy = false;
        operationCaller = nullptr;
        operationSourcePath = nullptr;
        operationDestinationPath = nullptr;
        return false;
    }

    xTaskNotifyWait(0, UINT32_MAX, nullptr, portMAX_DELAY);
    operationCaller = nullptr;
    operationBusy = false;

    return operationResult;
}
