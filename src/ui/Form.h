#ifndef UI_FORM_H
#define UI_FORM_H

#include <inttypes.h>
#include "UIElement.h"
#include "../Keyboard.h"
#include "../Display.h"
#include "../DS3231.h"
#include "TextRef.h"
#include "Arduino_FreeRTOS.h"

// Forward declarations для UI элементов
class Button;
class IconButton;
class CheckBox;
class ProgressBar;
class Label;
class Edit;
typedef Edit TextBox;
class DateEdit;
class TimeEdit;
class Memo;
class List;
class ArrayList;
class Menu;
class FileList;

/**
 * @brief Базовый класс формы — контейнер для UI элементов
 *
 * Форма представляет собой контейнер, содержащий набор UI элементов (кнопки, поля ввода,
 * списки и т.д.). Формы управляют фокусом элементов, обрабатывают события клавиатуры.
 * Формы организованы в стек на задаче (Task) — верхняя форма активна.
 *
 * По умолчанию форма занимает весь экран. Для создания форм с произвольными размерами
 * используйте FormEx.
 */
class Form
{
public:
    /**
     * @brief Конструктор формы
     */
    Form();

    /**
     * @brief Виртуальный деструктор
     */
    virtual ~Form();

    // ============== Управление элементами ==============

    /**
     * @brief Добавляет UI элемент на форму
     *
     * Форма становится владельцем элемента. При необходимости расширяет внутренний массив.
     *
     * @param el Указатель на элемент для добавления
     * @return Указатель на добавленный элемент (для цепочечных вызовов)
     */
    UIElement *addElement(UIElement *el);

    /**
     * @brief Удаляет элемент с формы
     *
     * Не удаляет объект элемента из памяти — за это отвечает вызывающая сторона.
     *
     * @param el Указатель на элемент для удаления
     * @return true если элемент был найден и удалён
     */
    bool removeElement(UIElement *el);

    // ============== Управление видимостью ==============

    /**
     * @brief Показывает форму (добавляет в стек активных форм задачи)
     */
    void show();

    /**
     * @brief Скрывает форму (убирает из стека, но не удаляет)
     */
    void hide();

    /**
     * @brief Закрывает форму окончательно (удаляет из стека)
     */
    void close();

    // ============== Отрисовка и обновление ==============

    /**
     * @brief Отрисовывает форму и все её элементы
     *
     * Рисует рамку (если включена), заголовок (если включён) и вызывает draw()
     * для каждого видимого элемента. Элементы верхнего уровня рисуются последними.
     */
    virtual void draw();

    /**
     * @brief Перерисовывает элементы верхнего слоя (меню, всплывающие окна)
     */
    void redrawTopLayer();

    /**
     * @brief Обновляет состояние формы и её элементов (периодический вызов)
     * @param nowTicks Текущее время в тиках FreeRTOS
     */
    virtual void update(TickType_t nowTicks);

    /**
     * @brief Обрабатывает нажатие клавиши
     *
     * Сначала передаёт событие сфокусированному элементу. Если элемент не обработал,
     * форма может обработать клавишу сама (Tab/Shift+Tab для смены фокуса).
     *
     * @param key Код клавиши
     * @return true если клавиша обработана
     */
    virtual bool onKey(uint8_t key);

    /**
     * @brief Обрабатывает состояние клавиатуры (для комбинаций клавиш)
     * @param kb Структура состояния клавиатуры
     * @return true если событие обработано
     */
    virtual bool onKey(const KBState &kb);

    /**
     * @brief Обрабатывает события от UI элементов
     *
     * Вызывается элементами через triggerEvent(). Переопределите этот метод
     * для обработки кликов по кнопкам, изменений значений и других событий.
     *
     * @param sender Указатель на элемент, сгенерировавший событие
     * @param event Тип события (EVENT_CLICK, EVENT_VALUE_CHANGED и т.д.)
     * @param value Дополнительное значение события
     * @return true если событие обработано
     */
    virtual bool onEvent(UIElement *sender, uint8_t event, uint8_t value)
    {
        (void)sender;
        (void)event;
        (void)value;
        return false;
    }

    // ============== Управление фокусом ==============

    /**
     * @brief Переключает фокус на следующий фокусируемый элемент
     */
    void focusNext();

    /**
     * @brief Переключает фокус на предыдущий фокусируемый элемент
     */
    void focusPrev();

