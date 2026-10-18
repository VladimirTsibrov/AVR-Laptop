#include "List.h"
#include "../Form.h"
#include "../../Display.h"
#include "../../Keyboard.h"
#include "../UIEvents.h"
#include "../../Common.h"
#include <string.h>

List::List(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
{
    this->left = left;
    this->top = top;
    this->width = (width < 3) ? 3 : width;
    this->height = (height < 3) ? 3 : height;
    flags = FLAG_VISIBLE | LIST_FLAG_SCROLLBAR;
    ownerForm = nullptr;
    eventMask = 0;
    itemCount = 0;
    scrollOffset = 0;
    focusedRow = 0;
    rowHeight = lcd.fonts[lcd.getSystemFontIndex()].height + 1;
}

uint8_t List::getContentLeft() const
{
    return left + ((flags & LIST_FLAG_BORDER) ? 1 : 0);
}

uint8_t List::getContentTop() const
{
    return top + ((flags & LIST_FLAG_BORDER) ? 1 : 0);
}

uint8_t List::getContentWidth() const
{
    uint8_t reserved = ((flags & LIST_FLAG_BORDER) ? 2 : 0);
    if (needsScrollBar())
        reserved += 3;
    return (width > reserved) ? width - reserved : 0;
}

uint8_t List::getContentHeight() const
{
    return height - ((flags & LIST_FLAG_BORDER) ? 2 : 0);
}

uint8_t List::getVisibleRows() const
{
    uint8_t rows = getContentHeight() / rowHeight;
    return rows ? rows : 1;
}

bool List::needsScrollBar() const
{
    return (flags & LIST_FLAG_SCROLLBAR) && itemCount > getVisibleRows();
}

uint16_t List::getSelectedIndex() const
{
    return scrollOffset + focusedRow;
}

void List::setItemCount(uint16_t count)
{
    itemCount = count;
    normalizeSelection();
}

void List::setSelectedIndex(uint16_t index)
{
    if (itemCount == 0)
    {
        scrollOffset = 0;
        focusedRow = 0;
        return;
    }

    if (index >= itemCount)
        index = itemCount - 1;

    uint8_t rows = getVisibleRows();
    scrollOffset = (index >= rows) ? index - rows + 1 : 0;
    focusedRow = index - scrollOffset;
}

void List::setDrawBorder(bool value)
{
    if (value)
        flags |= LIST_FLAG_BORDER;
    else
        flags &= ~LIST_FLAG_BORDER;
    draw();
}

void List::setScrollBarVisible(bool value)
{
    if (value)
        flags |= LIST_FLAG_SCROLLBAR;
    else
        flags &= ~LIST_FLAG_SCROLLBAR;
    draw();
}

void List::normalizeSelection()
{
    if (itemCount == 0)
    {
        scrollOffset = 0;
        focusedRow = 0;
        return;
    }

    uint8_t rows = getVisibleRows();
    if (focusedRow >= rows)
        focusedRow = rows - 1;
    if (scrollOffset + focusedRow >= itemCount)
        setSelectedIndex(itemCount - 1);
}

bool List::getItemText(uint16_t index, char *buffer, uint8_t bufferSize)
{
    (void)index;
    if (bufferSize)
        buffer[0] = '\0';
    return false;
}

void List::drawFocusRow(uint8_t row)
{
    if (!ownerForm)
        return;

    uint8_t x = getContentLeft();
    uint8_t y = getContentTop() + row * rowHeight;
    uint8_t w = getContentWidth();
    if (w == 0)
        return;

    ownerForm->drawRect(x, y, x + w - 1, y + rowHeight - 1, modeInvert, true);
}

void List::draw()
{
    if (!isVisible() || ownerForm == nullptr)
        return;

    SchedulerSuspender lock;
    ownerForm->drawRect(left, top, left + width - 1, top + height - 1, modeClear, true);
    if (flags & LIST_FLAG_BORDER)
        ownerForm->drawRect(left, top, left + width - 1, top + height - 1, modeSet, false);

    normalizeSelection();

    uint8_t rows = getVisibleRows();
    uint8_t x = getContentLeft();
    uint8_t y = getContentTop();
    uint8_t w = getContentWidth();
    char text[LIST_TEXT_BUF_SIZE];

    for (uint8_t row = 0; row < rows; row++)
    {
        uint16_t index = scrollOffset + row;
        if (index >= itemCount)
            break;

        text[0] = '\0';
        getItemText(index, text, sizeof(text));
        text[sizeof(text) - 1] = '\0';

        if (w > 0)
        {
            ownerForm->setPrintRect(x, y + row * rowHeight, x + w - 1, y + (row + 1) * rowHeight - 1, true);
            ownerForm->print(text, modeSet);
        }
    }

    if (needsScrollBar())
    {
        uint8_t sx = getContentLeft() + getContentWidth() + 1;
        uint8_t sy = getContentTop();
        uint8_t sh = getContentHeight();
        uint8_t rows = getVisibleRows();
        uint8_t thumbHeight = ((uint32_t)rows * sh) / itemCount;
        if (thumbHeight < 2)
            thumbHeight = 2;
        if (thumbHeight > sh)
            thumbHeight = sh;

        uint8_t maxThumbTop = sh - thumbHeight;
        uint8_t thumbTop = 0;
        if (itemCount > rows && maxThumbTop > 0)
            thumbTop = ((uint32_t)scrollOffset * maxThumbTop) / (itemCount - rows);

        ownerForm->drawVLine(sx, sy, sh, modeSet, true);
        ownerForm->drawVLine(sx + 1, sy + thumbTop, thumbHeight, modeSet, false);
    }

    ownerForm->resetPrintRect();
    if (itemCount > 0)
        drawFocusRow(focusedRow);
}

bool List::onKey(uint8_t key)
{
    if (!isVisible() || !isFocused())
        return false;

    bool changed = false;
    uint8_t rows = getVisibleRows();

    if (key == keyUp)
    {
        if (focusedRow > 0)
        {
            drawFocusRow(focusedRow);
            focusedRow--;
            drawFocusRow(focusedRow);
            changed = true;
        }
        else if (scrollOffset > 0)
        {
            scrollOffset--;
            draw();
            changed = true;
        }
    }
    else if (key == keyDown)
    {
        if (itemCount > 0 && getSelectedIndex() < itemCount - 1)
        {
            if (focusedRow + 1 < rows)
            {
                drawFocusRow(focusedRow);
                focusedRow++;
                drawFocusRow(focusedRow);
            }
            else
            {
                scrollOffset++;
                draw();
            }
            changed = true;
        }
    }
    else if (key == keyEnter)
    {
        triggerEvent(EVENT_CLICK);
        return true;
    }
    if (changed)
        restoreTopLayer();

    if (changed)
        triggerEvent(EVENT_VALUE_CHANGED);

    return changed;
}
