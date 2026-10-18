// Form.cpp
#include "Form.h"
#include "UI.h"
#include "elements/All.h"
#include "../Common.h"
#include "../Display.h"
#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

Form::Form()
    : elements(nullptr), elementCount(0), elementCapacity(0), focusedIndex(0), flags(0), ownerTask(NULL)
{
}

Form::~Form()
{
    if (elements)
    {
        for (uint8_t i = 0; i < elementCount; i++)
        {
            delete elements[i];
        }
        vPortFree(elements);
        elements = nullptr;
    }
}

void Form::ensureCapacity()
{
    if (elementCount < elementCapacity)
        return;

    uint8_t newCapacity = (elementCapacity == 0) ? 4 : elementCapacity * 2;
    UIElement **newElements = (UIElement **)pvPortMalloc(newCapacity * sizeof(UIElement *));

    if (!newElements)
        return;

    if (elements)
    {
        memcpy(newElements, elements, elementCount * sizeof(UIElement *));
        vPortFree(elements);
    }

    elements = newElements;
    elementCapacity = newCapacity;

    for (uint8_t i = elementCount; i < elementCapacity; i++)
    {
        elements[i] = nullptr;
    }
}

UIElement *Form::addElement(UIElement *el)
{
    if (!el)
        return nullptr;

    ensureCapacity();
    if (elementCount >= elementCapacity)
    {
        delete el;
        return nullptr;
    }

    el->setOwnerForm(this);
    elements[elementCount++] = el;

    if (elementCount == 1)
    {
        focusedIndex = 0;
    }

    UIElement *focused = getFocused();
    if (el->isVisible() && el->isFocusable() &&
        (elementCount == 1 || !focused || !focused->isVisible() || !focused->isFocusable()))
    {
        setFocusedElement(el);
    }

    return el;
}

bool Form::removeElement(UIElement *el)
{
    if (!el || elementCount == 0)
        return false;

    for (uint8_t i = 0; i < elementCount; i++)
    {
        if (elements[i] == el)
        {
            delete elements[i];
            elements[i] = nullptr;

            for (uint8_t j = i; j + 1 < elementCount; j++)
            {
                elements[j] = elements[j + 1];
            }
            elementCount--;

            if (elementCount == 0)
            {
                focusedIndex = 0;
                return true;
            }

            if (focusedIndex >= elementCount)
            {
                focusedIndex = elementCount - 1;
            }

            for (uint8_t k = 0; k < elementCount; k++)
            {
                if (elements[k])
                    elements[k]->setFocus(k == focusedIndex);
            }

            return true;
        }
    }

    return false;
}

void Form::show()
{
    flags |= FORM_FLAG_VISIBLE;
    draw();
}

void Form::hide()
{
    flags &= ~FORM_FLAG_VISIBLE;
    if (elementCount > 0)
    {
        for (uint8_t i = 0; i < elementCount; i++)
        {
            if (elements[i])
                elements[i]->setFocus(false);
        }
    }
}

void Form::close()
{
    ui.closeForm(this);
}

void Form::requestCloseOwnerTask()
{
    ui.requestCloseOwnerTask(ownerTask);
}

void Form::draw()
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    uint8_t left = formLeft();
    uint8_t top = formTop();
    uint8_t right = formRight();
    uint8_t bottom = formBottom();

    lcd.drawRect(left, top, right, bottom, modeClear, true);

    if (isDrawBorder())
    {
        // Рисуем рамку
        lcd.drawRect(left, top, right, bottom, modeSet, false);
    }

    if (isShowCaption())
    {
        // Если есть заголовок, то выводим его по центру
        uint8_t captionBottom = top + 8;
        if (captionBottom > bottom)
            captionBottom = bottom;

        lcd.drawRect(left, top, right, captionBottom, modeSet, true);
        if (!caption.isEmpty())
        {
            uint8_t captionX, captionY;
            calculateTextPosition(caption, left, top, formWidth(), captionBottom - top + 1, 1, 1, captionX, captionY);
            lcd.setPrintRect(captionX, top, right, captionBottom, true);
            print(caption, modeClear);
        }
    }

    for (uint8_t i = 0; i < elementCount; i++)
    {
        if (elements[i] && elements[i]->isVisible() && !elements[i]->isOnTop())
        {
            elements[i]->draw();
        }
    }

    redrawTopLayer();
}

void Form::redrawTopLayer()
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    for (uint8_t i = 0; i < elementCount; i++)
    {
        if (elements[i] && elements[i]->isVisible() && elements[i]->isOnTop())
            elements[i]->draw();
    }
}

