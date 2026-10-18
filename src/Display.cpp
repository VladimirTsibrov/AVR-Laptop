#include "Arduino_FreeRTOS.h"
#include "Display.h"
#include "Arduino.h"
#include <inttypes.h>
#include "Sdcard.h"
#include "Debug.h"
#include "Common.h"
#include "fonts/font5x8.h"

Display lcd;
uint8_t videoBuffer[Display::width() * Display::height() / 8];
volatile uint8_t *frameBuffer = videoBuffer;
volatile uint8_t *isrDataPointer __attribute__((used)) = nullptr;
volatile uint8_t bytesToSend __attribute__((used));

#define SET_RW_RS(RW, RS) (PORTG = (PORTG & ~0x03) | (((RW) & 1) << 1) | (((RS) & 1) << 0))
#define CLR_E() (PORTG &= ~0x04)
#define SET_E() (PORTG |= 0x04)

uint8_t Display::lcdRead(uint8_t lcdMode)
{
    DDRK = 0x00;

    if (lcdMode)
        SET_RW_RS(1, 1);
    else
        SET_RW_RS(1, 0);

    asm volatile("nop");
    SET_E();
    asm volatile("nop\n nop\n nop");
    uint8_t value = PINK;
    CLR_E();
    return value;
}

void Display::lcdWrite(uint8_t lcdMode, uint8_t value)
{
    while (lcdRead(0) >= 0x80)
        ;

    DDRK = 0xFF; // Порт на выход

    PORTK = value;
    if (lcdMode)
        SET_RW_RS(0, 1);
    else
        SET_RW_RS(0, 0);

    asm volatile("nop");
    SET_E();
    asm volatile("nop\n nop\n nop");
    CLR_E();
}

Display::Display()
    : backlightLevel(backlightOff)
{
    memset(dirtyLines, 0, sizeof(dirtyLines));
    fonts = nullptr;
    fontCount = 0;
    isUpdating = false;
    repaintY = 0;

    for (uint8_t i = 0; i < MAX_OVERLAYS; i++)
    {
        overlays[i].active = false;
        overlays[i].buf = nullptr;
    }

    addFont("5*8", 5, 8, font5x8);
    setPrintRect(0, 0, width() - 1, height() - 1);
    cls();
}

void Display::setBacklight(bool value)
{
    setBacklightLevel(value ? backlightMax : backlightOff);
}

void Display::setBacklightLevel(BacklightLevel level)
{
    if (level > backlightMax)
        level = backlightMax;

    backlightLevel = level;
    OCR3A = (level == backlightOff) ? 0 : (0xFF >> (backlightMax - level));
}

void Display::increaseBacklight()
{
    if (backlightLevel < backlightMax)
        setBacklightLevel((BacklightLevel)(backlightLevel + 1));
}

void Display::decreaseBacklight()
{
    if (backlightLevel > backlightOff)
        setBacklightLevel((BacklightLevel)(backlightLevel - 1));
}

uint8_t *Display::getFrameBuffer()
{
    return videoBuffer;
}

void Display::setPrintPos(uint8_t x, uint8_t y)
{
    if ((x & ~0x7F) || (y & ~0x3F))
        return;
    printXPos = x;
    printYPos = y;
}

bool Display::drawAllowed()
{
    // Рисовать может только активная задача. Оболочка (Shell) — привилегированный
    // владелец дисплея: рисует системные оверлеи и перерисовывает UI активной
    // задачи из своего контекста. До старта планировщика (cur == NULL при
    // инициализации) рисование разрешено.
    TaskHandle_t cur = xTaskGetCurrentTaskHandle();
    return cur == activeTask || cur == shellTask;
}

void Display::setByteByPtr(uint8_t *p, uint8_t value, DrawMode drawMode = modeSet)
{
    if (!drawAllowed())
        return;
    if (drawMode == modeSet)
        *p = value;
    else if (drawMode == modeClear)
        *p &= ~value;
    else if (drawMode == modeInvert)
        *p ^= value;
    uint16_t offset = p - videoBuffer;
    uint8_t row = offset / 16;
    setDirtyLine(row);
}

void Display::setByteByXY(uint8_t x, uint8_t y, uint8_t value, DrawMode drawMode)
{
    if (!drawAllowed())
        return;
    if ((x & ~0x0F) || (y & ~0x3F))
        return;
    uint16_t i = (y << 4) | x;
    if (drawMode == modeSet)
        frameBuffer[i] |= value;
    else if (drawMode == modeClear)
        frameBuffer[i] &= ~value;
    else if (drawMode == modeInvert)
        frameBuffer[i] ^= value;
    setDirtyLine(y);
}

void Display::checkBuf(void *buf)
{
    if (!buf)
    {
        lcd.cls();
        // lcd.print("Out of memory");
        lcd.repaint();
        while (1)
            ; // Зависаем
    }
}

// Public methods
void Display::begin()
{
    DDRA |= 0x80;
    DDRG |= 0x07;
    PORTA &= ~(0x80); // сбросить RST (7 бит PORTA)
    vTaskDelay(pdMS_TO_TICKS(1));
    PORTA |= 0x80; // Установить RST
    vTaskDelay(pdMS_TO_TICKS(40));
    PORTG &= ~(0x03);
    lcdWrite(0, cmdFunctionSet | bitDL); // DL = 1: 8 bit mode
    vTaskDelay(pdMS_TO_TICKS(10));
    lcdWrite(0, cmdFunctionSet | bitDL); // DL = 1: 8 bit mode
    vTaskDelay(pdMS_TO_TICKS(10));
    lcdWrite(0, cmdDisplay | bitDisplay); // Display on
    vTaskDelay(pdMS_TO_TICKS(10));
    lcdWrite(0, cmdFunctionSet | bitDL | bitRE); // RE = 1: +extended mode
    vTaskDelay(pdMS_TO_TICKS(10));
    lcdWrite(0, cmdFunctionSet | bitDL | bitRE | bitG); // G = 1: +graphic mode
    vTaskDelay(pdMS_TO_TICKS(1));

    DDRE |= (1 << PE3); // D5 / OC3A: PWM управления подсветкой
    TCCR3A = (1 << COM3A1) | (1 << WGM30);
    TCCR3B = (1 << WGM32) | (1 << CS31);
    setBacklightLevel(backlightMedium);
    repaintY = 0;
}

