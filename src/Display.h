#ifndef DISPLAY_H
#define DISPLAY_H
#include <inttypes.h>
#include <string.h>
#include "ui/TextRef.h"

class __FlashStringHelper;

#define cmdClear B00000001
#define cmdDisplay B00001000
#define bitBlink B00000001
#define bitCursor B00000010
#define bitDisplay B00000100
#define cmdFunctionSet B00100000
#define bitDL B00010000
#define bitRE B00000100
#define bitG B00000010
#define cmdSetDDRAMAddress B10000000

struct Font
{
    const char *fontName;
    uint8_t width;
    uint8_t height;
    const uint8_t *data;
    uint8_t offset;
};

enum DrawMode
{
    modeSet = 0,
    modeClear,
    modeInvert
};

struct Rect
{
    uint8_t x1;
    uint8_t y1;
    uint8_t x2;
    uint8_t y2;
};

enum BacklightLevel : uint8_t
{
    backlightOff = 0,
    backlightLow,
    backlightMedium,
    backlightHigh,
    backlightMax
};

// Максимум одновременных оверлеев (фрагментов, домешиваемых поверх базы)
#define MAX_OVERLAYS 4

// Оверлей — прямоугольный фрагмент со своим буфером, который компоновщик
// (Display::service) домешивает поверх основного видеобуфера на этапе отправки
// строки на дисплей.
struct Overlay
{
    uint8_t x;    // левый край, кратен 8
    uint8_t y;    // верхняя строка
    uint8_t w;    // ширина, кратна 8
    uint8_t h;    // высота
    uint8_t *buf; // (w/8)*h байт — содержимое оверлея
    bool active;
};

class Display
{
private:
    uint8_t dirtyLines[8];
    bool isUpdating;
    uint8_t printXPos;
    uint8_t printYPos;
    Rect printRect;
    uint8_t fontCount;
    uint8_t repaintY;
    BacklightLevel backlightLevel;
    uint8_t lcdRead(uint8_t lcdMode);
    void lcdWrite(uint8_t lcdMode, uint8_t value);
    void setByteByXY(uint8_t x, uint8_t y, uint8_t value, DrawMode drawMode = modeSet);
    void setByteByPtr(uint8_t *p, uint8_t value, DrawMode drawMode = modeSet);
    void setByteByPos(uint16_t bufferPos, uint8_t value, DrawMode drawMode, uint8_t mask);
    void checkBuf(void *buf);
    void startDisplayTimer();
    void setDirtyLine(uint8_t y)
    {
        if (y < height())
            dirtyLines[y >> 3] |= (uint8_t)(1 << (y & 7));
    }
    void clearDirtyLine(uint8_t y) { dirtyLines[y >> 3] &= (uint8_t)~(1 << (y & 7)); }
    bool isDirtyLine(uint8_t y) const { return dirtyLines[y >> 3] & (uint8_t)(1 << (y & 7)); }
    void setAllDirtyLines() { memset(dirtyLines, 0xFF, sizeof(dirtyLines)); }

    // Рисовать в видеобуфер разрешено только активной задаче (и оболочке).
    bool drawAllowed();

    // Компоновщик оверлеев
    Overlay overlays[MAX_OVERLAYS];
    uint8_t rowMergeBuf[16];              // транзитный буфер строки для домешивания
    bool anyOverlayRow(uint8_t y);        // покрывает ли активный оверлей строку y
    void mergeOverlaysIntoRow(uint8_t y); // собрать смёрженную строку в rowMergeBuf

public:
    Font *fonts;
    Display();
    static constexpr uint8_t width() { return 128; }
    static constexpr uint8_t height() { return 64; }

    /// @brief Инициализация дисплея
    void begin();

    uint8_t *getFrameBuffer();

    /// @brief Очистка дисплея
    void cls();

    /// @brief Полная перерисовка дисплея
    void repaint();

    //void repaintOld();

    /// @brief Начать пакетное обновление дисплея
    /// @details Отключает автоматическую отрисовку грязных строк.
    ///          Все изменения будут применены только после вызова endUpdate().
    void beginUpdate();

    /// @brief Завершить пакетное обновление и перерисовать измененные строки
    void endUpdate();

    /// @brief Получить состояние пикселя (x, y)
    /// @param x координата X
    /// @param y координата y
    /// @return true - если пиксель установлен; false - если сброшен
    bool getPixel(uint8_t x, uint8_t y);

    /// @brief Пометить строку кадрового буфера как изменённую.
    /// @details Нужен при прямой записи в getFrameBuffer().
    void markDirtyLine(uint8_t y);