    /**
     * @brief Устанавливает фокус на указанный элемент
     * @param element Указатель на элемент
     * @return true если фокус установлен успешно
     */
    bool setFocusedElement(UIElement *element);

    /**
     * @brief Получает указатель на элемент, имеющий фокус
     * @return Указатель на сфокусированный элемент или nullptr
     */
    UIElement *getFocused() const;

    // ============== Свойства формы ==============

    /**
     * @brief Устанавливает видимость формы
     * @param v true — видима, false — скрыта
     */
    void setVisible(bool v);

    /**
     * @brief Проверяет видимость формы
     * @return true если форма видима
     */
    bool isVisible() const;

    /**
     * @brief Устанавливает заголовок формы (строка из RAM)
     * @param text Текст заголовка
     */
    void setCaption(const char *text);

    /**
     * @brief Устанавливает заголовок формы (строка из PROGMEM)
     * @param text Текст заголовка из Flash памяти
     */
    void setCaption(const __FlashStringHelper *text);

    /**
     * @brief Получает заголовок формы как C-строку (только для RAM)
     * @return Указатель на заголовок или nullptr, если заголовок в PROGMEM
     */
    const char *getCaption() const { return caption.c_str(); }

    /**
     * @brief Получает заголовок формы как TextRef (работает для RAM и PROGMEM)
     * @return Ссылка на заголовок
     */
    TextRef getCaptionRef() const { return caption; }

    /**
     * @brief Устанавливает задачу-владельца формы
     * @param task Handle задачи FreeRTOS
     */
    void setOwnerTask(TaskHandle_t task);

    /**
     * @brief Получает задачу-владельца формы
     * @return Handle задачи или nullptr
     */
    TaskHandle_t getOwnerTask() const { return ownerTask; }

    /**
     * @brief Запрашивает закрытие задачи-владельца
     */
    void requestCloseOwnerTask();

    /**
     * @brief Включает/выключает отрисовку рамки формы
     * @param draw true — рисовать рамку
     */
    void setDrawBorder(bool draw);

    /**
     * @brief Проверяет, отрисовывается ли рамка
     * @return true если рамка включена
     */
    bool isDrawBorder() const { return flags & FORM_FLAG_DRAW_BORDER; }

    /**
     * @brief Включает/выключает отображение заголовка
     * @param show true — показывать заголовок
     */
    void setShowCaption(bool show);

    /**
     * @brief Проверяет, отображается ли заголовок
     * @return true если заголовок включён
     */
    bool isShowCaption() const { return flags & FORM_FLAG_SHOW_CAPTION; }

    // ============== Геометрия формы ==============

    /**
     * @brief Получает X-координату левого края формы
     * @return Координата X (по умолчанию 0)
     */
    virtual uint8_t formLeft() const;

    /**
     * @brief Получает Y-координату верхнего края формы
     * @return Координата Y (по умолчанию 0)
     */
    virtual uint8_t formTop() const;

    /**
     * @brief Получает ширину формы
     * @return Ширина в пикселях (по умолчанию 128 — вся ширина экрана)
     */
    virtual uint8_t formWidth() const;

    /**
     * @brief Получает высоту формы
     * @return Высота в пикселях (по умолчанию 64 — вся высота экрана)
     */
    virtual uint8_t formHeight() const;

    /**
     * @brief Получает X-координату правого края формы
     * @return Координата X (left + width - 1)
     */
    uint8_t formRight() const;

    /**
     * @brief Получает Y-координату нижнего края формы
     * @return Координата Y (top + height - 1)
     */
    uint8_t formBottom() const;

    /**
     * @brief Получает X-координату левого края клиентской области
     * @return Координата X (с учётом рамки)
     */
    uint8_t clientLeft() const;

    /**
     * @brief Получает Y-координату верхнего края клиентской области
     * @return Координата Y (с учётом рамки и заголовка)
     */
    uint8_t clientTop() const;

    /**
     * @brief Получает X-координату правого края клиентской области
     * @return Координата X
     */
    uint8_t clientRight() const;

    /**
     * @brief Получает Y-координату нижнего края клиентской области
     * @return Координата Y
     */
    uint8_t clientBottom() const;

    /**
     * @brief Получает ширину клиентской области
     * @return Ширина в пикселях (с учётом рамок)
     */
    uint8_t clientWidth() const;

    /**
     * @brief Получает высоту клиентской области
     * @return Высота в пикселях (с учётом рамок и заголовка)
     */
    uint8_t clientHeight() const;