void Display::cls()
{
    if (!drawAllowed())
        return;
    SchedulerSuspender lock;
    memset(videoBuffer, 0, 1024);
    printXPos = 0;
    printYPos = fonts[0].height - 1;
    setAllDirtyLines();
}

void Display::repaint()
{
    if (!drawAllowed())
        return;
    SchedulerSuspender lock;
    setAllDirtyLines();
}
void Display::beginUpdate()
{
    isUpdating = true;
}
void Display::endUpdate()
{
    isUpdating = false;
}

/*
void Display::repaintOld()
{
    uint8_t *p1 = videoBuffer;
    uint8_t *p2 = &videoBuffer[512];
    for (uint8_t y = 0; y < 32; y++)
    {
        lcdWrite(0, cmdSetDDRAMAddress + y);
        lcdWrite(0, cmdSetDDRAMAddress);
        for (uint8_t x = 0; x < 32; x++)
        {
            if (x & ~0x0F)
                lcdWrite(1, *p2++);
            else
                lcdWrite(1, *p1++);
        }
    }
}
*/

bool Display::getPixel(uint8_t x, uint8_t y)
{
    if ((x & ~0x7F) || (y & ~0x3F))
        return false;
    uint16_t i = (y << 4) | (x >> 3);
    uint8_t bitNum = (x ^ 0x07) & 0x07;
    return (frameBuffer[i] & (1 << bitNum));
}

void Display::markDirtyLine(uint8_t y)
{
    if (!drawAllowed() || (y & ~0x3F))
        return;
    setDirtyLine(y);
}

void Display::setPixel(uint8_t x, uint8_t y, DrawMode drawMode)
{
    if (!drawAllowed())
        return;
    if ((x & ~0x7F) || (y & ~0x3F))
        return;
    uint16_t i = (y << 4) | (x >> 3);
    uint8_t bitNum = (x ^ 0x07) & 0x07;

    if (drawMode == modeSet)
        frameBuffer[i] |= 1 << bitNum;
    else if (drawMode == modeClear)
        frameBuffer[i] &= ~(1 << bitNum);
    else if (drawMode == modeInvert)
        frameBuffer[i] ^= 1 << bitNum;
    setDirtyLine(y);
}

void Display::setByte(uint8_t x, uint8_t y, uint8_t value, DrawMode drawMode)
{
    if (!drawAllowed())
        return;
    // x 0..16, y 0..64
    if ((x & ~0x0F) || (y & ~0x3F))
        return;
    uint16_t i = (y << 4) | x;
    if (drawMode == modeSet)
        frameBuffer[i] |= value;
    else if (drawMode == modeClear)
        frameBuffer[i] &= ~value;
    else if (drawMode == modeInvert)
        frameBuffer[i] ^= value;
    setDirtyLine(y);
}

void Display::drawHLine(uint8_t x, uint8_t y, uint8_t len, DrawMode drawMode, bool doted)
{
    if (!drawAllowed())
        return;
    SchedulerSuspender lock;
    // x 0..128, y 0..64
    // if ((x & ~0x7F) || (y & ~0x3F))
    //    return;
    uint8_t _x = x;
    uint8_t remaining = len;
    uint8_t infill;
    uint8_t f = y & 0x01;
    if (doted)
        if ((x & 0x01) == (y & 0x01))
            infill = 0x55;
        else
            infill = 0xAA;
    else
        infill = 0xFF;

    while (_x % 8 != 0 && remaining > 0)
    {
        if (!(doted) || ((_x & 0x01) == f))
            setPixel(_x, y, drawMode);
        _x++;
        remaining--;
    }

    while (remaining >= 8)
    {
        setByte(_x >> 3, y, infill, drawMode);
        _x += 8;
        remaining -= 8;
    }

    while (remaining > 0)
    {
        if (!(doted) || ((_x & 0x01) == f))
            setPixel(_x, y, drawMode);
        _x++;
        remaining--;
    }
    setDirtyLine(y);
}

void Display::drawVLine(uint8_t x, uint8_t y, uint8_t len, DrawMode drawMode, bool doted)
{
    if (!drawAllowed())
        return;
    SchedulerSuspender lock;
    // x 0..128, y 0..64
    // if ((x & ~0x7F) || (y & ~0x3F)) // ничего не делаем если x > 127 или y > 63
    //    return;
    uint16_t i;
    uint8_t bitNum = (x ^ 0x07) & 0x07;
    uint8_t f = x & 0x01;
    for (uint8_t row = y; row < y + len; row++)
    {
        if ((doted) && !((row & 0x01) == f))
            continue;
        i = (row << 4) | (x >> 3);
        if (drawMode == modeClear)
            frameBuffer[i] &= ~(1 << bitNum); // Очистить
        else if (drawMode == modeSet)
            frameBuffer[i] |= 1 << bitNum; // Установить
        else if (drawMode == modeInvert)
            frameBuffer[i] ^= 1 << bitNum; // Инвертировать
        setDirtyLine(row);
    }
}

void Display::drawRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode drawMode, bool infill)
{
    SchedulerSuspender lock;
    if ((x1 & ~0x7F) || (y1 & ~0x3F) || (x2 & ~0x7F) || (y2 & ~0x3F))
        return;
    uint8_t minX = (x1 < x2) ? x1 : x2;
    uint8_t maxX = (x1 < x2) ? x2 : x1;
    uint8_t minY = (y1 < y2) ? y1 : y2;
    uint8_t maxY = (y1 < y2) ? y2 : y1;

    if (infill)
    {
        for (uint8_t y = minY; y <= maxY; y++)
            drawHLine(minX, y, maxX - minX + 1, drawMode);
    }
    else
    {
        // Рисуем верхнюю и нижнюю стороны прямоугольника
        drawHLine(minX, minY, maxX - minX + 1, drawMode);
        drawHLine(minX, maxY, maxX - minX + 1, drawMode);

        // Рисуем левые и правые стороны прямоугольника
        drawVLine(minX, minY + 1, maxY - minY - 1, drawMode);
        drawVLine(maxX, minY + 1, maxY - minY - 1, drawMode);
    }
}