    /// @brief Установить состояние пикселя (x, y)
    /// @param x координата X
    /// @param y координата Y
    /// @param drawMode режим отрисовки (modeSet - установить пиксель, modeClear - очистить, modeInvert - инвертировать)
    void setPixel(uint8_t x, uint8_t y, DrawMode drawMode = modeSet);

    /// @brief Установить значение байта с координатами (x, y) в видеобуфере
    /// @param x координата X
    /// @param y координата Y
    /// @param value значение
    /// @param drawMode
    void setByte(uint8_t x, uint8_t y, uint8_t value, DrawMode drawMode = modeSet);

    /// @brief Нарисовать горизонтальную линию
    /// @param x координата X начала линии
    /// @param y координата Y начала линии
    /// @param len длина линии
    /// @param drawMode режим отрисовки (modeSet - установить пиксель, modeClear - очистить, modeInvert - инвертировать)
    /// @param doted рисовать пунктирную линию, если true
    void drawHLine(uint8_t x, uint8_t y, uint8_t wlen, DrawMode drawMode = modeSet, bool doted = false);

    /// @brief Нарисовать вертикальную линию
    /// @param x координата X начала линии
    /// @param y координата Y начала линии
    /// @param len длина линии
    /// @param drawMode режим отрисовки (modeSet - установить пиксель, modeClear - очистить, modeInvert - инвертировать)
    /// @param doted рисовать пунктирную линию, если true
    void drawVLine(uint8_t x, uint8_t y, uint8_t len, DrawMode drawMode = modeSet, bool doted = false);

