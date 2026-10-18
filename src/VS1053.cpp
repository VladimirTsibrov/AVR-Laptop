#include "VS1053.h"
#include "Common.h"
#include <Arduino.h>
#include <SPI.h>

namespace
{
    const uint8_t VS1053_DCS = 27;
    const uint8_t VS1053_CS = 26;
    const uint8_t VS1053_DREQ = 12;
    const uint8_t VS1053_RESET = 13;

    const uint8_t SCI_WRITE = 0x02;
    const uint8_t SCI_MODE = 0x00;
    const uint8_t SCI_CLOCKF = 0x03;
    const uint8_t SCI_AUDATA = 0x05;
    const uint8_t SCI_VOLUME = 0x0B;

    const uint16_t SM_SDINEW = 0x0800;
    const uint16_t SM_RESET = 0x0004;
}

VS1053 vs1053;

bool VS1053::isOwnedByCurrentTask() const
{
    return owner != nullptr && owner == xTaskGetCurrentTaskHandle();
}

bool VS1053::waitReady(uint16_t timeoutMs) const
{
    uint32_t deadline = millis() + timeoutMs;
    while (digitalRead(VS1053_DREQ) == LOW)
    {
        if ((int32_t)(millis() - deadline) >= 0)
            return false;
        vTaskDelay(1);
    }
    return true;
}

void VS1053::sciWrite(uint8_t address, uint16_t value)
{
    if (!waitReady(100))
        return;

    SchedulerSuspender lock;
    SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE0));
    digitalWrite(VS1053_CS, LOW);
    SPI.transfer(SCI_WRITE);
    SPI.transfer(address);
    SPI.transfer(value >> 8);
    SPI.transfer(value & 0xFF);
    digitalWrite(VS1053_CS, HIGH);
    SPI.endTransaction();
}

void VS1053::reset()
{
    digitalWrite(VS1053_RESET, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
    digitalWrite(VS1053_RESET, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));

    sciWrite(SCI_MODE, SM_SDINEW | SM_RESET);
    vTaskDelay(pdMS_TO_TICKS(100));
    sciWrite(SCI_CLOCKF, 0xE430);
    sciWrite(SCI_AUDATA, 0xAC45);
    applyVolume();
}

bool VS1053::acquire()
{
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    if (owner != nullptr)
        return owner == currentTask;

    pinMode(VS1053_DREQ, INPUT);
    pinMode(VS1053_CS, OUTPUT);
    pinMode(VS1053_DCS, OUTPUT);
    pinMode(VS1053_RESET, OUTPUT);
    digitalWrite(VS1053_CS, HIGH);
    digitalWrite(VS1053_DCS, HIGH);

    readPos = 0;
    writePos = 0;
    used = 0;
    owner = currentTask;

    reset();
    return true;
}

void VS1053::release()
{
    if (!isOwnedByCurrentTask())
        return;

    SchedulerSuspender lock;
    owner = nullptr;
    used = 0;
    readPos = 0;
    writePos = 0;
}

uint16_t VS1053::available() const
{
    SchedulerSuspender lock;
    return used;
}

uint16_t VS1053::freeSpace() const
{
    SchedulerSuspender lock;
    return BUFFER_SIZE - used;
}

void VS1053::clearBuffer()
{
    if (!isOwnedByCurrentTask())
        return;

    SchedulerSuspender lock;
    readPos = 0;
    writePos = 0;
    used = 0;
}

uint16_t VS1053::feedFile(FsFile &file)
{
    if (!file || !isOwnedByCurrentTask())
        return 0;

    SchedulerSuspender lock;
    uint16_t free = BUFFER_SIZE - used;
    uint16_t toEnd = BUFFER_SIZE - writePos;
    uint16_t count = free < toEnd ? free : toEnd;
    if (count > MAX_FILE_READ_SIZE)
        count = MAX_FILE_READ_SIZE;
    if (count == 0)
        return 0;

    size_t read = file.read(buffer + writePos, count);
    writePos += read;
    if (writePos == BUFFER_SIZE)
        writePos = 0;
    used += read;
    return read;
}

void VS1053::service()
{
    if (!owner || used == 0 || digitalRead(VS1053_DREQ) == LOW)
        return;

    SchedulerSuspender lock;
    SPI.beginTransaction(SPISettings(12000000, MSBFIRST, SPI_MODE0));
    for (uint8_t block = 0; block < MAX_BLOCKS_PER_SERVICE && used && digitalRead(VS1053_DREQ) == HIGH; block++)
    {
        uint8_t count = DATA_BLOCK_SIZE;
        if (count > used)
            count = used;
        uint16_t toEnd = BUFFER_SIZE - readPos;
        if (count > toEnd)
            count = toEnd;

        digitalWrite(VS1053_DCS, LOW);
        for (uint8_t i = 0; i < count; i++)
            SPI.transfer(buffer[readPos + i]);
        digitalWrite(VS1053_DCS, HIGH);

        readPos += count;
        if (readPos == BUFFER_SIZE)
            readPos = 0;
        used -= count;
    }
    SPI.endTransaction();
}

void VS1053::applyVolume()
{
    static const uint8_t attenuation[16] = {
        254, 70, 60, 52, 45, 39, 34, 29,
        24, 20, 16, 12, 8, 5, 2, 0};
    uint8_t attenuationValue = muted ? 254 : attenuation[volumeLevel];
    uint16_t value = ((uint16_t)attenuationValue << 8) | attenuationValue;
    sciWrite(SCI_VOLUME, value);
}

void VS1053::setVolume(uint8_t level)
{
    if (level > 15)
        level = 15;
    volumeLevel = level;
    if (owner)
        applyVolume();
}

void VS1053::increaseVolume()
{
    if (volumeLevel < 15)
        setVolume(volumeLevel + 1);
}

void VS1053::decreaseVolume()
{
    if (volumeLevel > 0)
        setVolume(volumeLevel - 1);
}

void VS1053::setMuted(bool value)
{
    muted = value;
    if (owner)
        applyVolume();
}