void Display::drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode drawMode)
{
    SchedulerSuspender lock;
    int8_t dx = x2 - x1;            // Изменение по x
    int8_t dy = y2 - y1;            // Изменение по y
    uint8_t abs_dx = abs(dx);       // Абсолютное значение изменения по x
    uint8_t abs_dy = abs(dy);       // Абсолютное значение изменения по y
    int8_t sx = (dx >= 0) ? 1 : -1; // Направление по x
    int8_t sy = (dy >= 0) ? 1 : -1; // Направление по y

    // Если линия более горизонтальная
    if (abs_dx > abs_dy)
    {
        int16_t err = abs_dx / 2; // Ошибка
        while (x1 != x2)
        {
            setPixel(x1, y1, drawMode); // Рисуем пиксель
            err -= abs_dy;              // Уменьшаем ошибку
            if (err < 0)
            {
                y1 += sy;      // Изменяем y
                err += abs_dx; // Увеличиваем ошибку
            }
            x1 += sx; // Изменяем x
        }
    }
    else
    {                             // Если линия более вертикальная
        int16_t err = abs_dy / 2; // Ошибка
        while (y1 != y2)
        {
            setPixel(x1, y1, drawMode); // Рисуем пиксель
            err -= abs_dx;              // Уменьшаем ошибку
            if (err < 0)
            {
                x1 += sx;      // Изменяем x
                err += abs_dy; // Увеличиваем ошибку
            }
            y1 += sy; // Изменяем y
        }
    }
}

void Display::drawCircle(uint8_t x, uint8_t y, uint8_t r, DrawMode drawMode)
{
    SchedulerSuspender lock;
    uint8_t dx = r;  // Начальная точка по x
    uint8_t dy = 0;  // Начальная точка по d
    uint8_t err = 0; // Ошибка

    while (dx >= dy)
    {
        // Рисуем пиксели в восьми симметричных позициях
        setPixel(x + dx, y + dy, drawMode);
        setPixel(x + dy, y + dx, drawMode);
        setPixel(x - dy, y + dx, drawMode);
        setPixel(x - dx, y + dy, drawMode);
        setPixel(x - dx, y - dy, drawMode);
        setPixel(x - dy, y - dx, drawMode);
        setPixel(x + dy, y - dx, drawMode);
        setPixel(x + dx, y - dy, drawMode);

        dy++;
        err += (dy + dy) + 1; // Увеличиваем ошибку (2 * dy + 1)
        if (err > dx + dx)
        { // Если ошибка превышает dx
            dx--;
            err -= (dx + dx) + 1; // Уменьшаем ошибку (2 * dx + 1)
        }
    }
}

void Display::drawEllipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b, DrawMode drawMode)
{
    SchedulerSuspender lock;
    int8_t dx = 0;        // Начальная точка по x
    int8_t dy = b;        // Начальная точка по y
    int16_t a2 = a * a;   // a^2
    int16_t b2 = b * b;   // b^2
    int16_t fa2 = 4 * a2; // 4 * a^2
    int16_t p;

    // Первая половина эллипса
    p = round(b2 - (a2 * dy) + (0.25 * a2));
    while (a2 * dy > b2 * dx)
    {
        // Рисуем пиксели в четырех симметричных позициях
        setPixel(x + dx, y + dy, drawMode);
        setPixel(x - dx, y + dy, drawMode);
        setPixel(x + dx, y - dy, drawMode);
        setPixel(x - dx, y - dy, drawMode);

        if (p < 0)
        {
            p += (b2 * (2 * dx + 3));
        }
        else
        {
            p += (b2 * (2 * dx + 3) - (fa2 * (dy - 1)));
            dy--;
        }
        dx++;
    }

    // Вторая половина эллипса
    p = round(b2 * (dx + 0.5) * (dx + 0.5) + a2 * (dy - 1) * (dy - 1) - a2 * b2);
    while (dy >= 0)
    {
        // Рисуем пиксели в четырех симметричных позициях
        setPixel(x + dx, y + dy, drawMode);
        setPixel(x - dx, y + dy, drawMode);
        setPixel(x + dx, y - dy, drawMode);
        setPixel(x - dx, y - dy, drawMode);

        if (p > 0)
        {
            p -= (fa2 * (dy - 1));
        }
        else
        {
            p += (b2 * (2 * dx + 2));
            dx++;
        }
        dy--;
    }
}

void Display::drawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, DrawMode drawMode)
{
    SchedulerSuspender lock;
    drawLine(x1, y1, x2, y2, drawMode);
    drawLine(x2, y2, x3, y3, drawMode);
    drawLine(x3, y3, x1, y1, drawMode);
}


