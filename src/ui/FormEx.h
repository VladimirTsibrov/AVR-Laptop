#ifndef UI_FORM_EX_H
#define UI_FORM_EX_H

#include "Form.h"

/**
 * @brief Расширенная форма с произвольными размерами и позицией
 * 
 * В отличие от базового класса Form, который по умолчанию занимает весь экран,
 * FormEx позволяет создавать формы произвольного размера и расположения.
 * 
 * @code
 * FormEx *dialog = new FormEx(10, 10, 100, 40);  // Форма 100x40 в позиции (10, 10)
 * dialog->setDrawBorder(true);
 * dialog->setCaption(F("FormEx"));
 * @endcode
 */
class FormEx : public Form
{
public:
    /**
     * @brief Конструктор расширенной формы с заданными координатами и размерами
     * @param left X-координата левого верхнего угла формы
     * @param top Y-координата левого верхнего угла формы
     * @param width Ширина формы в пикселях
     * @param height Высота формы в пикселях
     */
    FormEx(uint8_t left, uint8_t top, uint8_t width, uint8_t height);

    /**
     * @brief Получает X-координату левого края формы
     * @return Координата X, заданная в конструкторе
     */
    virtual uint8_t formLeft() const override;
    
    /**
     * @brief Получает Y-координату верхнего края формы
     * @return Координата Y, заданная в конструкторе
     */
    virtual uint8_t formTop() const override;
    
    /**
     * @brief Получает ширину формы
     * @return Ширина в пикселях, заданная в конструкторе
     */
    virtual uint8_t formWidth() const override;
    
    /**
     * @brief Получает высоту формы
     * @return Высота в пикселях, заданная в конструкторе
     */
    virtual uint8_t formHeight() const override;

private:
    uint8_t left;    ///< X-координата формы
    uint8_t top;     ///< Y-координата формы
    uint8_t width;   ///< Ширина формы
    uint8_t height;  ///< Высота формы
};

#endif
