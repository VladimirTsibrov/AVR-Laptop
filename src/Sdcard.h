#ifndef SDCARD_H
#define SDCARD_H
#include <SPI.h>
#include <SdFat.h>
#include "Arduino_FreeRTOS.h"

#define MAX_PATH_LEN 96

class SDCard
{
public:
    bool begin(uint8_t chipSelectPin, uint32_t maxSck);
    FsFile open(const char *path, oflag_t flags = O_RDONLY);
    bool exists(const char *path);
    bool mkdir(const char *path, bool createParents = false);
    bool remove(const char *path);
    bool rmdir(const char *path);
    bool rename(const char *oldPath, const char *newPath);
    uint8_t sdErrorCode() const;
    uint32_t sdErrorData() const;

    // Выполняются в краткоживущей задаче, чтобы не раздувать стек вызывающей задачи.
    bool copyTree(const char *sourcePath, const char *destinationPath);
    bool removeTree(const char *path);

    // Простое копирование одного файла (для тестирования расхода стека)
    bool copySingleFile(const char *sourcePath, const char *destinationPath);

    // Асинхронное копирование файла (не блокирует вызывающую задачу)
    bool copyFileAsync(const char *sourcePath, const char *destinationPath, TaskHandle_t caller);
    bool checkAsyncOperationComplete(bool *result);

private:
    static const uint8_t navigationDepth = 8;
    static const uint8_t fileNameCapacity = 28;
    static const uint16_t singleFileStackSize = 800;
    static const uint16_t treeStackSize = 800;

    enum Operation : uint8_t
    {
        operationCopy,
        operationRemove,
        operationCopySingleFile
    };

    SdFs filesystem;
    const char *operationSourcePath = nullptr;
    const char *operationDestinationPath = nullptr;
    TaskHandle_t operationCaller = nullptr;
    volatile bool operationResult = false;
    volatile bool operationBusy = false;
    volatile uint16_t operationStackFreeBytes = 0;
    Operation operation;

    static void operationTask(void *parameter);
    bool appendPath(char *path, const char *part) const;
    bool removeLastPathComponent(char *path) const;
    bool copyFile(const char *sourcePath, const char *destinationPath);
    bool copyTreeInternal();
    bool removeTreeInternal();
    bool copySingleFileInternal();
};

extern SDCard sdCard;

#endif // SDCARD_H
