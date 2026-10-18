#ifndef TIMEEDIT_H
#define TIMEEDIT_H

#include "TextBox.h"
#include "../../DS3231.h"

/**
 * @brief Специализированное поле ввода времени в формате "ЧЧ:ММ:СС"
 * 
 * Расширяет Edit для удобного ввода времени с автоматическим форматированием.
 * Особенности:
 * - Фиксированный формат: "23:59:45"
 * - Автоматическая вставка разделителей (двоеточий)
 * - Курсор перепрыгивает через разделители
 * - Валидация: только цифры, часы 00-23, минуты и секунды 00-59
 * - Режим замены символов (не вставки)
 * 
 * Используется в формах для установки времени, будильников, таймеров и т.д.
 */
class TimeEdit : public Edit
{
public:
    /**
     * @brief Конструктор поля ввода времени
     * @param left X-координата левого верхнего угла
     * @param top Y-координата левого верхнего угла
     * @param width Ширина поля в пикселях
     * @param height Высота поля в пикселях
     * @param value Начальное значение времени (по умолчанию текущее время из RTC)
     * @param fontIndex Индекс шрифта для отрисовки (по умолчанию 0)
     */
    TimeEdit(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
             const RTCTime &value = RTCTime(), uint8_t fontIndex = 0);

    /**
     * @brief Устанавливает значение времени
     * 
     * Преобразует структуру RTCTime в строку формата "ЧЧ:ММ:СС" и
     * отображает в поле.
     * 
     * @param value Структура с временем
     */
    void setValue(const RTCTime &value);
    
    /**
     * @brief Получает введённое время
     * 
     * Парсит текст из поля и возвращает структуру RTCTime.
     * Если формат некорректен, возвращает время по умолчанию (00:00:00).
     * 
     * @return Структура RTCTime с введённым временем
     */
    RTCTime getValue() const;
    
    /**
     * @brief Проверяет корректность введённого времени
     * 
     * Проверяет формат и валидность чисел (часы 00-23, минуты 00-59, секунды 00-59).
     * 
     * @return true если время корректно
     */
    bool isValid() const;

protected:
    /**
     * @brief Проверяет, можно ли вставить символ в указанную позицию
     * 
     * Разрешены только цифры (0-9) в позициях, не занятых разделителями.
     * Позиции 2 и 5 зарезервированы под двоеточия.
     * 
     * @param index Позиция для вставки
     * @param ch Символ для вставки
     * @return true если символ разрешён в этой позиции
     */
    bool canInsertCharAt(uint8_t index, char ch) const override;
    
    /**
     * @brief Определяет позицию курсора после вставки
     * 
     * Автоматически перепрыгивает через двоеточия-разделители (позиции 2 и 5).
     * 
     * @param index Позиция, куда был вставлен символ
     * @return Новая позиция курсора (с учётом пропуска разделителей)
     */
    uint8_t getNextCursorPosAfterInsert(uint8_t index) const override;
    
    /**
     * @brief Определяет, заменяется ли символ при вводе
     * 
     * TimeEdit работает в режиме замены — символы перезаписываются, не сдвигаются.
     * 
     * @param index Позиция символа
     * @return true (всегда режим замены)
     */
    bool overwritesCharAt(uint8_t index) const override;
    
    /**
     * @brief Определяет позицию курсора после перемещения
     * 
     * Пропускает разделители (двоеточия) при навигации стрелками.
     * 
     * @param currentPos Текущая позиция курсора
     * @param delta Направление перемещения (-1 влево, +1 вправо)
     * @return Новая позиция курсора
     */
    uint8_t getCursorPosAfterMove(uint8_t currentPos, int8_t delta) const override;

public:
    /**
     * @brief Проверяет корректность компонентов времени
     * @param hour Часы (0-23)
     * @param minute Минуты (0-59)
     * @param second Секунды (0-59)
     * @return true если время валидно
     */
    static bool isValidTimeParts(uint8_t hour, uint8_t minute, uint8_t second);
    
    /**
     * @brief Парсит текст времени в структуру RTCTime
     * 
     * Принимает строку формата "ЧЧ:ММ:СС" или частично заполненную
     * (при allowPartial=true).
     * 
     * @param text Строка для парсинга
     * @param[out] value Структура для записи результата
     * @param allowPartial true — разрешить частично заполненное время
     * @return true если парсинг успешен
     */
    static bool parseTimeText(const char *text, RTCTime &value, bool allowPartial);
};

#endif // TIMEEDIT_H
