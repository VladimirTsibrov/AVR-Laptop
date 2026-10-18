#include "TextBox.h"
#include "../Form.h"
#include "../../Display.h"
#include "../../Keyboard.h"
#include <string.h>
#include <Arduino.h>

Edit::Edit(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
           const char *text, uint8_t maxLength, uint8_t fontIndex)
{
    this->left = left;
    this->top = top;
    this->width = (width < 4) ? 4 : width;
    this->height = (height < 3) ? 3 : height;
    this->flags = FLAG_VISIBLE;
    this->ownerForm = nullptr;
    this->eventMask = 0;
    this->maxLength = (maxLength > MAX_TEXT_LEN) ? MAX_TEXT_LEN : maxLength;
    this->fontIndex = fontIndex;
    this->cursorPos = 0;
    this->scrollOffset = 0;
    setFlag(FLAG_CURSOR_VISIBLE, false);
    this->lastBlinkTime = 0;
    this->cursorOnDurationTicks = pdMS_TO_TICKS(400);
    this->cursorOffDurationTicks = pdMS_TO_TICKS(700);
    textBuffer[0] = '\0';
    setText(text);
}

bool Edit::canInsertCharAt(uint8_t index, char ch) const
{
    (void)index;
    return ch >= 32 && ch <= 126;
}

uint8_t Edit::getCursorPosAfterMove(uint8_t currentPos, int8_t delta) const
{
    int16_t newPos = (int16_t)currentPos + delta;
    if (newPos < 0)
        newPos = 0;
    if (newPos > (int16_t)strlen(textBuffer))
        newPos = strlen(textBuffer);
    return (uint8_t)newPos;
}

void Edit::setText(const char *text)
{
    if (!text)
    {
        textBuffer[0] = '\0';
    }
    else
    {
        uint8_t i = 0;
        for (; text[i] && i < maxLength; i++)
            textBuffer[i] = text[i];
        textBuffer[i] = '\0';
    }
    cursorPos = strlen(textBuffer); // Курсор в конец
    scrollOffset = 0;
    adjustScrollOffset();
    draw();
    restoreTopLayer();
}

void Edit::setMaxLength(uint8_t value)
{
    if (value == 0)
        value = 1;
    if (value > MAX_TEXT_LEN)
        value = MAX_TEXT_LEN;
    maxLength = value;
    if (strlen(textBuffer) > maxLength)
        textBuffer[maxLength] = '\0';
    cursorPos = min(cursorPos, (uint8_t)strlen(textBuffer));
    adjustScrollOffset();
    draw();
    restoreTopLayer();
}

void Edit::setFontIndex(uint8_t index)
{
    fontIndex = index;
    adjustScrollOffset();
    draw();
    restoreTopLayer();
}

uint8_t Edit::getMaxVisibleChars() const
{
    if (ownerForm == nullptr)
        return 0;
    
    const Font *font = &lcd.fonts[fontIndex];
    uint8_t charWidth = font->width;
    uint8_t availableWidth = width - 4; // 2 пикселя слева + 2 справа для рамки
    uint8_t maxChars = availableWidth / (charWidth + 1); // +1 для межсимвольного расстояния
    
    return maxChars;
}

void Edit::adjustScrollOffset()
{
    uint8_t maxVisible = getMaxVisibleChars();
    if (maxVisible == 0)
        return;
    
    // Если курсор ушёл правее видимой области, прокручиваем вправо
    if (cursorPos >= scrollOffset + maxVisible)
    {
        scrollOffset = cursorPos - maxVisible + 1;
    }
    
    // Если курсор ушёл левее видимой области, прокручиваем влево
    if (cursorPos < scrollOffset)
    {
        scrollOffset = cursorPos;
    }
    
    // Убеждаемся, что scrollOffset не слишком большой
    uint8_t textLen = strlen(textBuffer);
    if (scrollOffset > textLen)
        scrollOffset = (textLen > 0) ? textLen : 0;
}

void Edit::updateCursorVisibility(TickType_t nowTicks)
{
    if (!isFocused())
    {
        setFlag(FLAG_CURSOR_VISIBLE, false);
        return;
    }

    TickType_t interval = (flags & FLAG_CURSOR_VISIBLE) ? cursorOffDurationTicks : cursorOnDurationTicks;
    if (nowTicks - lastBlinkTime >= interval)
    {
        flags ^= FLAG_CURSOR_VISIBLE;
        lastBlinkTime = nowTicks;
    }
}

void Edit::drawCursor()
{
    if (!isFocused() || !isVisible() || ownerForm == nullptr)
        return;

    const Font *font = &lcd.fonts[fontIndex];
    uint8_t charWidth = font->width;
    uint8_t charHeight = font->height;
    uint8_t textTop = top + 1;
    uint8_t textLeft = left + 2;
    
    // Вычисляем позицию курсора относительно видимой области (с учетом scrollOffset)
    uint8_t visibleCursorPos = cursorPos - scrollOffset;
    uint8_t cursorX = textLeft + visibleCursorPos * (charWidth + 1);
    uint8_t cursorY = textTop;

    // Очищаем область курсора
    ownerForm->drawRect(cursorX - 1, cursorY, cursorX + charWidth, cursorY + charHeight, modeClear, true);
    
    // Перерисовываем символ под курсором (если есть)
    uint8_t textLen = strlen(textBuffer);
    if (cursorPos < textLen)
    {
        char cursorChar[2] = {textBuffer[cursorPos], '\0'};
        ownerForm->setPrintRect(cursorX, cursorY, cursorX + charWidth, cursorY + charHeight);
        ownerForm->print(cursorChar, 1, fontIndex, modeSet);
    }
    
    // Инвертируем область курсора если он должен быть видим
    if (flags & FLAG_CURSOR_VISIBLE)
    {
        ownerForm->drawRect(cursorX - 1, cursorY, cursorX + charWidth, cursorY + charHeight, modeInvert, true);
    }
}