uint8_t Form::formLeft() const
{
    return 0;
}

uint8_t Form::formTop() const
{
    return 0;
}

uint8_t Form::formWidth() const
{
    return Display::width();
}

uint8_t Form::formHeight() const
{
    return Display::height();
}

uint8_t Form::formRight() const
{
    return formLeft() + formWidth() - 1;
}

uint8_t Form::formBottom() const
{
    return formTop() + formHeight() - 1;
}

uint8_t Form::clientLeft() const
{
    return formLeft() + (isDrawBorder() ? 1 : 0);
}

uint8_t Form::clientTop() const
{
    return formTop() + (isShowCaption() ? 9 : (isDrawBorder() ? 1 : 0));
}

uint8_t Form::clientRight() const
{
    return formRight() - (isDrawBorder() ? 1 : 0);
}

uint8_t Form::clientBottom() const
{
    return formBottom() - (isDrawBorder() ? 1 : 0);
}

uint8_t Form::clientWidth() const
{
    return clientRight() - clientLeft() + 1;
}

uint8_t Form::clientHeight() const
{
    return clientBottom() - clientTop() + 1;
}

void Form::drawRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode drawMode, bool infill)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    uint8_t cw = clientWidth();
    uint8_t ch = clientHeight();

    if (x2 < x1 || y2 < y1 || x1 >= cw || y1 >= ch)
        return;

    if (x2 >= cw)
        x2 = cw - 1;
    if (y2 >= ch)
        y2 = ch - 1;

    lcd.drawRect(clientLeft() + x1, clientTop() + y1,
                 clientLeft() + x2, clientTop() + y2,
                 drawMode, infill);
}

void Form::drawVLine(uint8_t x, uint8_t y, uint8_t len, DrawMode drawMode, bool doted)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    uint8_t cw = clientWidth();
    uint8_t ch = clientHeight();

    if (len == 0 || x >= cw || y >= ch)
        return;

    uint8_t maxLen = ch - y;
    if (len > maxLen)
        len = maxLen;

    lcd.drawVLine(clientLeft() + x, clientTop() + y, len, drawMode, doted);
}

void Form::drawHLine(uint8_t x, uint8_t y, uint8_t len, DrawMode drawMode, bool doted)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    uint8_t cw = clientWidth();
    uint8_t ch = clientHeight();

    if (len == 0 || x >= cw || y >= ch)
        return;

    uint8_t maxLen = cw - x;
    if (len > maxLen)
        len = maxLen;

    lcd.drawHLine(clientLeft() + x, clientTop() + y, len, drawMode, doted);
}

void Form::drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.drawLine(clientLeft() + x1, clientTop() + y1,
                 clientLeft() + x2, clientTop() + y2,
                 drawMode);
}

void Form::drawCircle(uint8_t x, uint8_t y, uint8_t r, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.drawCircle(clientLeft() + x, clientTop() + y, r, drawMode);
}

void Form::drawEllipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.drawEllipse(clientLeft() + x, clientTop() + y, a, b, drawMode);
}

void Form::drawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.drawTriangle(clientLeft() + x1, clientTop() + y1,
                     clientLeft() + x2, clientTop() + y2,
                     clientLeft() + x3, clientTop() + y3,
                     drawMode);
}

void Form::drawXBM(const uint8_t *data, uint8_t width, uint8_t height, uint8_t left, uint8_t top, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    if (data == nullptr || width == 0 || height == 0 || left >= clientWidth() || top >= clientHeight())
        return;

    lcd.drawXBM(data, width, height, clientLeft() + left, clientTop() + top, drawMode);
}

void Form::setPrintPos(uint8_t x, uint8_t y)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.setPrintPos(clientLeft() + x, clientTop() + y);
}

void Form::setPrintRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool resetPrintPos)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    uint8_t cw = clientWidth();
    uint8_t ch = clientHeight();

    if (x2 < x1 || y2 < y1 || x1 >= cw || y1 >= ch)
        return;

    if (x2 >= cw)
        x2 = cw - 1;
    if (y2 >= ch)
        y2 = ch - 1;

    lcd.setPrintRect(clientLeft() + x1, clientTop() + y1,
                     clientLeft() + x2, clientTop() + y2,
                     resetPrintPos);
}

void Form::print(const char *str, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.print(str, drawMode);
}

void Form::print(const char *str, uint8_t len, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.print(str, len, drawMode);
}

void Form::print(const char *str, uint8_t len, uint8_t fontIndex, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.print(str, len, fontIndex, drawMode);
}