    /// @brief Нарисовать прямоугольник
    /// @param x1 координата X верхнего левого угла
    /// @param y1 координата Y верхнего левого угла
    /// @param x2 координата X нижнего правого угла
    /// @param y2 координата Y нижнего правого угла
    /// @param drawMode режим отрисовки (modeSet - установить пиксель, modeClear - очистить, modeInvert - инвертировать)
    /// @param infill заполнить прямоугольник, если true
    void drawRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode drawMode = modeSet, bool infill = false);

    /// @brief Нарисовать линию между двумя точками
    /// @param x1 координата X первой точки
    /// @param y1 координата Y первой точки
    /// @param x2 координата X второй точки
    /// @param y2 координата Y второй точки
    /// @param drawMode режим отрисовки (modeSet - установить пиксель, modeClear - очистить, modeInvert - инвертировать)
    void drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode drawMode = modeSet);

    /// @brief Нарисовать окружность
    /// @param x координата X центра окружности
    /// @param y координата Y центра окружности
    /// @param r радиус окружности
    /// @param drawMode режим отрисовки (modeSet - установить пиксель, modeClear - очистить, modeInvert - инвертировать)
    void drawCircle(uint8_t x, uint8_t y, uint8_t r, DrawMode drawMode = modeSet);

    /// @brief Нарисовать эллипс
    /// @param x координата X центра эллипса
    /// @param y координата Y центра эллипса
    /// @param a радиус по горизонтали
    /// @param b радиус по вертикали
    /// @param drawMode режим отрисовки (modeSet - установить пиксель, modeClear - очистить, modeInvert - инвертировать)
    void drawEllipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b, DrawMode drawMode = modeSet);

    /// @brief Нарисовать треугольник
    /// @param x1 координата X первой вершины
    /// @param y1 координата Y первой вершины
    /// @param x2 координата X второй вершины
    /// @param y2 координата Y второй вершины
    /// @param x3 координата X третьей вершины
    /// @param y3 координата Y третьей вершины
    /// @param drawMode режим отрисовки (modeSet - установить пиксель, modeClear - очистить, modeInvert - инвертировать)
    void drawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, DrawMode drawMode = modeSet);

    /// @brief Вывести XBM изображение из flash памяти
    /// @param data указатель на данные изображения
    /// @param width ширина изображения
    /// @param height высота изображения
    /// @param left координата X левого верхнего угла изображения
    /// @param top координата Y левого верхнего угла изображения
    /// @param drawMode режим отрисовки (modeSet - установить пиксель, modeClear - очистить, modeInvert - инвертировать)
    void drawXBM(const uint8_t *data, uint8_t width, uint8_t height, uint8_t left = 0, uint8_t top = 0, DrawMode drawMode = modeSet);

    /// @brief Вывести XBM изображение из RAM буфера
    /// @param data указатель на данные изображения в RAM
    /// @param width ширина изображения
    /// @param height высота изображения
    /// @param left координата X левого верхнего угла изображения
    /// @param top координата Y левого верхнего угла изображения
    /// @param drawMode режим отрисовки
    void drawXBM_RAM(const uint8_t *data, uint8_t width, uint8_t height, uint8_t left = 0, uint8_t top = 0, DrawMode drawMode = modeSet);

    /// @brief Добавить шрифт
    /// @param fontName имя шрифта
    /// @param width ширина символа
    /// @param height высота символа
    /// @param data указатель на данные шрифта
    /// @param offset смещение символов
    /// @return индекс добавленного шрифта
    uint8_t addFont(const char *fontName, uint8_t width, uint8_t height, const uint8_t *data, uint8_t firstChar = 0);

    /// @brief Получить индекс системного шрифта
    uint8_t getSystemFontIndex() const;

    /// @brief Вывести строку
    /// @param str указатель на строку
    /// @param len длина строки
    /// @param drawMode режим отрисовки
    void print(const char *str, uint8_t len, DrawMode drawMode = modeSet);
    void print(const char *str, uint8_t len, uint8_t fontIndex, DrawMode drawMode = modeSet);
    void print(TextRef text, DrawMode drawMode = modeSet);
    void print(TextRef text, uint8_t fontIndex, DrawMode drawMode = modeSet);
    // void print(const String &str, DrawMode drawMode = modeSet);

    /// @brief Вывести строку
    /// @param str строка для вывода
    /// @param drawMode режим отрисовки
    void print(const char *str, DrawMode drawMode = modeSet);

    /// @brief Вывести символ
    /// @param ch символ для вывода
    /// @param drawMode режим отрисовки
    void print(char ch, DrawMode drawMode = modeSet);

    /// @brief Вывести беззнаковое число
    /// @param value число для вывода
    /// @param drawMode режим отрисовки
    void print(uint8_t value, DrawMode drawMode = modeSet);

    /// @brief Вывести беззнаковое длинное число
    /// @param value число для вывода
    /// @param drawMode режим отрисовки
    void print(unsigned long value, DrawMode drawMode = modeSet);

    /// @brief Вывести знаковое длинное число
    /// @param value число для вывода
    /// @param drawMode режим отрисовки
    void print(long value, DrawMode drawMode = modeSet);

    /// @brief Вывести знаковое число
    /// @param value число для вывода
    /// @param drawMode режим отрисовки
    void print(int value, DrawMode drawMode = modeSet);

    /// @brief Вывести беззнаковое число
    /// @param value число для вывода
    /// @param drawMode режим отрисовки
    void print(unsigned int value, DrawMode drawMode = modeSet);

    /// @brief Установить позицию для вывода текста
    /// @param x координата X
    /// @param y координата Y
    void setPrintPos(uint8_t x, uint8_t y);

    /// @brief Сбросить область для вывода текста
    /// @param resetPrintPos сбросить позицию вывода текста, если true
    void resetPrintRect(bool resetPrintPos = true);

    /// @brief Установить область для вывода текста
    /// @param x1 координата X верхнего левого угла
    /// @param y1 координата Y верхнего левого угла
    /// @param x2 координата X нижнего правого угла
    /// @param y2 координата Y нижнего правого угла
    /// @param resetPrintPos сбросить позицию вывода текста, если true
    void setPrintRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool resetPrintPos = true);

    /// @brief Проверить, включена ли подсветка дисплея
    /// @return true, если установлен уровень выше нуля
    bool getBacklight();

    /// @brief Установить уровень яркости подсветки дисплея
    void setBacklightLevel(BacklightLevel level);
    BacklightLevel getBacklightLevel() const { return backlightLevel; }
    void increaseBacklight();
    void decreaseBacklight();

    /// @brief Включить подсветку на максимум или выключить
    /// @param value true - максимальная яркость, false - выключить
    void setBacklight(bool value);

    /// @brief Создать оверлей (фрагмент поверх базы).
    /// @details X и ширина выравниваются по целому байту (кратно 8 пикселям).
    ///          Буфер выделяется, но не заполняется — нарисуйте содержимое в
    ///          основной видеобуфер и вызовите snapshotOverlay().
    /// @return указатель на оверлей или nullptr, если нет слота/памяти.
    Overlay *addOverlay(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

    /// @brief Снять слепок региона оверлея из видеобуфера в его буфер.
    /// @details Вызывать после отрисовки содержимого оверлея обычными lcd.*.
    void snapshotOverlay(Overlay *ov);

    /// @brief Убрать оверлей: пометить его строки грязными и освободить буфер.
    void removeOverlay(Overlay *ov);

    void service();
};

extern Display lcd;
#endif // DISPLAY_H