    // ============== Примитивы рисования (обёртки над Display) ==============

    void drawRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode drawMode, bool infill = false);
    void drawHLine(uint8_t x, uint8_t y, uint8_t len, DrawMode drawMode = modeSet, bool doted = false);
    void drawVLine(uint8_t x, uint8_t y, uint8_t len, DrawMode drawMode = modeSet, bool doted = false);
    void drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, DrawMode drawMode = modeSet);
    void drawCircle(uint8_t x, uint8_t y, uint8_t r, DrawMode drawMode = modeSet);
    void drawEllipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b, DrawMode drawMode = modeSet);
    void drawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, DrawMode drawMode = modeSet);
    void drawXBM(const uint8_t *data, uint8_t width, uint8_t height, uint8_t left, uint8_t top, DrawMode drawMode = modeSet);
    void setPrintPos(uint8_t x, uint8_t y);
    void setPrintRect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool resetPrintPos = true);
    void print(const char *str, DrawMode drawMode = modeSet);
    void print(const char *str, uint8_t len, DrawMode drawMode = modeSet);
    void print(const char *str, uint8_t len, uint8_t fontIndex, DrawMode drawMode = modeSet);
    void print(const __FlashStringHelper *str, DrawMode drawMode = modeSet);
    void print(const __FlashStringHelper *str, uint8_t fontIndex, DrawMode drawMode = modeSet);
    void print(TextRef text, DrawMode drawMode = modeSet);
    void print(uint8_t value, DrawMode drawMode = modeSet);
    void resetPrintRect();

    // ============== Хелперы для быстрого создания элементов ==============

    Button *addButton(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char *caption);
    Button *addButton(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const __FlashStringHelper *caption);
    IconButton *addIconButton(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
                              const uint8_t *icon, uint8_t iconWidth, uint8_t iconHeight);
    IconButton *addIconButton(uint8_t left, uint8_t top, const uint8_t *icon, uint8_t iconSize);
    CheckBox *addCheckBox(uint8_t left, uint8_t top, bool checked, const char *caption);
    CheckBox *addCheckBox(uint8_t left, uint8_t top, bool checked, const __FlashStringHelper *caption);
    ProgressBar *addProgressBar(uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t minValue, uint8_t maxValue);
    Label *addLabel(const char *text, uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t hAlign = 0, uint8_t vAlign = 0);
    Label *addLabel(const __FlashStringHelper *text, uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t hAlign = 0, uint8_t vAlign = 0);
    Label *addLabel(TextRef text, uint8_t left, uint8_t top, uint8_t width, uint8_t height, uint8_t hAlign = 0, uint8_t vAlign = 0);
    TextBox *addTextBox(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const char *text = "", uint8_t maxLength = 16);
    DateEdit *addDateEdit(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const RTCDate &value = RTCDate());
    TimeEdit *addTimeEdit(uint8_t left, uint8_t top, uint8_t width, uint8_t height, const RTCTime &value = RTCTime());
    Memo *addMemo(uint8_t left, uint8_t top, uint8_t width, uint8_t height);
    List *addList(uint8_t left, uint8_t top, uint8_t width, uint8_t height);
    ArrayList *addArrayList(uint8_t left, uint8_t top, uint8_t width, uint8_t height);
    Menu *addMenu(uint8_t left, uint8_t top, uint8_t width, uint8_t height);
    FileList *addFileList(uint8_t left, uint8_t top, uint8_t width, uint8_t height);

private:
    static const uint8_t FORM_FLAG_VISIBLE = 0x01;      ///< Флаг видимости формы
    static const uint8_t FORM_FLAG_DRAW_BORDER = 0x02;  ///< Флаг отрисовки рамки
    static const uint8_t FORM_FLAG_SHOW_CAPTION = 0x04; ///< Флаг отображения заголовка

    TextRef caption;         ///< Заголовок формы
    UIElement **elements;    ///< Динамический массив указателей на элементы
    uint8_t elementCount;    ///< Количество элементов на форме
    uint8_t elementCapacity; ///< Ёмкость массива элементов
    uint8_t focusedIndex;    ///< Индекс сфокусированного элемента
    uint8_t flags;           ///< Флаги состояния формы
    TaskHandle_t ownerTask;  ///< Задача-владелец формы

    /**
     * @brief Расширяет массив элементов при необходимости
     */
    void ensureCapacity();
};

#endif
