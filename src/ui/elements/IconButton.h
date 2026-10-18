#ifndef ICON_BUTTON_H
#define ICON_BUTTON_H

#include "../UIElement.h"

/**
 * @brief Кнопка с иконкой — элемент управления с графическим изображением
 * 
 * IconButton отображает растровую иконку в формате XBM вместо текста.
 * Иконка автоматически центрируется внутри кнопки и инвертируется при фокусе.
 * 
 * Особенности:
 * - Поддержка XBM-иконок (массив байтов в PROGMEM)
 * - Автоцентрирование иконки внутри кнопки
 * - Инверсия иконки при получении фокуса
 * - Минимальный размер кнопки 3×3 пикселя
 * - Удобный конструктор для квадратных кнопок
 * - Реакция на Enter и Space с генерацией EVENT_CLICK
 * 
 * @code
 * static const uint8_t playIcon[] PROGMEM = {
 *     0x00, 0x00, 0x08, 0x00, 0x18, 0x00, 0x38, 0x00,
 *     0x78, 0x00, 0x38, 0x00, 0x18, 0x00, 0x08, 0x00
 * };
 * IconButton* btnPlay = new IconButton(10, 10, playIcon, 8);
 * btnPlay->setEventHandler(EVENT_CLICK, []() { playMusic(); });
 * form->addElement(btnPlay);
 * @endcode
 * 
 * @see UIElement, Button
 */
class IconButton : public UIElement
{
public:
    /**
     * @brief Создаёт кнопку с иконкой произвольного размера
     * @param left Левая координата X
     * @param top Верхняя координата Y
     * @param width Ширина кнопки (минимум 3 пикселя)
     * @param height Высота кнопки (минимум 3 пикселя)
     * @param icon Указатель на XBM-иконку в PROGMEM (массив байтов)
     * @param iconWidth Ширина иконки в пикселях
     * @param iconHeight Высота иконки в пикселях
     * 
     * Если иконка меньше кнопки, она центрируется внутри.
     * Если больше — обрезается по границам кнопки.
     */
    IconButton(uint8_t left, uint8_t top, uint8_t width, uint8_t height,
               const uint8_t *icon, uint8_t iconWidth, uint8_t iconHeight);
    
    /**
     * @brief Создаёт квадратную кнопку с квадратной иконкой (удобный конструктор)
     * @param left Левая координата X
     * @param top Верхняя координата Y
     * @param icon Указатель на XBM-иконку в PROGMEM
     * @param iconSize Размер иконки и кнопки в пикселях
     * 
     * Автоматически добавляет отступ 1 пиксель по периметру иконки.
     * Размер кнопки будет iconSize + 2 пикселя.
     * 
     * @code
     * IconButton* btn = new IconButton(10, 10, myIcon, 16); // Кнопка 18×18
     * @endcode
     */
    IconButton(uint8_t left, uint8_t top, const uint8_t *icon, uint8_t iconSize);

    /**
     * @brief Отрисовывает кнопку с иконкой на экране
     * 
     * Рисует рамку (тонкую/жирную в зависимости от фокуса) и центрированную
     * иконку. При фокусе иконка инвертируется (modeInvert), иначе обычная (modeSet).
     */
    void draw() override;
    
    /**
     * @brief Обрабатывает нажатие клавиш
     * @param key Код нажатой клавиши
     * @return true если клавиша обработана (Enter/Space), false если игнорируется
     * 
     * Реагирует на keyEnter и keySpace, генерирует событие EVENT_CLICK.
     */
    bool onKey(uint8_t key) override;

    /**
     * @brief Возвращает указатель на текущую иконку
     * @return Указатель на массив байтов XBM-иконки в PROGMEM
     */
    const uint8_t *getIcon() const { return icon; }
    
    /**
     * @brief Возвращает ширину иконки
     * @return Ширина иконки в пикселях
     * @note Возвращает iconWidth, несмотря на название getIconSize
     */
    uint8_t getIconSize() const { return iconWidth; }
    
    /**
     * @brief Устанавливает новую иконку и перерисовывает кнопку
     * @param icon Указатель на новую XBM-иконку в PROGMEM
     * @param iconWidth Ширина новой иконки
     * @param iconHeight Высота новой иконки
     * 
     * Обновляет иконку и автоматически вызывает draw() для перерисовки.
     */
    void setIcon(const uint8_t *icon, uint8_t iconWidth, uint8_t iconHeight);

protected:
    /**
     * @brief Вызывается при изменении фокуса
     * 
     * Перерисовывает кнопку с новым состоянием фокуса (инверсия иконки
     * и жирная/тонкая рамка) и восстанавливает верхний слой UI.
     */
    void onFocusChanged() override;

private:
    const uint8_t *icon;    ///< Указатель на XBM-иконку в PROGMEM (массив байтов)
    uint8_t iconWidth;      ///< Ширина иконки в пикселях
    uint8_t iconHeight;     ///< Высота иконки в пикселях
};

#endif // ICON_BUTTON_H