void Display::drawXBM(const uint8_t *data, uint8_t width, uint8_t height, uint8_t left, uint8_t top, DrawMode drawMode)
{
    if (!drawAllowed())
        return;
    SchedulerSuspender lock;
    const uint8_t x1 = left >> 3;
    const uint8_t bytesPerRow = (width + 7) >> 3;
    const uint8_t x2 = x1 + bytesPerRow;
    const uint8_t shiftR = left & 0x07;
    const uint8_t lastByteBits = width % 8; // сколько битов реально используется в последнем байте
    uint8_t shiftL;
    uint8_t maskL;
    uint8_t maskR;
    if (shiftR)
    {
        shiftL = 8 - shiftR;
        if (drawMode == modeSet)
        {
            maskL = (1 << shiftL) - 1;
            maskR = ~maskL;
        }
    }

    for (uint8_t y = top; y < top + height; y++)
    {
        uint16_t i = y * 16 + x1;
        uint8_t *p = (uint8_t *)&frameBuffer[i];
        for (uint8_t x = x1; x < x2; x++)
        {
            uint8_t c = flipBits(pgm_read_byte(data++));

            // обрезаем лишние биты в последнем байте строки
            if (x == x2 - 1 && lastByteBits != 0)
            {
                uint8_t mask = (0xFF << (8 - lastByteBits));
                if (drawMode == modeSet)
                    c = c & mask;
                else
                    c = c & mask; // для других режимов по аналогии
            }

            if (shiftR)
            {
                if (drawMode == modeSet)
                {
                    if (x == x2 - 1 && lastByteBits != 0)
                    {
                        const uint8_t firstBits = (lastByteBits < shiftL) ? lastByteBits : shiftL;
                        const uint8_t firstMask = ((1 << firstBits) - 1) << (shiftL - firstBits);
                        setByteByPtr(p, (*p & ~firstMask) | ((c >> shiftR) & firstMask), drawMode);

                        const uint8_t secondBits = lastByteBits - firstBits;
                        if (secondBits)
                        {
                            const uint8_t secondMask = 0xFF << (8 - secondBits);
                            setByteByPtr(p + 1, (*(p + 1) & ~secondMask) | ((c << shiftL) & secondMask), drawMode);
                        }
                    }
                    else
                        setByteByPtr((uint8_t *)p++, (*p & maskR) | (c >> shiftR), drawMode);
                }
                else
                    setByteByPtr((uint8_t *)p++, c >> shiftR, drawMode);
                if (drawMode == modeSet)
                {
                    if (x != x2 - 1 || lastByteBits == 0)
                        setByteByPtr((uint8_t *)p, (*p & maskL) | (c << shiftL), drawMode);
                }
                else
                    setByteByPtr((uint8_t *)p, c << shiftL, drawMode);
            }
            else
            {
                if (drawMode == modeSet && x == x2 - 1 && lastByteBits != 0)
                {
                    const uint8_t mask = 0xFF << (8 - lastByteBits);
                    setByteByPtr(p, (*p & ~mask) | (c & mask), drawMode);
                }
                else
                    setByteByPtr((uint8_t *)p++, c, drawMode);
            }
        }
        setDirtyLine(y);
    }
}

void Display::drawXBM_RAM(const uint8_t *data, uint8_t width, uint8_t height, uint8_t left, uint8_t top, DrawMode drawMode)
{
    if (!drawAllowed())
        return;
    SchedulerSuspender lock;
    const uint8_t x1 = left >> 3;
    const uint8_t bytesPerRow = (width + 7) >> 3;
    const uint8_t x2 = x1 + bytesPerRow;
    const uint8_t shiftR = left & 0x07;
    const uint8_t lastByteBits = width % 8; // сколько битов реально используется в последнем байте
    uint8_t shiftL;
    uint8_t maskL;
    uint8_t maskR;

    if (shiftR)
    {
        shiftL = 8 - shiftR;
        if (drawMode == modeSet)
        {
            maskL = (1 << shiftL) - 1;
            maskR = ~maskL;
        }
    }

    for (uint8_t y = top; y < top + height; y++)
    {
        uint16_t i = y * 16 + x1;
        uint8_t *p = (uint8_t *)&frameBuffer[i];
        for (uint8_t x = x1; x < x2; x++)
        {
            uint8_t c = flipBits(*data++);

            // обрезаем лишние биты в последнем байте строки
            if (x == x2 - 1 && lastByteBits != 0)
            {
                uint8_t mask = (0xFF << (8 - lastByteBits));
                c = c & mask;
            }

            if (shiftR)
            {
                if (drawMode == modeSet)
                {
                    if (x == x2 - 1 && lastByteBits != 0)
                    {
                        const uint8_t firstBits = (lastByteBits < shiftL) ? lastByteBits : shiftL;
                        const uint8_t firstMask = ((1 << firstBits) - 1) << (shiftL - firstBits);
                        setByteByPtr(p, (*p & ~firstMask) | ((c >> shiftR) & firstMask), drawMode);

                        const uint8_t secondBits = lastByteBits - firstBits;
                        if (secondBits)
                        {
                            const uint8_t secondMask = 0xFF << (8 - secondBits);
                            setByteByPtr(p + 1, (*(p + 1) & ~secondMask) | ((c << shiftL) & secondMask), drawMode);
                        }
                    }
                    else
                        setByteByPtr((uint8_t *)p++, (*p & maskR) | (c >> shiftR), drawMode);
                }
                else
                    setByteByPtr((uint8_t *)p++, c >> shiftR, drawMode);
                if (drawMode == modeSet)
                {
                    if (x != x2 - 1 || lastByteBits == 0)
                        setByteByPtr((uint8_t *)p, (*p & maskL) | (c << shiftL), drawMode);
                }
                else
                    setByteByPtr((uint8_t *)p, c << shiftL, drawMode);
            }
            else
            {
                if (drawMode == modeSet && x == x2 - 1 && lastByteBits != 0)
                {
                    const uint8_t mask = 0xFF << (8 - lastByteBits);
                    setByteByPtr(p, (*p & ~mask) | (c & mask), drawMode);
                }
                else
                    setByteByPtr((uint8_t *)p++, c, drawMode);
            }
        }
        setDirtyLine(y);
    }
}

uint8_t Display::addFont(const char *fontName, uint8_t width, uint8_t height, const uint8_t *data, uint8_t offset)
{
    SchedulerSuspender lock;
    for (uint8_t i = 0; i < fontCount; i++)
    {
        if (strcmp(fontName, fonts[i].fontName) == 0)
        {
            return i;
        }
    }

    Font *newFonts = (Font *)pvPortMalloc((fontCount + 1) * sizeof(Font));
    checkBuf(newFonts);
    if (fonts)
    {
        memcpy(newFonts, fonts, fontCount * sizeof(Font));
        vPortFree(fonts);
    }
    fonts = newFonts;
    fonts[fontCount].fontName = fontName;
    fonts[fontCount].width = width;
    fonts[fontCount].height = height;
    fonts[fontCount].data = data;
    fonts[fontCount].offset = offset;
    return fontCount++;
}

uint8_t Display::getSystemFontIndex() const
{
    return (fontCount > 0) ? 0 : 0xFF;
}

