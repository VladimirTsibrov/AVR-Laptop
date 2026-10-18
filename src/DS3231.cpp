#include "DS3231.h"

DS3231 rtc;

DS3231::DS3231(uint8_t addr) : i2c_addr(addr)
{
    Wire.begin();
}

uint8_t DS3231::bcdToDec(uint8_t val)
{
    return ((val >> 4) * 10) + (val & 0x0F);
}

uint8_t DS3231::decToBcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

bool DS3231::readBytes(uint8_t reg, uint8_t *data, uint8_t len)
{
    Wire.beginTransmission(i2c_addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
        return false;

    Wire.requestFrom(i2c_addr, len);
    for (uint8_t i = 0; i < len && Wire.available(); i++)
    {
        data[i] = Wire.read();
    }
    return true;
}

bool DS3231::writeBytes(uint8_t reg, uint8_t *data, uint8_t len)
{
    Wire.beginTransmission(i2c_addr);
    Wire.write(reg);
    for (uint8_t i = 0; i < len; i++)
    {
        Wire.write(data[i]);
    }
    return Wire.endTransmission() == 0;
}

// === ВРЕМЯ ===

bool DS3231::getTime(RTCTime &value)
{
    uint8_t hour, min, sec;
    if (!getTime(hour, min, sec))
        return false;
    value.hour = hour;
    value.minute = min;
    value.second = sec;
    return true;
}

bool DS3231::getDate(RTCDate &value)
{
    uint8_t day, month;
    uint16_t year;
    if (!getDate(day, month, year))
        return false;
    value.day = day;
    value.month = month;
    value.year = year;
    return true;
}

bool DS3231::getDateTime(RTCDateTime &value)
{
    uint8_t sec, min, hour, day, month;
    uint16_t year;
    if (!getDateTime(sec, min, hour, day, month, year))
        return false;
    value.time.hour = hour;
    value.time.minute = min;
    value.time.second = sec;
    value.date.day = day;
    value.date.month = month;
    value.date.year = year;
    return true;
}

// === НОВЫЕ ФУНКЦИИ: возврат значений ===

RTCTime DS3231::getTime()
{
    RTCTime value;
    getTime(value);
    return value;
}

RTCDate DS3231::getDate()
{
    RTCDate value;
    getDate(value);
    return value;
}

RTCDateTime DS3231::getDateTime()
{
    RTCDateTime value;
    getDateTime(value);
    return value;
}

bool DS3231::setTime(const RTCTime &value)
{
    return setTime(value.hour, value.minute, value.second);
}

bool DS3231::setDate(const RTCDate &value)
{
    uint8_t data[3] = {
        decToBcd(value.day),
        decToBcd(value.month),
        decToBcd(value.year % 100)};
    return writeBytes(0x04, data, 3);
}

bool DS3231::setDateTime(const RTCDateTime &value)
{
    return setDateTime(value.time.hour, value.time.minute, value.time.second,
                       value.date.day, value.date.month, value.date.year);
}

bool DS3231::getTime(uint8_t &hour, uint8_t &min, uint8_t &sec)
{
    uint8_t data[3];
    if (!readBytes(0x00, data, 3))
        return false;

    sec = bcdToDec(data[0] & 0x7F);
    min = bcdToDec(data[1] & 0x7F);
    hour = bcdToDec(data[2] & 0x3F);
    return true;
}

bool DS3231::getTime(uint8_t &hour, uint8_t &min)
{
    uint8_t s;
    return getTime(hour, min, s);
}

uint8_t DS3231::getSeconds()
{
    uint8_t data;
    return readBytes(0x00, &data, 1) ? bcdToDec(data & 0x7F) : 0;
}

bool DS3231::getTimeStr(char *buffer, uint8_t size)
{
    if (size < 9)
        return false;

    uint8_t h, m, s;
    if (!getTime(h, m, s))
        return false;

    buffer[0] = (h / 10) + '0';
    buffer[1] = (h % 10) + '0';
    buffer[2] = ':';
    buffer[3] = (m / 10) + '0';
    buffer[4] = (m % 10) + '0';
    buffer[5] = ':';
    buffer[6] = (s / 10) + '0';
    buffer[7] = (s % 10) + '0';
    buffer[8] = '\0';
    return true;
}

bool DS3231::getTimeShortStr(char *buffer, uint8_t size)
{
    if (size < 6)
        return false;

    uint8_t h, m;
    if (!getTime(h, m))
        return false;

    buffer[0] = (h / 10) + '0';
    buffer[1] = (h % 10) + '0';
    buffer[2] = ':';
    buffer[3] = (m / 10) + '0';
    buffer[4] = (m % 10) + '0';
    buffer[5] = '\0';
    return true;
}

bool DS3231::getDateTime(uint8_t &sec, uint8_t &min, uint8_t &hour,
                         uint8_t &day, uint8_t &month, uint16_t &year)
{
    uint8_t data[7];
    if (!readBytes(0x00, data, 7))
        return false;

    sec = bcdToDec(data[0] & 0x7F);
    min = bcdToDec(data[1] & 0x7F);
    hour = bcdToDec(data[2] & 0x3F);
    day = bcdToDec(data[4] & 0x3F);
    month = bcdToDec(data[5] & 0x1F);
    year = bcdToDec(data[6]) + 2000;
    return true;
}

bool DS3231::getDate(uint8_t &day, uint8_t &month, uint16_t &year)
{
    uint8_t s, m, h;
    return getDateTime(s, m, h, day, month, year);
}

bool DS3231::setTime(uint8_t hour, uint8_t min, uint8_t sec)
{
    uint8_t data[3] = {decToBcd(sec), decToBcd(min), decToBcd(hour)};
    return writeBytes(0x00, data, 3);
}

bool DS3231::setDateTime(uint8_t hour, uint8_t min, uint8_t sec,
                         uint8_t day, uint8_t month, uint16_t year)
{
    uint8_t data[7] = {
        decToBcd(sec), decToBcd(min), decToBcd(hour),
        0x01, decToBcd(day), decToBcd(month), decToBcd(year % 100)};
    return writeBytes(0x00, data, 7);
}

// === БУДИЛЬНИКИ ===

bool DS3231::setAlarm1(uint8_t hour, uint8_t min, uint8_t sec, uint8_t mode)
{
    uint8_t data[4];

    data[0] = decToBcd(sec) | ((mode & 0x01) ? 0x80 : 0);
    data[1] = decToBcd(min) | ((mode & 0x02) ? 0x80 : 0);
    data[2] = decToBcd(hour) | ((mode & 0x04) ? 0x80 : 0);
    data[3] = 0x80;

    return writeBytes(0x07, data, 4);
}

bool DS3231::setAlarm2(uint8_t hour, uint8_t min, uint8_t mode)
{
    uint8_t data[3];

    data[0] = decToBcd(min) | ((mode & 0x02) ? 0x80 : 0);
    data[1] = decToBcd(hour) | ((mode & 0x04) ? 0x80 : 0);
    data[2] = 0x80;

    return writeBytes(0x0B, data, 3);
}

bool DS3231::clearAlarm1()
{
    uint8_t status;
    if (!readBytes(0x0F, &status, 1))
        return false;

    status &= ~0x01;
    return writeBytes(0x0F, &status, 1);
}

bool DS3231::clearAlarm2()
{
    uint8_t status;
    if (!readBytes(0x0F, &status, 1))
        return false;

    status &= ~0x02;
    return writeBytes(0x0F, &status, 1);
}

bool DS3231::isAlarm1Triggered()
{
    uint8_t status;
    return readBytes(0x0F, &status, 1) ? (status & 0x01) != 0 : false;
}

bool DS3231::isAlarm2Triggered()
{
    uint8_t status;
    return readBytes(0x0F, &status, 1) ? (status & 0x02) != 0 : false;
}

// === УТИЛИТЫ ===

void DS3231::clearAlarms()
{
    uint8_t status;
    if (readBytes(0x0F, &status, 1))
    {
        status &= ~0x03;
        writeBytes(0x0F, &status, 1);
    }
}

void DS3231::enableAlarmInterrupt(bool enable)
{
    uint8_t control;
    if (readBytes(0x0E, &control, 1))
    {
        if (enable)
        {
            control |= 0x05;
        }
        else
        {
            control &= ~0x05;
        }
        writeBytes(0x0E, &control, 1);
    }
}

float DS3231::getTemperature()
{
    uint8_t data[2];
    if (!readBytes(0x11, data, 2))
        return -1000;

    int16_t temp = (data[0] << 8) | data[1];
    temp >>= 6;
    if (temp & 0x200)
        temp |= 0xFC00;
    return temp * 0.25;
}

bool DS3231::isConnected()
{
    Wire.beginTransmission(i2c_addr);
    return Wire.endTransmission() == 0;
}