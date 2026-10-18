#ifndef DS3231_H
#define DS3231_H

#include <Arduino.h>
#include <Wire.h>

struct RTCTime
{
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
};

struct RTCDate
{
    uint8_t day = 1;
    uint8_t month = 1;
    uint16_t year = 2000;
};

struct RTCDateTime
{
    RTCTime time;
    RTCDate date;
};

class DS3231
{
private:
    uint8_t i2c_addr;

    uint8_t bcdToDec(uint8_t val);
    uint8_t decToBcd(uint8_t val);
    bool readBytes(uint8_t reg, uint8_t *data, uint8_t len);
    bool writeBytes(uint8_t reg, uint8_t *data, uint8_t len);

public:
    DS3231(uint8_t addr = 0x68);

    bool getTime(RTCTime &value);
    bool getDate(RTCDate &value);
    bool getDateTime(RTCDateTime &value);

    // === ВОЗВРАТ ЗНАЧЕНИЙ (новые функции) ===
    RTCTime getTime();
    RTCDate getDate();
    RTCDateTime getDateTime();

    bool setTime(const RTCTime &value);
    bool setDate(const RTCDate &value);
    bool setDateTime(const RTCDateTime &value);

    // === ПЕРЕГРУЗКИ ДЛЯ РАЗНЫХ ФОРМАТОВ ===
    bool getTime(uint8_t &hour, uint8_t &min, uint8_t &sec);
    bool getTime(uint8_t &hour, uint8_t &min);
    uint8_t getSeconds();
    bool getTimeStr(char *buffer, uint8_t size);
    bool getTimeShortStr(char *buffer, uint8_t size);
    bool getDateTime(uint8_t &sec, uint8_t &min, uint8_t &hour,
                     uint8_t &day, uint8_t &month, uint16_t &year);
    bool getDate(uint8_t &day, uint8_t &month, uint16_t &year);

    // === УСТАНОВКА ВРЕМЕНИ ===
    bool setTime(uint8_t hour, uint8_t min, uint8_t sec = 0);
    bool setDateTime(uint8_t hour, uint8_t min, uint8_t sec,
                     uint8_t day, uint8_t month, uint16_t year);

    // === БУДИЛЬНИКИ ===
    bool setAlarm1(uint8_t hour, uint8_t min, uint8_t sec = 0, uint8_t mode = 0x07);
    bool setAlarm2(uint8_t hour, uint8_t min, uint8_t mode = 0x06);
    bool clearAlarm1();
    bool clearAlarm2();
    bool isAlarm1Triggered();
    bool isAlarm2Triggered();

    // === УТИЛИТЫ ===
    void clearAlarms();
    void enableAlarmInterrupt(bool enable);
    float getTemperature();
    bool isConnected();
};
extern DS3231 rtc;
#endif