void Display::setByteByPos(uint16_t bufferPos, uint8_t value, DrawMode drawMode, uint8_t mask)
{
    // Вызывается только из print. Не управляет грязными строками
    if (!drawAllowed())
        return;

    if (drawMode == modeSet)
    {
        frameBuffer[bufferPos] &= ~mask;
        frameBuffer[bufferPos] |= value;
    }
    else if (drawMode == modeClear)
    {
        frameBuffer[bufferPos] |= mask;
        frameBuffer[bufferPos] &= ~value;
    }
    else if (drawMode == modeInvert)
        frameBuffer[bufferPos] ^= value;
}

void Display::print(const char *str, DrawMode drawMode)
{
    print(str, strlen(str), getSystemFontIndex(), drawMode);
}

void Display::print(const char *str, uint8_t len, DrawMode drawMode)
{
    print(str, len, getSystemFontIndex(), drawMode);
}

void Display::print(const char *str, uint8_t len, uint8_t fontIndex, DrawMode drawMode)
{
    if (!drawAllowed())
        return;
    SchedulerSuspender lock;
    if (!str || fontIndex >= fontCount)
        return;

    // вывести текст начиная с точки printXPos, printYPos
    uint16_t fontDataOffset; // позиция данных для символа в шрифте
    uint16_t bufferPos;      // позиция в видеобуфере
    uint8_t fontByte;        // байт данных из шрифта
    uint8_t bytesPerWidth = 1;
    uint8_t maskLeft;
    uint8_t maskRight;
    uint8_t h = fonts[fontIndex].height;
    uint8_t w = fonts[fontIndex].width;
    bool needSpace = false;
    const unsigned char *fontData = fonts[fontIndex].data;
    if (w > 8)
        bytesPerWidth++;

    for (uint8_t d = 0; d < h; d++)
    {
        setDirtyLine(printYPos - d);
    }
    for (uint8_t i = 0; i < len; i++)
    { // цикл по символам строки
        fontDataOffset = h;
        fontDataOffset *= (uint8_t(str[i]) - fonts[fontIndex].offset); // buffer++ ???
        if (bytesPerWidth > 1)
            fontDataOffset = (fontDataOffset << 1) + 1;
        // Проверить, умещаемся ли на текущей строке
        if (printXPos + w + needSpace > printRect.x2)
        {
            // Не умещаемся
            if (printYPos + 1 + h <= printRect.y2)
            {
                // Переносим на сл.строку
                printYPos += 1 + h;
                printXPos = printRect.x1;
                needSpace = false;
                for (uint8_t d = 0; d < h; d++)
                {
                    setDirtyLine(printYPos - d);
                }
            }
            else
            {
                // Больше некуда выводить
                break;
            }
        }
        else if (needSpace)
        {
            // Если умещаемся на текущей строке и нужен разделитель между символами
            printXPos += 1;
            if (drawMode == modeSet)
                drawVLine(printXPos - 1, printYPos - h + 1, h, modeClear);
            else if (drawMode == modeClear)
                drawVLine(printXPos - 1, printYPos - h + 1, h, modeSet);
        }

        for (uint8_t fx = 0; fx < bytesPerWidth; fx++)
        {                                              // цикл по кол-ву байт в шрифте (по ширине), не более 2 байт
            uint8_t freeBits = 8 - (printXPos & 0x07); // кол-во доступных битов в байте
            uint16_t fontDataOffset2 = fontDataOffset;
            uint8_t bitsToPrint;
            if (fx)
            { // старший байт
                bitsToPrint = 8;
                fontDataOffset2--;
            }
            else
            { // младший байт
                bitsToPrint = (w > 8) ? (w & 0x07) : (w);
            }
            uint8_t shiftCountLeft;
            uint8_t shiftCountRight;
            if (freeBits >= bitsToPrint)
            { // Места достаточно для вывода символа
                shiftCountLeft = freeBits - bitsToPrint;
                maskLeft = (1 << bitsToPrint) - 1;
                if (shiftCountLeft)
                    maskLeft <<= shiftCountLeft;
            }
            else
            { // Места недостаточно. Вывод в 2 байта
                shiftCountRight = bitsToPrint - freeBits;
                shiftCountLeft = 8 - shiftCountRight;
                maskRight = (1 << bitsToPrint) - 1;
                maskLeft = maskRight;
                // if (shiftCountRight)
                maskRight >>= shiftCountRight;
                maskLeft <<= shiftCountLeft;
            }

            for (uint8_t fy = printYPos - h + 1; fy <= printYPos; fy++)
            {
                bufferPos = 16 * fy + (printXPos >> 3);
                fontByte = pgm_read_byte(fontData + fontDataOffset2);
                if (freeBits < bitsToPrint)
                { // Сдвиг вправо
                    uint8_t fontPart = fontByte >> shiftCountRight;
                    setByteByPos(bufferPos++, fontPart, drawMode, maskRight);
                }
                fontByte = fontByte << shiftCountLeft;
                setByteByPos(bufferPos, fontByte, drawMode, maskLeft);
                fontDataOffset2 += bytesPerWidth;
                setDirtyLine(fy);
            }
            printXPos += bitsToPrint;
        }
        needSpace = true;
    }
}

void Display::print(TextRef text, DrawMode drawMode)
{
    print(text, getSystemFontIndex(), drawMode);
}

