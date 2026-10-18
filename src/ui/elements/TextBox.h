#ifndef EDIT_H
#define EDIT_H

#include "../UIElement.h"

/**
 * @brief Однострочное текстовое поле ввода с курсором и прокруткой
 * 
 * Позволяет вводить и редактировать текст с клавиатуры. Поддерживает:
 * - Вставку и удаление символов
 * - Мигающий курсор
 * - Горизонтальную прокрутку при переполнении
 * - Ограничение максимальной длины текста
 * - Переключение между режимами вставки и замены (через виртуальные методы)
 * 
 * Наследники (DateEdit, TimeEdit) переопределяют поведение курсора и валидацию
 * для ввода дат и времени в формате "ДД.ММ.ГГГГ" и "ЧЧ:ММ:СС".
 */
class Edit : public UIElement
{
public:
    static const uint8_t FLAG_CURSOR_VISIBLE = 0x10;  ///< Флаг видимости курсора (мигание)

    /**
     * @brief Конструктор текстового поля
     * @param left X-координата левого верхнего угла
     * @param top Y-координата левого верхнего угла
     * @param width Ширина поля в пикселях
     * @param height Высота поля в пикселях
     * @param text Начальный текст (по умолчанию пустая строка)
     * @param maxLength Максимальная длина текста (по умолчанию 16)
     * @param fontIndex Индекс шрифта для отрисовки (по умолчанию 0)
     */
    Edit(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
         const char *text = "", uint8_t maxLength = 16, uint8_t fontIndex = 0);

    /**
     * @brief Отрисовывает текстовое поле и курсор
     */
    void draw() override;
    
    /**
     * @brief Обновляет состояние курсора (мигание)
     * @param nowTicks Текущее время в тиках FreeRTOS
     */
    void update(TickType_t nowTicks) override;
    
    /**
     * @brief Обрабатывает нажатие клавиши
     * 
     * Поддерживаемые клавиши:
     * - Печатные символы: вставка в позицию курсора
     * - Backspace/Delete: удаление символов
     * - Влево/Вправо: перемещение курсора
     * - Home/End: переход к началу/концу текста
     * 
     * @param key Код клавиши
     * @return true если клавиша обработана
     */
    bool onKey(uint8_t key) override;
    
    /**
     * @brief Обрабатывает состояние клавиатуры (для расширенных возможностей)
     * @param kb Структура состояния клавиатуры
     * @return true если событие обработано
     */
    bool onKey(const KBState &kb) override;

    /**
     * @brief Устанавливает текст поля
     * @param text Новый текст (обрезается до maxLength)
     */
    void setText(const char *text);
    
    /**
     * @brief Получает текущий текст поля
     * @return Указатель на C-строку с текстом
     */
    const char *getText() const { return textBuffer; }

    /**
     * @brief Устанавливает максимальную длину текста
     * @param value Максимальная длина (не более MAX_TEXT_LEN)
     */
    void setMaxLength(uint8_t value);
    
    /**
     * @brief Получает максимальную длину текста
     * @return Максимальная длина
     */
    uint8_t getMaxLength() const { return maxLength; }

    /**
     * @brief Устанавливает индекс шрифта для отрисовки
     * @param index Индекс шрифта (0 или 1)
     */
    void setFontIndex(uint8_t index);
    
    /**
     * @brief Получает индекс используемого шрифта
     * @return Индекс шрифта
     */
    uint8_t getFontIndex() const { return fontIndex; }

protected:
    /**
     * @brief Вызывается при изменении фокуса
     * 
     * Сбрасывает состояние мигания курсора при получении фокуса.
     */
    void onFocusChanged() override;

    /**
     * @brief Проверяет, можно ли вставить символ в указанную позицию
     * 
     * Виртуальный метод для переопределения в наследниках (например, DateEdit
     * разрешает только цифры в определённых позициях).
     * 
     * @param index Позиция для вставки (0-based)
     * @param ch Символ для вставки
     * @return true если символ можно вставить
     */
    virtual bool canInsertCharAt(uint8_t index, char ch) const;
    
    /**
     * @brief Определяет позицию курсора после вставки символа
     * 
     * Позволяет наследникам пропускать разделители (например, DateEdit
     * перепрыгивает через точки).
     * 
     * @param index Позиция, куда был вставлен символ
     * @return Новая позиция курсора
     */
    virtual uint8_t getNextCursorPosAfterInsert(uint8_t index) const { return index + 1; }
    
    /**
     * @brief Определяет, перезаписывается ли символ на указанной позиции
     * 
     * Используется в наследниках для реализации режима замены
     * (например, DateEdit всегда заменяет символы).
     * 
     * @param index Позиция символа
     * @return true если символ должен быть заменён, а не вставлен
     */
    virtual bool overwritesCharAt(uint8_t index) const
    {
        (void)index;
        return false;
    }
    
    /**
     * @brief Определяет позицию курсора после перемещения
     * 
     * Позволяет наследникам контролировать навигацию курсора
     * (например, пропускать разделители).
     * 
     * @param currentPos Текущая позиция курсора
     * @param delta Направление перемещения (-1 влево, +1 вправо)
     * @return Новая позиция курсора
     */
    virtual uint8_t getCursorPosAfterMove(uint8_t currentPos, int8_t delta) const;

    /**
     * @brief Обновляет видимость курсора на основе времени (мигание)
     * @param nowTicks Текущее время в тиках
     */
    void updateCursorVisibility(TickType_t nowTicks);
    
    /**
     * @brief Перерисовывает содержимое поля (без рамки)
     */
    void redrawContent();
    
    /**
     * @brief Вставляет символ в позицию курсора
     * @param ch Символ для вставки
     * @return true если символ вставлен успешно
     */
    bool insertChar(char ch);
    
    /**
     * @brief Удаляет символ на позиции курсора или перед ним
     * @param backspace true — удалить символ перед курсором (Backspace),
     *                  false — удалить символ под курсором (Delete)
     */
    void deleteCharAtCursor(bool backspace);
    
    /**
     * @brief Перемещает курсор на указанное количество позиций
     * @param delta Смещение (-1 влево, +1 вправо)
     */
    void moveCursor(int8_t delta);
    
    /**
     * @brief Отрисовывает курсор на экране
     */
    void drawCursor();
    
    /**
     * @brief Вычисляет максимальное количество видимых символов
     * @return Количество символов, помещающихся в поле
     */
    uint8_t getMaxVisibleChars() const;
    
    /**
     * @brief Корректирует scrollOffset, чтобы курсор был виден
     */
    void adjustScrollOffset();

private:
    static const uint8_t MAX_TEXT_LEN = 32;  ///< Максимальная длина буфера текста
    
    char textBuffer[MAX_TEXT_LEN + 1];  ///< Буфер текста с завершающим '\0'
    uint8_t maxLength;                  ///< Максимальная разрешённая длина текста
    uint8_t fontIndex;                  ///< Индекс используемого шрифта
    uint8_t cursorPos;                  ///< Позиция курсора в тексте (0-based)
    uint8_t scrollOffset;               ///< Количество символов, скрытых слева при прокрутке
    TickType_t lastBlinkTime;           ///< Время последнего переключения курсора
    TickType_t cursorOnDurationTicks;   ///< Длительность видимой фазы мигания
    TickType_t cursorOffDurationTicks;  ///< Длительность невидимой фазы мигания
};

/**
 * @brief Алиас для класса Edit (для обратной совместимости)
 */
using TextBox = Edit;

#endif // EDIT_H