void Edit::redrawContent()
{
    if (!isVisible() || ownerForm == nullptr)
        return;

    ownerForm->drawRect(left, top, left + width, top + height, modeClear, true);
    ownerForm->drawRect(left, top, left + width, top + height, modeSet, false);

    uint8_t textLeft = left + 2;
    uint8_t textTop = top + 1;
    uint8_t textRight = left + width - 2;
    uint8_t textBottom = top + height - 1;
    ownerForm->setPrintRect(textLeft, textTop, textRight, textBottom);
    
    // Отображаем только видимую часть текста, начиная со scrollOffset
    uint8_t textLen = strlen(textBuffer);
    if (scrollOffset < textLen)
    {
        ownerForm->print(textBuffer + scrollOffset, textLen - scrollOffset, fontIndex, modeSet);
    }
}

bool Edit::insertChar(char ch)
{
    uint8_t len = strlen(textBuffer);
    if (!canInsertCharAt(cursorPos, ch))
        return false;

    if (cursorPos > len)
        cursorPos = len;

    if (overwritesCharAt(cursorPos) && cursorPos < len)
    {
        textBuffer[cursorPos] = ch;
        uint8_t nextPos = getNextCursorPosAfterInsert(cursorPos);
        if (nextPos <= len)
            cursorPos = nextPos;

        adjustScrollOffset();
        redrawContent();
        drawCursor();
        restoreTopLayer();
        return true;
    }

    if (len >= maxLength)
        return false;

    for (uint8_t i = len; i > cursorPos; i--)
        textBuffer[i] = textBuffer[i - 1];

    textBuffer[cursorPos] = ch;
    textBuffer[len + 1] = '\0';
    cursorPos++;

    uint8_t nextPos = getNextCursorPosAfterInsert(cursorPos - 1);
    if (nextPos <= strlen(textBuffer))
        cursorPos = nextPos;

    adjustScrollOffset();
    redrawContent();
    drawCursor();
    restoreTopLayer();
    return true;
}

void Edit::deleteCharAtCursor(bool backspace)
{
    uint8_t len = strlen(textBuffer);
    if (len == 0)
        return;

    if (backspace)
    {
        if (cursorPos == 0)
            return;
        for (uint8_t i = cursorPos - 1; i < len; i++)
            textBuffer[i] = textBuffer[i + 1];
        cursorPos--;
    }
    else
    {
        if (cursorPos >= len)
            return;
        for (uint8_t i = cursorPos; i < len; i++)
            textBuffer[i] = textBuffer[i + 1];
    }

    adjustScrollOffset();
    redrawContent();
    drawCursor();
    restoreTopLayer();
}

void Edit::moveCursor(int8_t delta)
{
    cursorPos = getCursorPosAfterMove(cursorPos, delta);
    adjustScrollOffset();
    redrawContent();
    drawCursor();
    restoreTopLayer();
}

void Edit::draw()
{
    if (!isVisible())
        return;

    redrawContent();
    drawCursor();
}

void Edit::update(TickType_t nowTicks)
{
    if (!isVisible() || !isFocused())
        return;

    bool prevVisible = flags & FLAG_CURSOR_VISIBLE;
    updateCursorVisibility(nowTicks);
    if (prevVisible != (flags & FLAG_CURSOR_VISIBLE))
    {
        // Для мигания курсора достаточно просто перерисовать курсор
        // Полная перерисовка контента не нужна и замедляет другие компоненты
        drawCursor();
        restoreTopLayer();
    }
}

bool Edit::onKey(uint8_t key)
{
    if (!isVisible() || !isFocused())
        return false;

    if (key == keyBackspace)
    {
        deleteCharAtCursor(true);
        return true;
    }

    if (key == keyDel)
    {
        deleteCharAtCursor(false);
        return true;
    }

    if (key == keyLeft)
    {
        moveCursor(-1);
        return true;
    }

    if (key == keyRight)
    {
        moveCursor(1);
        return true;
    }

    if (key == keyEnter)
    {
        triggerEvent(EVENT_CLICK);
        return true;
    }

    if (key >= 32 && key <= 126)
    {
        insertChar((char)key);
        return true;
    }

    return false;
}

bool Edit::onKey(const KBState &kb)
{
    if (!isVisible() || !isFocused())
        return false;

    if (kb.ctrl())
        return false;

    if (kb.pressedKey == keyBackspace || kb.pressedChar == '\b')
    {
        deleteCharAtCursor(true);
        return true;
    }

    if (kb.pressedKey == keyDel)
    {
        deleteCharAtCursor(false);
        return true;
    }

    if (kb.pressedKey == keyLeft)
    {
        moveCursor(-1);
        return true;
    }

    if (kb.pressedKey == keyRight)
    {
        moveCursor(1);
        return true;
    }

    if (kb.pressedKey == keyEnter || kb.pressedChar == '\n')
    {
        triggerEvent(EVENT_CLICK);
        return true;
    }

    if (kb.pressedChar >= 32 && kb.pressedChar <= 126)
    {
        insertChar(kb.pressedChar);
        return true;
    }

    return false;
}

void Edit::onFocusChanged()
{
    if (isFocused())
    {
        setFlag(FLAG_CURSOR_VISIBLE, true);
        lastBlinkTime = xTaskGetTickCount();
    }
    else
    {
        setFlag(FLAG_CURSOR_VISIBLE, false);
    }
    draw();
    restoreTopLayer();
}