void Display::print(TextRef text, uint8_t fontIndex, DrawMode drawMode)
{
    if (!drawAllowed())
        return;
    SchedulerSuspender lock;
    if (text.isNull() || fontIndex >= fontCount)
        return;

    uint16_t fontDataOffset;
    uint16_t bufferPos;
    uint8_t fontByte;
    uint8_t bytesPerWidth = 1;
    uint8_t maskLeft;
    uint8_t maskRight;
    uint8_t textIndex = 0;
    uint8_t h = fonts[fontIndex].height;
    uint8_t w = fonts[fontIndex].width;
    bool needSpace = false;
    const unsigned char *fontData = fonts[fontIndex].data;
    if (w > 8)
        bytesPerWidth++;

    for (uint8_t d = 0; d < h; d++)
    {
        setDirtyLine(printYPos - d);
    }

    while (true)
    {
        char ch = text.charAt(textIndex++);
        if (ch == '\0')
            break;

        fontDataOffset = h;
        fontDataOffset *= (uint8_t(ch) - fonts[fontIndex].offset);
        if (bytesPerWidth > 1)
            fontDataOffset = (fontDataOffset << 1) + 1;

        if (printXPos + w + needSpace > printRect.x2)
        {
            if (printYPos + 1 + h <= printRect.y2)
            {
                printYPos += 1 + h;
                printXPos = printRect.x1;
                needSpace = false;
                for (uint8_t d = 0; d < h; d++)
                {
                    setDirtyLine(printYPos - d);
                }
            }
            else
            {
                break;
            }
        }
        else if (needSpace)
        {
            printXPos += 1;
            if (drawMode == modeSet)
                drawVLine(printXPos - 1, printYPos - h + 1, h, modeClear);
            else if (drawMode == modeClear)
                drawVLine(printXPos - 1, printYPos - h + 1, h, modeSet);
        }

        for (uint8_t fx = 0; fx < bytesPerWidth; fx++)
        {
            uint8_t freeBits = 8 - (printXPos & 0x07);
            uint16_t fontDataOffset2 = fontDataOffset;
            uint8_t bitsToPrint;
            if (fx)
            {
                bitsToPrint = 8;
                fontDataOffset2--;
            }
            else
            {
                bitsToPrint = (w > 8) ? (w & 0x07) : (w);
            }
            uint8_t shiftCountLeft;
            uint8_t shiftCountRight;
            if (freeBits >= bitsToPrint)
            {
                shiftCountLeft = freeBits - bitsToPrint;
                maskLeft = (1 << bitsToPrint) - 1;
                if (shiftCountLeft)
                    maskLeft <<= shiftCountLeft;
            }
            else
            {
                shiftCountRight = bitsToPrint - freeBits;
                shiftCountLeft = 8 - shiftCountRight;
                maskRight = (1 << bitsToPrint) - 1;
                maskLeft = maskRight;
                maskRight >>= shiftCountRight;
                maskLeft <<= shiftCountLeft;
            }

            for (uint8_t fy = printYPos - h + 1; fy <= printYPos; fy++)
            {
                bufferPos = 16 * fy + (printXPos >> 3);
                fontByte = pgm_read_byte(fontData + fontDataOffset2);
                if (freeBits < bitsToPrint)
                {
                    uint8_t fontPart = fontByte >> shiftCountRight;
                    setByteByPos(bufferPos++, fontPart, drawMode, maskRight);
                }
                fontByte = fontByte << shiftCountLeft;
                setByteByPos(bufferPos, fontByte, drawMode, maskLeft);
                fontDataOffset2 += bytesPerWidth;
                setDirtyLine(fy);
            }
            printXPos += bitsToPrint;
        }
        needSpace = true;
    }
}

void Display::print(char ch, DrawMode drawMode)
{
    print(&ch, 1, drawMode);
}

void Display::print(uint8_t value, DrawMode drawMode)
{
    print((unsigned long)value, drawMode);
}

void Display::print(unsigned long value, DrawMode drawMode)
{
    SchedulerSuspender lock;
    char buf[8 * sizeof(long) + 1];
    char *str = &buf[sizeof(buf) - 1];
    *str = '\0';
    do
    {
        char c = value % 10;
        value /= 10;

        *--str = c + '0';
    } while (value);
    print(str, drawMode);
}

void Display::print(long value, DrawMode drawMode)
{
    SchedulerSuspender lock;
    if (value < 0)
    {
        print('-', drawMode);
        value = -value;
    }
    print((unsigned long)value, drawMode);
}

void Display::print(int value, DrawMode drawMode)
{
    return print((long)value, drawMode);
}

void Display::print(unsigned int value, DrawMode drawMode)
{
    return print((unsigned long)value, drawMode);
}

bool Display::getBacklight()
{
    return backlightLevel != backlightOff;
}

void Display::resetPrintRect(bool resetPrintPos)
{
    SchedulerSuspender lock;
    printRect.x1 = 0;
    printRect.y1 = 0;
    printRect.x2 = width() - 1;
    printRect.y2 = height() - 1;
    if (resetPrintPos)
        setPrintPos(printRect.x1, printRect.y1 + fonts[getSystemFontIndex()].height);
}

void Display::setPrintRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool resetPrintPos)
{
    SchedulerSuspender lock;
    printRect.x1 = min(x1, x2);
    printRect.y1 = min(y1, y2);
    printRect.x2 = max(x1, x2);
    printRect.y2 = max(y1, y2);
    if (resetPrintPos)
        setPrintPos(printRect.x1, printRect.y1 + fonts[getSystemFontIndex()].height);
}

