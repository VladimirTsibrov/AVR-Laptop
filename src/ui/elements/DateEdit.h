#ifndef DATEEDIT_H
#define DATEEDIT_H

#include "TextBox.h"
#include "../../DS3231.h"

/**
 * @brief Специализированное поле ввода даты в формате "ДД.ММ.ГГГГ"
 * 
 * Расширяет Edit для удобного ввода дат с автоматическим форматированием.
 * Особенности:
 * - Фиксированный формат: "12.03.2024"
 * - Автоматическая вставка разделителей (точек)
 * - Курсор перепрыгивает через разделители
 * - Валидация: только цифры, проверка дней в месяце, високосные годы
 * - Режим замены символов (не вставки)
 * 
 * Используется в формах для ввода дат рождения, событий и т.д.
 */
class DateEdit : public Edit
{
public:
    /**
     * @brief Конструктор поля ввода даты
     * @param left X-координата левого верхнего угла
     * @param top Y-координата левого верхнего угла
     * @param width Ширина поля в пикселях
     * @param height Высота поля в пикселях
     * @param value Начальное значение даты (по умолчанию текущая дата из RTC)
     * @param fontIndex Индекс шрифта для отрисовки (по умолчанию 0)
     */
    DateEdit(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
             const RTCDate &value = RTCDate(), uint8_t fontIndex = 0);

    /**
     * @brief Устанавливает значение даты
     * 
     * Преобразует структуру RTCDate в строку формата "ДД.ММ.ГГГГ" и
     * отображает в поле.
     * 
     * @param value Структура с датой
     */
    void setValue(const RTCDate &value);
    
    /**
     * @brief Получает введённую дату
     * 
     * Парсит текст из поля и возвращает структуру RTCDate.
     * Если формат некорректен, возвращает дату по умолчанию (01.01.2000).
     * 
     * @return Структура RTCDate с введённой датой
     */
    RTCDate getValue() const;
    
    /**
     * @brief Проверяет корректность введённой даты
     * 
     * Проверяет формат, валидность чисел (день 1-31, месяц 1-12, год 2000-2099)
     * и соответствие дня количеству дней в месяце.
     * 
     * @return true если дата корректна
     */
    bool isValid() const;

protected:
    /**
     * @brief Проверяет, можно ли вставить символ в указанную позицию
     * 
     * Разрешены только цифры (0-9) в позициях, не занятых разделителями.
     * Позиции 2 и 5 зарезервированы под точки.
     * 
     * @param index Позиция для вставки
     * @param ch Символ для вставки
     * @return true если символ разрешён в этой позиции
     */
    bool canInsertCharAt(uint8_t index, char ch) const override;
    
    /**
     * @brief Определяет позицию курсора после вставки
     * 
     * Автоматически перепрыгивает через точки-разделители (позиции 2 и 5).
     * 
     * @param index Позиция, куда был вставлен символ
     * @return Новая позиция курсора (с учётом пропуска разделителей)
     */
    uint8_t getNextCursorPosAfterInsert(uint8_t index) const override;
    
    /**
     * @brief Определяет, заменяется ли символ при вводе
     * 
     * DateEdit работает в режиме замены — символы перезаписываются, не сдвигаются.
     * 
     * @param index Позиция символа
     * @return true (всегда режим замены)
     */
    bool overwritesCharAt(uint8_t index) const override;
    
    /**
     * @brief Определяет позицию курсора после перемещения
     * 
     * Пропускает разделители (точки) при навигации стрелками.
     * 
     * @param currentPos Текущая позиция курсора
     * @param delta Направление перемещения (-1 влево, +1 вправо)
     * @return Новая позиция курсора
     */
    uint8_t getCursorPosAfterMove(uint8_t currentPos, int8_t delta) const override;

public:
    /**
     * @brief Проверяет, является ли год високосным
     * @param year Год (например, 2024)
     * @return true если год високосный
     */
    static bool isLeapYear(uint16_t year);
    
    /**
     * @brief Возвращает количество дней в месяце
     * @param month Номер месяца (1-12)
     * @param year Год (для определения високосности февраля)
     * @return Количество дней (28-31)
     */
    static uint8_t daysInMonth(uint8_t month, uint16_t year);
    
    /**
     * @brief Проверяет корректность компонентов даты
     * @param day День (1-31)
     * @param month Месяц (1-12)
     * @param year Год (2000-2099)
     * @return true если дата валидна (с учётом дней в месяце)
     */
    static bool isValidDateParts(uint8_t day, uint8_t month, uint16_t year);
    
    /**
     * @brief Парсит текст даты в структуру RTCDate
     * 
     * Принимает строку формата "ДД.ММ.ГГГГ" или частично заполненную
     * (при allowPartial=true).
     * 
     * @param text Строка для парсинга
     * @param[out] value Структура для записи результата
     * @param allowPartial true — разрешить частично заполненные даты
     * @return true если парсинг успешен
     */
    static bool parseDateText(const char *text, RTCDate &value, bool allowPartial);
};

#endif // DATEEDIT_H
