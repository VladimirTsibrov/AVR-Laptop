#ifndef UIELEMENT_H
#define UIELEMENT_H

#include "UIEvents.h"
#include "../Keyboard.h"
#include <inttypes.h>

class Form;
class UIElement;
bool postUIEvent(Form *form, UIElement *sender, uint8_t event, uint8_t value);

/**
 * @brief Базовый абстрактный класс для всех UI элементов
 *
 * Определяет общий интерфейс для элементов управления: отрисовку, обработку событий,
 * управление фокусом и видимостью. Все конкретные UI элементы (кнопки, текстовые поля,
 * списки и т.д.) наследуются от этого класса.
 *
 * Элементы располагаются на формах (Form) и взаимодействуют с ней через события.
 */
class UIElement
{
public:
    static const uint8_t FLAG_FOCUSED = 0x01; ///< Флаг: элемент имеет фокус ввода
    static const uint8_t FLAG_VISIBLE = 0x02; ///< Флаг: элемент видим на экране

    /**
     * @brief Виртуальный деструктор
     */
    virtual ~UIElement() {}

    /**
     * @brief Отрисовывает элемент на экране
     *
     * Должен быть реализован в каждом конкретном элементе.
     * Вызывается системой UI при необходимости перерисовки.
     */
    virtual void draw() = 0;

    /**
     * @brief Обрабатывает нажатие клавиши
     * @param key Код клавиши
     * @return true если клавиша обработана элементом
     */
    virtual bool onKey(uint8_t key) { return false; }

    /**
     * @brief Обрабатывает состояние клавиатуры (для комбинаций клавиш)
     *
     * По умолчанию делегирует обработку методу onKey(uint8_t).
     *
     * @param kb Структура состояния клавиатуры
     * @return true если событие обработано
     */
    virtual bool onKey(const KBState &kb) { return onKey(kb.pressedKey); }

    /**
     * @brief Обновляет состояние элемента (вызывается периодически)
     *
     * Используется для анимации, мигания курсора и других динамических эффектов.
     *
     * @param nowTicks Текущее время в тиках FreeRTOS
     */
    virtual void update(TickType_t nowTicks) {}

    /**
     * @brief Определяет, может ли элемент получать фокус
     * @return true если элемент может получать фокус (по умолчанию — может)
     */
    virtual bool isFocusable() const { return true; }

    /**
     * @brief Определяет, отрисовывается ли элемент поверх остальных
     *
     * Элементы верхнего уровня (меню, всплывающие окна) отрисовываются последними.
     *
     * @return true если элемент находится на верхнем слое
     */
    virtual bool isOnTop() const { return false; }

    /**
     * @brief Устанавливает или снимает фокус с элемента
     *
     * При изменении фокуса генерируются события EVENT_FOCUS или EVENT_LOST_FOCUS.
     *
     * @param focus true — установить фокус, false — снять фокус
     */
    void setFocus(bool focus)
    {
        if (isFocused() != focus)
        {
            setFlag(FLAG_FOCUSED, focus);
            onFocusChanged();
            if (isFocused())
            {
                triggerEvent(EVENT_FOCUS);
            }
            else
            {
                triggerEvent(EVENT_LOST_FOCUS);
            }
        }
    }

    /**
     * @brief Проверяет, имеет ли элемент фокус
     * @return true если элемент сфокусирован
     */
    bool isFocused() const { return flags & FLAG_FOCUSED; }

    /**
     * @brief Запрашивает фокус для этого элемента через форму-владельца
     * @return true если фокус успешно установлен
     */
    bool focus();

    /**
     * @brief Устанавливает видимость элемента
     * @param vis true — сделать видимым, false — скрыть
     */
    void setVisible(bool vis) { setFlag(FLAG_VISIBLE, vis); }

    /**
     * @brief Проверяет, видим ли элемент
     * @return true если элемент видим
     */
    bool isVisible() const { return flags & FLAG_VISIBLE; }

    /**
     * @brief Получает X-координату левого края элемента
     * @return Координата X в пикселях
     */
    uint8_t getLeft() const { return left; }

    /**
     * @brief Получает Y-координату верхнего края элемента
     * @return Координата Y в пикселях
     */
    uint8_t getTop() const { return top; }

    /**
     * @brief Устанавливает маску событий, которые элемент может генерировать
     *
     * Только события, включённые в маску, будут отправлены форме через onEvent().
     *
     * @param mask Битовая маска событий (комбинация EVENT_*)
     */
    void setEventMask(uint8_t mask) { eventMask = mask; }

    /**
     * @brief Устанавливает форму-владельца элемента
     * @param form Указатель на форму, содержащую этот элемент
     */
    void setOwnerForm(Form *form) { ownerForm = form; }

    /**
     * @brief Получает форму-владельца элемента
     * @return Указатель на форму или nullptr
     */
    Form *getOwnerForm() const { return ownerForm; }

    /**
     * @brief Генерирует событие и отправляет его форме-владельцу
     *
     * Событие обрабатывается только если оно включено в eventMask.
     *
     * @param event Тип события (EVENT_CLICK, EVENT_VALUE_CHANGED и т.д.)
     * @param value Дополнительное значение события (по умолчанию 0)
     * @return true если событие было обработано формой
     */
    bool triggerEvent(uint8_t event, uint8_t value = 0)
    {
        if (!(eventMask & event))
            return false;

        return postUIEvent(ownerForm, this, event, value);
    }

protected:
    uint8_t left, top, width, height; ///< Позиция и размер элемента в пикселях
    uint8_t flags;                    ///< Флаги состояния (фокус, видимость)
    Form *ownerForm;                  ///< Указатель на форму-владельца
    uint8_t eventMask;                ///< Маска событий, которые элемент может генерировать

    /**
     * @brief Устанавливает или снимает флаг состояния
     * @param flag Флаг для изменения (FLAG_FOCUSED, FLAG_VISIBLE)
     * @param value true — установить флаг, false — снять флаг
     */
    void setFlag(uint8_t flag, bool value)
    {
        if (value)
            flags |= flag;
        else
            flags &= ~flag;
    }

    /**
     * @brief Восстанавливает отрисовку верхнего слоя UI после изменения элемента
     *
     * Используется когда элемент изменился и нужно перерисовать элементы поверх него.
     */
    void restoreTopLayer();

    /**
     * @brief Вызывается при изменении состояния фокуса
     *
     * Переопределяется в наследниках для выполнения действий при получении/потере фокуса.
     */
    virtual void onFocusChanged() {}
};

#endif // UIELEMENT_H