/*
void Display::copyRect(uint8_t **buf, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    uint8_t minX = (x1 < x2) ? x1 : x2;
    uint8_t maxX = (x1 < x2) ? x2 : x1;
    uint8_t minY = (y1 < y2) ? y1 : y2;
    uint8_t maxY = (y1 < y2) ? y2 : y1;

    // Вычисляем ширину и высоту фрагмента
    // uint8_t width = maxX - minX + 1;  // количество пикселей по ширине
    uint8_t height = maxY - minY + 1; // количество строк

    // Вычисляем начальный и конечный байты
    uint8_t startByte = minX >> 3; // индекс байта для minX
    uint8_t endByte = maxX >> 3;   // индекс байта для maxX

    // Вычисляем необходимый размер памяти
    size_t requiredSize = height * (endByte - startByte + 1);

    // Буфер целиком перезаписывается ниже, поэтому старое содержимое сохранять
    // не нужно: освобождаем прежний блок и выделяем новый из кучи FreeRTOS.
    if (*buf != NULL)
        vPortFree(*buf);
    *buf = (uint8_t *)pvPortMalloc(requiredSize);
    checkBuf(buf);

    for (uint8_t row = 0; row < height; row++)
    {
        const uint8_t *srcRow = (const uint8_t *)frameBuffer + (minY + row) * 16;
        uint8_t *bufRow = *buf + row * (endByte - startByte + 1);
        memcpy(bufRow, srcRow + startByte, endByte - startByte + 1);
    }
}

void Display::pasteRect(uint8_t *buf, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    uint8_t minX = (x1 < x2) ? x1 : x2;
    uint8_t maxX = (x1 < x2) ? x2 : x1;
    uint8_t minY = (y1 < y2) ? y1 : y2;
    uint8_t maxY = (y1 < y2) ? y2 : y1;

    // Вычисляем ширину и высоту фрагмента
    // uint8_t width = maxX - minX + 1;  // количество пикселей по ширине
    uint8_t height = maxY - minY + 1; // количество строк

    // Вычисляем начальный и конечный байты
    uint8_t startByte = minX >> 3; // индекс байта для minX
    uint8_t endByte = maxX >> 3;   // индекс байта для maxX

    for (uint8_t row = 0; row < height; row++)
    {
        uint8_t *dstRow = (uint8_t *)frameBuffer + (minY + row) * 16;
        const uint8_t *srcRow = buf + row * (endByte - startByte + 1);
        memcpy((void *)dstRow + startByte, srcRow, endByte - startByte + 1);
    }
    for (int y = y1; y < y2; y++)
    {
        setDirtyLine(y);
    }
}


void Display::screenshot()
{
    FsFile f = sdCard.open("scr.xbm", FILE_WRITE);
    // Открываем файл для записи
    if (!f)
    {
        DEBUG_PRINTLN(F("Ошибка открытия файла для записи"));
        return;
    }
    uint8_t *p = (uint8_t *)frameBuffer;
    f.print("#define scr_width ");
    f.println(width());
    f.print("#define scr_height ");
    f.println(height());
    f.println("static char scr_bits[] = {");

    for (uint8_t y = 0; y < height(); y++)
    {
        for (uint8_t x = 0; x < width() / 8; x++)
        {
            f.print("0x");
            f.print(MyUtils::flipBits(*p++), HEX);
            f.print(",");
        }
        f.println();
    }

    f.println("};");
    f.close();
}
*/
bool Display::anyOverlayRow(uint8_t y)
{
    for (uint8_t i = 0; i < MAX_OVERLAYS; i++)
    {
        const Overlay &ov = overlays[i];
        if (ov.active && ov.buf && y >= ov.y && y < (uint8_t)(ov.y + ov.h))
            return true;
    }
    return false;
}

void Display::mergeOverlaysIntoRow(uint8_t y)
{
    bool merged = false;
    for (uint8_t i = 0; i < MAX_OVERLAYS; i++)
    {
        const Overlay &ov = overlays[i];
        if (!ov.active || !ov.buf)
            continue;
        if (y < ov.y || y >= (uint8_t)(ov.y + ov.h))
            continue;

        if (!merged)
        {
            // Копируем базовую строку целиком, дальше подменяем байты оверлеев
            memcpy(rowMergeBuf, (const void *)isrDataPointer, 16);
            merged = true;
        }

        uint8_t byteW = ov.w >> 3;
        uint8_t startByte = ov.x >> 3;
        const uint8_t *src = ov.buf + (uint16_t)(y - ov.y) * byteW;
        memcpy(rowMergeBuf + startByte, src, byteW);
    }

    if (merged)
        isrDataPointer = rowMergeBuf;
}

Overlay *Display::addOverlay(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    x &= ~7;          // левый край по байту
    w = (w + 7) & ~7; // ширина кратна байту

    if (x >= width() || y >= height())
        return nullptr;
    if (x + w > width())
        w = width() - x;
    if (y + h > height())
        h = height() - y;
    if (w == 0 || h == 0)
        return nullptr;

    for (uint8_t i = 0; i < MAX_OVERLAYS; i++)
    {
        if (!overlays[i].active)
        {
            uint8_t byteW = w >> 3;
            overlays[i].buf = (uint8_t *)pvPortMalloc((uint16_t)byteW * h);
            if (overlays[i].buf == nullptr)
                return nullptr;

            overlays[i].x = x;
            overlays[i].y = y;
            overlays[i].w = w;
            overlays[i].h = h;
            overlays[i].active = true;

            for (uint8_t r = y; r < (uint8_t)(y + h); r++)
                setDirtyLine(r);

            return &overlays[i];
        }
    }
    return nullptr;
}

void Display::snapshotOverlay(Overlay *ov)
{
    if (!ov || !ov->buf)
        return;

    uint8_t byteW = ov->w >> 3;
    uint8_t startByte = ov->x >> 3;
    for (uint8_t r = 0; r < ov->h; r++)
    {
        const uint8_t *srcRow = (const uint8_t *)frameBuffer + (uint16_t)(ov->y + r) * 16 + startByte;
        memcpy(ov->buf + (uint16_t)r * byteW, srcRow, byteW);
        setDirtyLine(ov->y + r);
    }
}

void Display::removeOverlay(Overlay *ov)
{
    if (!ov)
        return;

    for (uint8_t r = ov->y; r < (uint8_t)(ov->y + ov->h); r++)
        setDirtyLine(r);

    if (ov->buf)
    {
        vPortFree(ov->buf);
        ov->buf = nullptr;
    }
    ov->active = false;
}

void Display::service()
{
    if (TCCR5B != 0 || isUpdating)
        return;
    SchedulerSuspender lock;
    for (uint8_t i = 0; i < 64; i++)
    {
        if (isDirtyLine(repaintY) /*|| anyOverlayRow(repaintY)*/)
        {
            uint8_t logicalY = repaintY & 0x1F;
            if (repaintY >= 32)
                isrDataPointer = &frameBuffer[(logicalY << 4) + 512];
            else
                isrDataPointer = &frameBuffer[logicalY << 4];

            // Домешать оверлеи: isrDataPointer может переехать на rowMergeBuf
            mergeOverlaysIntoRow(repaintY);
            lcdWrite(0, cmdSetDDRAMAddress + logicalY);
            lcdWrite(0, cmdSetDDRAMAddress + (repaintY >= 32 ? 8 : 0));
            bytesToSend = 16;
            // SET_RW_RS(0, 1);
            clearDirtyLine(repaintY);         // Авансом считаем ее перерисованной
            repaintY = (repaintY + 1) & 0x3F; // и переключаем указатель на следующую строку
            startDisplayTimer();
            break;
        }
        repaintY = (repaintY + 1) & 0x3F;
    }
}

