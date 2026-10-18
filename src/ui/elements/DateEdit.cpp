#include "DateEdit.h"
#include <string.h>
#include <Arduino.h>

namespace
{

    bool isDigitChar(char ch)
    {
        return ch >= '0' && ch <= '9';
    }

    uint8_t charToDigit(char ch)
    {
        return (uint8_t)(ch - '0');
    }

    bool isPartialDayValid(const char *text)
    {
        const char *p = text;
        if (p[0] == '\0')
            return true;
        if (p[1] == '\0')
            return p[0] >= '0' && p[0] <= '3';
        if (p[0] < '0' || p[0] > '3')
            return false;
        if (p[0] == '3' && (p[1] < '0' || p[1] > '1'))
            return false;
        if (p[0] != '3' && (p[1] < '0' || p[1] > '9'))
            return false;
        return true;
    }

    bool isPartialMonthValid(const char *text)
    {
        const char *p = text;
        if (p[0] == '\0')
            return true;
        if (p[1] == '\0')
            return p[0] >= '0' && p[0] <= '1';
        if (p[0] == '1')
            return p[1] >= '0' && p[1] <= '2';
        if (p[0] == '0')
            return p[1] >= '0' && p[1] <= '9';
        return false;
    }

    bool isPartialYearValid(const char *text)
    {
        if (text[0] == '\0')
            return true;
        if (text[1] == '\0')
            return true;
        if (text[2] == '\0')
            return true;
        if (text[3] == '\0')
            return true;
        for (uint8_t i = 0; i < 4; ++i)
        {
            if (!isDigitChar(text[i]))
                return false;
        }
        uint16_t year = (uint16_t)((charToDigit(text[0]) * 1000) +
                                   (charToDigit(text[1]) * 100) +
                                   (charToDigit(text[2]) * 10) +
                                   charToDigit(text[3]));
        return year >= 2000 && year <= 2099;
    }

    bool isPartialDateTextValid(const char *text)
    {
        char dayBuf[3] = {0};
        char monthBuf[3] = {0};
        char yearBuf[5] = {0};

        if (text[0] == '\0')
            return true;
        if (text[2] != '.')
            return true;

        uint8_t dayLen = 0;
        uint8_t monthLen = 0;
        uint8_t yearLen = 0;

        for (uint8_t i = 0; i < 2; ++i)
        {
            if (text[i] == '\0')
                break;
            if (!isDigitChar(text[i]))
                return false;
            dayBuf[i] = text[i];
            dayLen++;
        }

        if (dayLen > 0 && !isPartialDayValid(dayBuf))
            return false;

        if (text[3] == '\0')
            return true;
        if (text[3] == '.' || text[5] == '\0')
        {
            if (text[3] != '.')
                return false;
            if (text[4] == '\0')
                return true;
            if (text[4] == '.' || text[5] == '\0')
                return true;
        }

        if (text[3] != '.')
            return false;
        for (uint8_t i = 0; i < 2; ++i)
        {
            if (text[4 + i] == '\0' || text[4 + i] == '.')
                break;
            if (!isDigitChar(text[4 + i]))
                return false;
            monthBuf[i] = text[4 + i];
            monthLen++;
        }

        if (monthLen > 0 && !isPartialMonthValid(monthBuf))
            return false;

        if (text[5] == '\0')
            return true;
        if (text[5] != '.')
            return false;

        for (uint8_t i = 0; i < 4; ++i)
        {
            if (text[6 + i] == '\0')
                break;
            if (!isDigitChar(text[6 + i]))
                return false;
            yearBuf[i] = text[6 + i];
            yearLen++;
        }

        if (yearLen > 0 && !isPartialYearValid(yearBuf))
            return false;

        if (dayLen == 0 || monthLen == 0 || yearLen == 0)
            return true;

        RTCDate value;
        if (!DateEdit::parseDateText(text, value, false))
            return false;
        return true;
    }

} // namespace

DateEdit::DateEdit(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
                   const RTCDate &value, uint8_t fontIndex)
    : Edit(left, top, width, height, "", 10, fontIndex)
{
    setValue(value);
}

bool DateEdit::isLeapYear(uint16_t year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

uint8_t DateEdit::daysInMonth(uint8_t month, uint16_t year)
{
    switch (month)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        return 31;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    case 2:
        return isLeapYear(year) ? 29 : 28;
    default:
        return 0;
    }
}

bool DateEdit::isValidDateParts(uint8_t day, uint8_t month, uint16_t year)
{
    if (month < 1 || month > 12)
        return false;
    if (year < 2000 || year > 2099)
        return false;
    if (day < 1 || day > daysInMonth(month, year))
        return false;
    return true;
}

bool DateEdit::parseDateText(const char *text, RTCDate &value, bool allowPartial)
{
    if (!text || text[0] == '\0')
    {
        value = RTCDate();
        return allowPartial;
    }

    if (strlen(text) != 10 || text[2] != '.' || text[5] != '.')
        return false;

    const uint8_t digitPositions[] = {0, 1, 3, 4, 6, 7, 8, 9};
    for (uint8_t i = 0; i < sizeof(digitPositions); ++i)
    {
        if (!isDigitChar(text[digitPositions[i]]))
            return false;
    }

    uint8_t day = (uint8_t)(charToDigit(text[0]) * 10 + charToDigit(text[1]));
    uint8_t month = (uint8_t)(charToDigit(text[3]) * 10 + charToDigit(text[4]));
    uint16_t year = (uint16_t)(charToDigit(text[6]) * 1000 +
                               charToDigit(text[7]) * 100 +
                               charToDigit(text[8]) * 10 +
                               charToDigit(text[9]));

    if (!isValidDateParts(day, month, year))
        return false;

    value.day = day;
    value.month = month;
    value.year = year;
    return true;
}

void DateEdit::setValue(const RTCDate &value)
{
    char buf[11];
    snprintf(buf, sizeof(buf), "%02u.%02u.%04u", value.day, value.month, value.year);
    Edit::setText(buf);
}

RTCDate DateEdit::getValue() const
{
    RTCDate value = RTCDate();
    if (parseDateText(getText(), value, false))
        return value;
    return RTCDate();
}

bool DateEdit::isValid() const
{
    RTCDate value = RTCDate();
    return parseDateText(getText(), value, false);
}

bool DateEdit::canInsertCharAt(uint8_t index, char ch) const
{
    if (index > 9 || index == 2 || index == 5)
        return false;
    return isDigitChar(ch);
}

uint8_t DateEdit::getNextCursorPosAfterInsert(uint8_t index) const
{
    if (index == 1)
        return 3;
    if (index == 4)
        return 6;
    if (index >= 9)
        return 9;
    return index + 1;
}

bool DateEdit::overwritesCharAt(uint8_t index) const
{
    return index != 2 && index != 5;
}

uint8_t DateEdit::getCursorPosAfterMove(uint8_t currentPos, int8_t delta) const
{
    int8_t nextPos = (int8_t)currentPos + delta;
    if (nextPos < 0)
        return 0;
    if (nextPos > 9)
        return 9;
    if (nextPos == 2 || nextPos == 5)
        nextPos += delta;
    return (uint8_t)nextPos;
}
