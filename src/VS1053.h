#ifndef VS1053_H
#define VS1053_H

#include <inttypes.h>
#include <SdFat.h>
#include "Arduino_FreeRTOS.h"

class VS1053
{
public:
    bool acquire();
    void release();

    bool isInUse() const { return owner != nullptr; }
    bool isOwnedByCurrentTask() const;

    uint16_t available() const;
    uint16_t freeSpace() const;
    void clearBuffer();
    uint16_t feedFile(FsFile &file);

    void service();
    uint8_t getVolume() const { return volumeLevel; }
    void setVolume(uint8_t level);
    void increaseVolume();
    void decreaseVolume();
    void setMuted(bool muted);

private:
    static const uint8_t DATA_BLOCK_SIZE = 32;
    static const uint8_t BUFFER_SIZE = 96;
    static const uint8_t MAX_BLOCKS_PER_SERVICE = 3;
    static const uint8_t MAX_FILE_READ_SIZE = 64;

    uint8_t buffer[BUFFER_SIZE];
    uint16_t readPos = 0;
    uint16_t writePos = 0;
    uint16_t used = 0;
    uint8_t volumeLevel = 12;
    bool muted = false;
    TaskHandle_t owner = nullptr;

    bool waitReady(uint16_t timeoutMs) const;
    void reset();
    void sciWrite(uint8_t address, uint16_t value);
    void applyVolume();
};

extern VS1053 vs1053;

#endif