void Form::print(const __FlashStringHelper *str, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.print(str, drawMode);
}

void Form::print(const __FlashStringHelper *str, uint8_t fontIndex, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.print(str, fontIndex, drawMode);
}

void Form::print(TextRef text, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    if (text.isFlash())
        lcd.print(text.flashString(), drawMode);
    else
        lcd.print(text.c_str(), drawMode);
}

void Form::print(uint8_t value, DrawMode drawMode)
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.print(value, drawMode);
}

void Form::resetPrintRect()
{
    if (!ui.isFormDrawingAllowed(this))
        return;

    lcd.resetPrintRect();
}

void Form::update(TickType_t nowTicks)
{
    if (!isVisible())
        return;

    for (uint8_t i = 0; i < elementCount; i++)
    {
        if (elements[i] && elements[i]->isVisible())
        {
            elements[i]->update(nowTicks);
        }
    }
}

bool Form::onKey(uint8_t key)
{
    if (!isVisible())
        return false;

    for (int8_t i = elementCount - 1; i >= 0; i--)
    {
        if (elements[i] && elements[i]->isVisible() && elements[i]->isOnTop())
        {
            if (elements[i]->onKey(key))
                return true;
        }
    }

    if (key == keyTab)
    {
        focusNext();
        return true;
    }
    else
    {
        UIElement *f = getFocused();
        if (f)
        {
            if (f->onKey(key))
                return true;
            return f->triggerEvent(EVENT_KEY, key);
        }
    }
    return false;
}

bool Form::onKey(const KBState &kb)
{
    if (!isVisible())
        return false;

    for (int8_t i = elementCount - 1; i >= 0; i--)
    {
        if (elements[i] && elements[i]->isVisible() && elements[i]->isOnTop())
        {
            if (elements[i]->onKey(kb))
                return true;
        }
    }

    if (kb.pressedKey == keyTab)
    {
        focusNext();
        return true;
    }

    UIElement *f = getFocused();
    if (f)
    {
        if (f->onKey(kb))
            return true;
        return f->triggerEvent(EVENT_KEY, kb.pressedKey);
    }

    return false;
}

void Form::focusNext()
{
    if (elementCount == 0)
        return;
    uint8_t prev = focusedIndex;
    for (uint8_t step = 1; step <= elementCount; step++)
    {
        uint8_t idx = (focusedIndex + step) % elementCount;
        if (elements[idx] && elements[idx]->isVisible() && !elements[idx]->isOnTop() && elements[idx]->isFocusable())
        {
            focusedIndex = idx;
            break;
        }
    }

    if (prev != focusedIndex)
    {
        if (elements[prev])
            elements[prev]->setFocus(false);
        if (elements[focusedIndex])
            elements[focusedIndex]->setFocus(true);
    }
}

void Form::focusPrev()
{
    if (elementCount == 0)
        return;
    uint8_t prev = focusedIndex;
    for (uint8_t step = 1; step <= elementCount; step++)
    {
        uint8_t idx = (focusedIndex + elementCount - step) % elementCount;
        if (elements[idx] && elements[idx]->isVisible() && !elements[idx]->isOnTop() && elements[idx]->isFocusable())
        {
            focusedIndex = idx;
            break;
        }
    }

    if (prev != focusedIndex)
    {
        if (elements[prev])
            elements[prev]->setFocus(false);
        if (elements[focusedIndex])
            elements[focusedIndex]->setFocus(true);
    }
}

bool Form::setFocusedElement(UIElement *element)
{
    if (!element || !element->isVisible() || !element->isFocusable() || element->isOnTop())
        return false;

    for (uint8_t i = 0; i < elementCount; i++)
    {
        if (elements[i] != element)
            continue;

        if (focusedIndex == i && element->isFocused())
            return true;

        if (focusedIndex < elementCount && elements[focusedIndex])
            elements[focusedIndex]->setFocus(false);
        focusedIndex = i;
        element->setFocus(true);
        return true;
    }

    return false;
}

UIElement *Form::getFocused() const
{
    if (elementCount == 0 || focusedIndex >= elementCount)
        return nullptr;
    return elements[focusedIndex];
}

void Form::setVisible(bool v)
{
    if (v)
        flags |= FORM_FLAG_VISIBLE;
    else
        flags &= ~FORM_FLAG_VISIBLE;
}

bool Form::isVisible() const
{
    return flags & FORM_FLAG_VISIBLE;
}

void Form::setCaption(const char *text)
{
    caption = TextRef(text);
    setShowCaption(true);
}

