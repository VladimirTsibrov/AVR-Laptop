#include "ProgressBar.h"
#include "../Form.h"
#include "../../Display.h"
#include <Arduino.h>

ProgressBar::ProgressBar(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
                         uint8_t minValue, uint8_t maxValue)
    : minValue(minValue), maxValue(maxValue), value(minValue)
{
    this->left = left;
    this->top = top;
    this->width = (width < 3) ? 3 : width;
    this->height = (height < 3) ? 3 : height;
    flags = FLAG_VISIBLE | FLAG_SHOW_PERCENTS;
    ownerForm = nullptr;
    eventMask = 0;
}

void ProgressBar::draw()
{
    if (!isVisible() || ownerForm == nullptr)
        return;

    ownerForm->drawRect(left, top, left + width, top + height, modeClear, true);
    ownerForm->drawRect(left, top, left + width, top + height, modeSet, isFocused());

    if (width < 2 || height < 2 || maxValue <= minValue)
        return;

    uint8_t innerLeft = left + 2;
    uint8_t innerTop = top + 2;
    uint8_t innerWidth = width - 3;
    uint8_t innerHeight = height - 3;

    uint8_t percent;
    if (value <= minValue)
        percent = 0;
    else if (value >= maxValue)
        percent = 100;
    else
        percent = map(value, minValue, maxValue, 0, 100);

    uint8_t fillWidth = (innerWidth * percent) / 100;

    if (fillWidth == 0)
        return;

    ownerForm->drawRect(innerLeft, innerTop, innerLeft + fillWidth - 1, innerTop + innerHeight - 1, modeSet, true);
    for (uint8_t x = innerLeft + 8; x < innerLeft + fillWidth; x += 8)
    {
        ownerForm->drawVLine(x, innerTop, innerHeight, modeInvert);
    }
    /*
        if (showPercents)
        {
            lcd.setPrintPos(innerLeft + innerWidth / 2, innerTop + innerHeight);
            lcd.print(percent);
        }
    */
}

bool ProgressBar::onKey(uint8_t key)
{
    (void)key;
    return false;
}

void ProgressBar::setValue(uint8_t value)
{
    if (value < minValue)
    {
        this->value = minValue;
    }
    else if (value > maxValue)
    {
        this->value = maxValue;
    }
    else
    {
        this->value = value;
    }
    draw();
    restoreTopLayer();
}

bool ProgressBar::isFocusable() const
{
    return false;
}

void ProgressBar::setRange(uint8_t minValue, uint8_t maxValue)
{
    this->minValue = minValue;
    this->maxValue = maxValue;
    if (this->value < this->minValue)
    {
        this->value = this->minValue;
    }
    if (this->value > this->maxValue)
    {
        this->value = this->maxValue;
    }
    draw();
    restoreTopLayer();
}
