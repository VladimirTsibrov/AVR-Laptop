#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include "../UIElement.h"

/**
 * @brief ProgressBar — индикатор прогресса операции
 * 
 * Отображает прямоугольную шкалу с заполненной частью, пропорциональной
 * текущему значению относительно диапазона [minValue..maxValue].
 * 
 * Особенности:
 * - Настраиваемый диапазон значений (по умолчанию 0..100)
 * - Автоматическая конвертация значения в проценты
 * - Декоративная штриховка заполненной области (вертикальные линии каждые 8 пикселей)
 * - Минимальный размер 3×3 пикселя, рекомендуемый — не менее 10×8
 * - Элемент не получает фокус (isFocusable = false)
 * - Не реагирует на клавиатуру (только программное управление)
 * - Флаг FLAG_SHOW_PERCENTS зарезервирован для будущей функции отображения процентов
 * 
 * @code
 * ProgressBar* progress = new ProgressBar(10, 10, 80, 10, 0, 100);
 * form->addElement(progress);
 * 
 * // Обновление прогресса
 * for (uint8_t i = 0; i <= 100; i++) {
 *     progress->setValue(i);
 *     delay(50);
 * }
 * @endcode
 * 
 * @see UIElement
 */
class ProgressBar : public UIElement
{
public:
    static const uint8_t FLAG_SHOW_PERCENTS = 0x10; ///< Флаг отображения процентов (зарезервирован, не используется)

    /**
     * @brief Создаёт ProgressBar с заданным диапазоном значений
     * @param left Левая координата X
     * @param top Верхняя координата Y
     * @param width Ширина прогресс-бара (минимум 3 пикселя, рекомендуется ≥20)
     * @param height Высота прогресс-бара (минимум 3 пикселя, рекомендуется ≥8)
     * @param minValue Минимальное значение диапазона (по умолчанию 0)
     * @param maxValue Максимальное значение диапазона (по умолчанию 100)
     * 
     * Начальное значение устанавливается равным minValue.
     * Флаг FLAG_SHOW_PERCENTS включён по умолчанию (для будущего использования).
     */
    ProgressBar(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
                uint8_t minValue = 0, uint8_t maxValue = 100);

    /**
     * @brief Отрисовывает прогресс-бар на экране
     * 
     * Рисует внешнюю рамку и заполненную область внутри, пропорциональную
     * текущему значению. Заполнение украшено вертикальными штрихами
     * каждые 8 пикселей (modeInvert).
     * 
     * Формула: percent = (value - minValue) / (maxValue - minValue) * 100
     * Заполнение: fillWidth = innerWidth * percent / 100
     */
    void draw() override;
    
    /**
     * @brief Обработчик клавиш (не используется)
     * @param key Код нажатой клавиши
     * @return Всегда false (ProgressBar не реагирует на клавиатуру)
     */
    bool onKey(uint8_t key) override;
    
    /**
     * @brief Проверяет, может ли элемент получить фокус
     * @return Всегда false (ProgressBar не получает фокус)
     */
    bool isFocusable() const override;

    /**
     * @brief Устанавливает текущее значение прогресса
     * @param value Новое значение (автоматически ограничивается диапазоном [minValue..maxValue])
     * 
     * Если value < minValue, устанавливается minValue.
     * Если value > maxValue, устанавливается maxValue.
     * После изменения элемент автоматически перерисовывается.
     */
    void setValue(uint8_t value);
    
    /**
     * @brief Возвращает текущее значение прогресса
     * @return Текущее значение в диапазоне [minValue..maxValue]
     */
    uint8_t getValue() const { return value; }

    /**
     * @brief Устанавливает новый диапазон значений
     * @param minValue Новое минимальное значение
     * @param maxValue Новое максимальное значение
     * 
     * Если текущее value выходит за новые границы, оно корректируется:
     * - Если value < minValue, устанавливается minValue
     * - Если value > maxValue, устанавливается maxValue
     * 
     * После изменения элемент автоматически перерисовывается.
     */
    void setRange(uint8_t minValue, uint8_t maxValue);
    
    /**
     * @brief Возвращает минимальное значение диапазона
     * @return Минимальное значение
     */
    uint8_t getMinValue() const { return minValue; }
    
    /**
     * @brief Возвращает максимальное значение диапазона
     * @return Максимальное значение
     */
    uint8_t getMaxValue() const { return maxValue; }

private:
    uint8_t minValue;  ///< Минимальное значение диапазона
    uint8_t maxValue;  ///< Максимальное значение диапазона
    uint8_t value;     ///< Текущее значение прогресса
};

#endif // PROGRESSBAR_H