void Form::setCaption(const __FlashStringHelper *text)
{
    caption = TextRef(text);
    setShowCaption(true);
}

void Form::setOwnerTask(TaskHandle_t task)
{
    ownerTask = task;
}

void Form::setDrawBorder(bool value)
{
    bool needRedraw = isDrawBorder() != value;
    if (value)
        flags |= FORM_FLAG_DRAW_BORDER;
    else
        flags &= ~FORM_FLAG_DRAW_BORDER;
    if (needRedraw)
        draw();
}

void Form::setShowCaption(bool value)
{
    bool needRedraw = isShowCaption() != value;
    if (value)
        flags |= FORM_FLAG_SHOW_CAPTION;
    else
        flags &= ~FORM_FLAG_SHOW_CAPTION;
    if (needRedraw)
        draw();
}

Button *Form::addButton(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char *caption)
{
    Button *btn = new Button(left, top, width, height, caption);
    return static_cast<Button *>(addElement(btn));
}

Button *Form::addButton(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const __FlashStringHelper *caption)
{
    Button *btn = new Button(left, top, width, height, caption);
    return static_cast<Button *>(addElement(btn));
}

IconButton *Form::addIconButton(uint8_t left, uint8_t top, const uint8_t *icon, uint8_t iconSize)
{
    IconButton *btn = new IconButton(left, top, icon, iconSize);
    return static_cast<IconButton *>(addElement(btn));
}

IconButton *Form::addIconButton(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
                                const uint8_t *icon, uint8_t iconWidth, uint8_t iconHeight)
{
    IconButton *btn = new IconButton(left, top, width, height, icon, iconWidth, iconHeight);
    return static_cast<IconButton *>(addElement(btn));
}

CheckBox *Form::addCheckBox(uint8_t left, uint8_t top, bool checked, const char *caption)
{
    CheckBox *chk = new CheckBox(left, top, checked, caption);
    return static_cast<CheckBox *>(addElement(chk));
}

CheckBox *Form::addCheckBox(uint8_t left, uint8_t top, bool checked, const __FlashStringHelper *caption)
{
    CheckBox *chk = new CheckBox(left, top, checked, caption);
    return static_cast<CheckBox *>(addElement(chk));
}

ProgressBar *Form::addProgressBar(uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t minValue, uint8_t maxValue)
{
    ProgressBar *bar = new ProgressBar(left, top, width, height, minValue, maxValue);
    return static_cast<ProgressBar *>(addElement(bar));
}

Label *Form::addLabel(const char *text, uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t hAlign, uint8_t vAlign)
{
    Label *lbl = new Label(text, left, top, width, height, hAlign, vAlign);
    return static_cast<Label *>(addElement(lbl));
}

Label *Form::addLabel(const __FlashStringHelper *text, uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t hAlign, uint8_t vAlign)
{
    Label *lbl = new Label(text, left, top, width, height, hAlign, vAlign);
    return static_cast<Label *>(addElement(lbl));
}

Label *Form::addLabel(TextRef text, uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t hAlign, uint8_t vAlign)
{
    Label *lbl = new Label(text, left, top, width, height, hAlign, vAlign);
    return static_cast<Label *>(addElement(lbl));
}

TextBox *Form::addTextBox(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char *text, uint8_t maxLength)
{
    TextBox *box = new TextBox(left, top, width, height, text, maxLength);
    return static_cast<TextBox *>(addElement(box));
}

DateEdit *Form::addDateEdit(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const RTCDate &value)
{
    DateEdit *box = new DateEdit(left, top, width, height, value);
    return static_cast<DateEdit *>(addElement(box));
}

TimeEdit *Form::addTimeEdit(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const RTCTime &value)
{
    TimeEdit *box = new TimeEdit(left, top, width, height, value);
    return static_cast<TimeEdit *>(addElement(box));
}

/*
Memo *Form::addMemo(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
{
    Memo *memo = new Memo(left, top, width, height);
    return static_cast<Memo *>(addElement(memo));
}
*/
List *Form::addList(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
{
    List *list = new List(left, top, width, height);
    return static_cast<List *>(addElement(list));
}

ArrayList *Form::addArrayList(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
{
    ArrayList *list = new ArrayList(left, top, width, height);
    return static_cast<ArrayList *>(addElement(list));
}

Menu *Form::addMenu(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
{
    Menu *menu = new Menu(left, top, width, height);
    return static_cast<Menu *>(addElement(menu));
}

FileList *Form::addFileList(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
{
    FileList *list = new FileList(left, top, width, height);
    return static_cast<FileList *>(addElement(list));
}
