#include "FormEx.h"

FormEx::FormEx(uint8_t left, uint8_t top, uint8_t width, uint8_t height)
    : left(left), top(top), width(width), height(height)
{
    if (this->left >= Display::width())
        this->left = Display::width() - 1;
    if (this->top >= Display::height())
        this->top = Display::height() - 1;

    uint8_t maxWidth = Display::width() - this->left;
    uint8_t maxHeight = Display::height() - this->top;

    if (this->width == 0 || this->width > maxWidth)
        this->width = maxWidth;
    if (this->height == 0 || this->height > maxHeight)
        this->height = maxHeight;
}

uint8_t FormEx::formLeft() const
{
    return left;
}

uint8_t FormEx::formTop() const
{
    return top;
}

uint8_t FormEx::formWidth() const
{
    return width;
}

uint8_t FormEx::formHeight() const
{
    return height;
}