void Display::startDisplayTimer()
{
    uint8_t sreg = SREG;
    cli();
    TCCR5A = 0;
    TCCR5B = 0;
    // OCR5A = 95;   // период 6 мкс  (96 тактов)
    OCR5A = 127; // период 8 мкс  (128 тактов)
    // OCR5A = 159;  // период 10 мкс (160 тактов)

    TCNT5 = 0;
    TIFR5 = (1 << OCF5A); // Сбросить отложенное совпадение от предыдущей строки.
    TIMSK5 = (1 << OCIE5A);
    TCCR5B = (1 << WGM52) | (1 << CS50);
    SREG = sreg;
}

/*
ISR(TIMER5_COMPA_vect)
{
    DDRK = 0x00;
    SET_RW_RS(1, 0);
    SET_E();
    asm volatile("nop\n nop");
    if (PINK & 0x80)
    {
        CLR_E();
        return;
    }
    CLR_E();
    DDRK = 0xFF;
    SET_RW_RS(0, 1);
    asm volatile("nop");
    SET_E();
    PORTK = *isrDataPointer++;
    asm volatile("nop");
    CLR_E();
    if (--bytesToSend == 0)
    {
        TIMSK5 &= ~(1 << OCIE5A);
        TCCR5B = 0;
    }
}
*/

// ISR переведён на naked, чтобы управлять порядком сохранения регистров
// Когда дисплей занят (это наиболее частый кейс) сохраняются только r0, SREG и r24
// Остальные регистры нужны исключительно при фактической передаче байта
// clang-format off
ISR(TIMER5_COMPA_vect, ISR_NAKED)
{
    asm volatile(
        "push r0" "\n\t"
        "in r0, __SREG__" "\n\t"
        "push r0" "\n\t"
        "push r24" "\n\t"

        // Проверить busy-флаг: RW=1, RS=0, E=1.
        "ldi r24, 0" "\n\t"
        "sts %[ddrk], r24" "\n\t"
        "in r24, %[portg]" "\n\t"
        "andi r24, 0xf8" "\n\t"
        "ori r24, 0x02" "\n\t"
        "out %[portg], r24" "\n\t"
        "sbi %[portg], 2" "\n\t"
        "nop" "\n\t"
        "nop" "\n\t"
        "lds r24, %[pink]" "\n\t"
        "cbi %[portg], 2" "\n\t"
        "sbrc r24, 7" "\n\t"
        "rjmp isr_exit" "\n\t"

        // Дисплей готов: сохранить регистры, нужные для передачи байта.
        "push r18" "\n\t"
        "push r30" "\n\t"
        "push r31" "\n\t"
        "ldi r24, 0xff" "\n\t"
        "sts %[ddrk], r24" "\n\t"
        "in r24, %[portg]" "\n\t"
        "andi r24, 0xf8" "\n\t"
        "ori r24, 0x01" "\n\t"
        "out %[portg], r24" "\n\t"
        "lds r30, isrDataPointer" "\n\t"
        "lds r31, isrDataPointer + 1" "\n\t"
        "ld r18, Z+" "\n\t"
        "sts isrDataPointer, r30" "\n\t"
        "sts isrDataPointer + 1, r31" "\n\t"
        "sts %[portk], r18" "\n\t"

        "nop" "\n\t"
        "sbi %[portg], 2" "\n\t"
        "nop" "\n\t"
        "cbi %[portg], 2" "\n\t"

        // Проверить, последний ли это байт
        "lds r24, bytesToSend" "\n\t"
        "subi r24, 1" "\n\t"
        "sts bytesToSend, r24" "\n\t"
        "breq stop_timer" "\n\t"  // Если последний - перейти к остановке таймера

        // Не последний: сбросить таймер (ISR выполняется ~5мкс, следующий вызов без сброса будет слишком рано)
        "ldi r24, 0" "\n\t"
        "sts %[tcnt5h], r24" "\n\t"
        "sts %[tcnt5l], r24" "\n\t"
        "rjmp restore_extra_regs" "\n\t"

        // Последний байт: остановить Timer5
        "stop_timer:" "\n\t"
        "lds r24, %[timsk5]" "\n\t"
        "andi r24, 0xfb" "\n\t"
        "sts %[timsk5], r24" "\n\t"
        "ldi r24, 0" "\n\t"
        "sts %[tccr5b], r24" "\n\t"

        // RW=0, RS=0, E=0
        //"in r24, %[portg]" "\n\t"
        //"andi r24, 0xf8" "\n\t"
        //"out %[portg], r24" "\n\t"

        "restore_extra_regs:" "\n\t"
        "pop r31" "\n\t"
        "pop r30" "\n\t"
        "pop r18" "\n\t"

        // Общий выход; r24, SREG и r0 были сохранены до polling.
        "isr_exit:" "\n\t"
        "pop r24" "\n\t"
        "pop r0" "\n\t"
        "out __SREG__, r0" "\n\t"
        "pop r0" "\n\t"
        "reti" "\n\t"
        :
        : [ddrk] "n" (_SFR_MEM_ADDR(DDRK)),
          [pink] "n" (_SFR_MEM_ADDR(PINK)),
          [portg] "I" (_SFR_IO_ADDR(PORTG)),
          [portk] "n" (_SFR_MEM_ADDR(PORTK)),
          [timsk5] "n" (_SFR_MEM_ADDR(TIMSK5)),
          [tccr5b] "n" (_SFR_MEM_ADDR(TCCR5B)),
          [tcnt5h] "n" (_SFR_MEM_ADDR(TCNT5H)),
          [tcnt5l] "n" (_SFR_MEM_ADDR(TCNT5L)));
}
// clang-format on
