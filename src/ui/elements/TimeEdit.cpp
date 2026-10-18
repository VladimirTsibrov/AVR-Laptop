#include "TimeEdit.h"
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

    bool isPartialHourValid(const char *text)
    {
        if (text[0] == '\0')
            return true;
        if (text[1] == '\0')
            return text[0] >= '0' && text[0] <= '2';
        if (text[0] == '2')
            return text[1] >= '0' && text[1] <= '3';
        return text[1] >= '0' && text[1] <= '9';
    }

    bool isPartialMinuteOrSecondValid(const char *text)
    {
        if (text[0] == '\0')
            return true;
        if (text[1] == '\0')
            return text[0] >= '0' && text[0] <= '5';
        return text[0] <= '5' && text[1] >= '0' && text[1] <= '9';
    }

    bool isPartialTimeTextValid(const char *text)
    {
        char hourBuf[3] = {0};
        char minuteBuf[3] = {0};
        char secondBuf[3] = {0};

        if (text[0] == '\0')
            return true;
        if (text[2] != ':' && text[2] != '\0')
            return false;

        for (uint8_t i = 0; i < 2; ++i)
        {
            if (text[i] == '\0')
                break;
            if (!isDigitChar(text[i]))
                return false;
            hourBuf[i] = text[i];
        }

        if (hourBuf[0] != '\0' && !isPartialHourValid(hourBuf))
            return false;
        if (text[2] == '\0')
            return true;
        if (text[2] != ':')
            return false;

        for (uint8_t i = 0; i < 2; ++i)
        {
            if (text[3 + i] == '\0' || text[3 + i] == ':')
                break;
            if (!isDigitChar(text[3 + i]))
                return false;
            minuteBuf[i] = text[3 + i];
        }

        if (minuteBuf[0] != '\0' && !isPartialMinuteOrSecondValid(minuteBuf))
            return false;
        if (text[5] == '\0')
            return true;
        if (text[5] != ':')
            return false;

        for (uint8_t i = 0; i < 2; ++i)
        {
            if (text[6 + i] == '\0')
                break;
            if (!isDigitChar(text[6 + i]))
                return false;
            secondBuf[i] = text[6 + i];
        }

        if (secondBuf[0] != '\0' && !isPartialMinuteOrSecondValid(secondBuf))
            return false;

        RTCTime value;
        return TimeEdit::parseTimeText(text, value, true);
    }

} // namespace

TimeEdit::TimeEdit(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
                   const RTCTime &value, uint8_t fontIndex)
    : Edit(left, top, width, height, "", 8, fontIndex)
{
    setValue(value);
}

bool TimeEdit::isValidTimeParts(uint8_t hour, uint8_t minute, uint8_t second)
{
    if (hour > 23)
        return false;
    if (minute > 59)
        return false;
    if (second > 59)
        return false;
    return true;
}

bool TimeEdit::parseTimeText(const char *text, RTCTime &value, bool allowPartial)
{
    if (!text || text[0] == '\0')
    {
        value = RTCTime();
        return allowPartial;
    }

    char tmp[9];
    strncpy(tmp, text, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    if (strlen(tmp) > 8)
        return false;

    RTCTime parsed = RTCTime();
    uint8_t hour = 0, minute = 0, second = 0;

    if (tmp[2] == ':' || tmp[2] == '\0')
    {
        if (tmp[0] != '\0')
        {
            if (!isDigitChar(tmp[0]))
                return false;
            hour = charToDigit(tmp[0]);
            if (tmp[1] != '\0' && tmp[1] != ':')
            {
                if (!isDigitChar(tmp[1]))
                    return false;
                hour = hour * 10 + charToDigit(tmp[1]);
            }
        }
        if (tmp[2] == '\0')
            return allowPartial;
        if (tmp[2] != ':')
            return false;
        if (tmp[3] != '\0')
        {
            if (!isDigitChar(tmp[3]))
                return false;
            minute = charToDigit(tmp[3]);
            if (tmp[4] != '\0' && tmp[4] != ':')
            {
                if (!isDigitChar(tmp[4]))
                    return false;
                minute = minute * 10 + charToDigit(tmp[4]);
            }
        }
        if (tmp[5] == '\0')
            return allowPartial;
        if (tmp[5] != ':')
            return false;
        if (tmp[6] != '\0')
        {
            if (!isDigitChar(tmp[6]))
                return false;
            second = charToDigit(tmp[6]);
            if (tmp[7] != '\0')
            {
                if (!isDigitChar(tmp[7]))
                    return false;
                second = second * 10 + charToDigit(tmp[7]);
            }
        }
    }

    if (hour > 0 && minute > 0 && second > 0)
    {
        if (!isValidTimeParts(hour, minute, second))
            return false;
    }
    else if (hour > 0 && minute > 0)
    {
        if (hour > 23 || minute > 59)
            return false;
    }
    else if (hour > 0)
    {
        if (hour > 23)
            return false;
    }
    else if (minute > 0)
    {
        if (minute > 59)
            return false;
    }
    else if (second > 0)
    {
        if (second > 59)
            return false;
    }

    if (hour > 0)
        parsed.hour = hour;
    if (minute > 0)
        parsed.minute = minute;
    if (second > 0)
        parsed.second = second;
    value = parsed;
    return true;
}

void TimeEdit::setValue(const RTCTime &value)
{
    char buf[9];
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", value.hour, value.minute, value.second);
    Edit::setText(buf);
}

RTCTime TimeEdit::getValue() const
{
    RTCTime value = RTCTime();
    if (parseTimeText(getText(), value, false))
        return value;
    return RTCTime();
}

bool TimeEdit::isValid() const
{
    RTCTime value = RTCTime();
    return parseTimeText(getText(), value, false);
}

bool TimeEdit::canInsertCharAt(uint8_t index, char ch) const
{
    if (index > 7 || index == 2 || index == 5)
        return false;
    return isDigitChar(ch);
}

uint8_t TimeEdit::getNextCursorPosAfterInsert(uint8_t index) const
{
    if (index == 1)
        return 3;
    if (index == 4)
        return 6;
    if (index >= 7)
        return 7;
    return index + 1;
}

bool TimeEdit::overwritesCharAt(uint8_t index) const
{
    return index != 2 && index != 5;
}

uint8_t TimeEdit::getCursorPosAfterMove(uint8_t currentPos, int8_t delta) const
{
    int8_t nextPos = (int8_t)currentPos + delta;
    if (nextPos < 0)
        return 0;
    if (nextPos > 7)
        return 7;
    if (nextPos == 2 || nextPos == 5)
        nextPos += delta;
    return (uint8_t)nextPos;
}
