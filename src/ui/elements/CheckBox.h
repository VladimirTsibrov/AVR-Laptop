#ifndef CHECKBOX_H
#define CHECKBOX_H

#include "../UIElement.h"
#include "../TextRef.h"

/**
 * @brief CheckBox — элемент управления «флажок» для выбора булевого значения
 * 
 * Представляет собой квадратик 8×8 с галочкой (checked) или пустой (unchecked)
 * и текстовой подписью справа. Используется для включения/выключения опций.
 * 
 * Особенности:
 * - Встроенные XBM-иконки для checked/unchecked состояний
 * - Автоматическое вычисление ширины элемента по длине текста
 * - Фиксированная высота 10 пикселей
 * - Переключение по Enter/Space
 * - Генерация EVENT_CLICK при переключении
 * - Генерация EVENT_VALUE_CHANGED при программном изменении состояния
 * - Поддержка строк из RAM и PROGMEM
 * 
 * @code
 * CheckBox* chkAutoSave = new CheckBox(10, 10, true, F("Автосохранение"));
 * chkAutoSave->setEventHandler(EVENT_CLICK, []() {
 *     bool enabled = chkAutoSave->isChecked();
 *     setAutoSave(enabled);
 * });
 * form->addElement(chkAutoSave);
 * @endcode
 * 
 * @see UIElement, TextRef
 */
class CheckBox : public UIElement
{
public:
    static const uint8_t FLAG_CHECKED = 0x10; ///< Флаг установленного состояния (галочка)

    /**
     * @brief Создаёт CheckBox с текстом из RAM
     * @param left Левая координата X
     * @param top Верхняя координата Y
     * @param checked Начальное состояние (true = отмечен, false = пустой)
     * @param caption Текст справа от флажка (строка в RAM)
     * 
     * Ширина элемента вычисляется автоматически: 10 пикселей (квадратик) +
     * ширина текста. Высота фиксирована — 10 пикселей.
     */
    CheckBox(uint8_t left, uint8_t top, bool checked, const char *caption);
    
    /**
     * @brief Создаёт CheckBox с текстом из PROGMEM
     * @param left Левая координата X
     * @param top Верхняя координата Y
     * @param checked Начальное состояние
     * @param caption Текст справа от флажка (F-строка из PROGMEM)
     */
    CheckBox(uint8_t left, uint8_t top, bool checked, const __FlashStringHelper *caption);
    
    /**
     * @brief Отрисовывает CheckBox на экране
     * 
     * Рисует квадратик 8×8 с иконкой (checked/unchecked) и текст справа.
     * При фокусе область элемента инвертируется (заливка white).
     */
    void draw() override;
    
    /**
     * @brief Обрабатывает нажатие клавиш
     * @param key Код нажатой клавиши
     * @return true если клавиша обработана (Enter/Space), false если игнорируется
     * 
     * При нажатии Enter или Space переключает состояние checked/unchecked
     * и генерирует события EVENT_CLICK и EVENT_VALUE_CHANGED.
     */
    bool onKey(uint8_t key) override;
    
    /**
     * @brief Устанавливает новый текст (RAM строка)
     * @param text Новый текст для отображения
     * @note Не пересчитывает ширину элемента. Для изменения текста
     *       с пересчётом ширины создайте новый CheckBox.
     */
    void setCaption(const char *text);
    
    /**
     * @brief Устанавливает новый текст (PROGMEM строка)
     * @param text Новый текст для отображения (F-строка)
     */
    void setCaption(const __FlashStringHelper *text);
    
    /**
     * @brief Возвращает указатель на текст как C-строку
     * @return Указатель на C-строку текста (может быть в RAM или PROGMEM)
     * @see getCaptionRef
     */
    const char *getCaption() const { return caption.c_str(); }
    
    /**
     * @brief Возвращает текст как TextRef объект
     * @return TextRef обёртка, позволяющая единообразно работать с RAM/PROGMEM
     * @see TextRef
     */
    TextRef getCaptionRef() const { return caption; }
    
    /**
     * @brief Проверяет, отмечен ли CheckBox
     * @return true если отмечен (галочка), false если пустой
     */
    bool isChecked() const { return flags & FLAG_CHECKED; }
    
    /**
     * @brief Устанавливает состояние CheckBox программно
     * @param checked Новое состояние (true = отметить, false = снять)
     * 
     * Обновляет флаг FLAG_CHECKED, перерисовывает элемент и генерирует
     * событие EVENT_VALUE_CHANGED. Не генерирует EVENT_CLICK.
     */
    void setChecked(bool checked);

private:
    TextRef caption;        ///< Текст справа от флажка (универсальная обёртка RAM/PROGMEM)
    uint8_t textX, textY;   ///< Кэшированные координаты текста (не используются в текущей реализации)

protected:
    /**
     * @brief Вызывается при изменении фокуса
     * 
     * Перерисовывает элемент с новым состоянием фокуса (инверсия фона)
     * и восстанавливает верхний слой UI.
     */
    void onFocusChanged() override;
};

#endif // CHECKBOX_H
