#ifndef UI_ARRAYLIST_H
#define UI_ARRAYLIST_H

#include "List.h"
#include <avr/pgmspace.h>

#define ARRAYLIST_SOURCE_NONE 0     ///< Источник данных не установлен
#define ARRAYLIST_SOURCE_RAM 1      ///< Данные в оперативной памяти
#define ARRAYLIST_SOURCE_PROGMEM 2  ///< Данные в программной памяти (Flash)

/**
 * @brief Список с данными из массива строк (RAM или PROGMEM)
 * 
 * Расширяет базовый класс List, добавляя поддержку отображения элементов
 * из массива строк. Массив может находиться как в оперативной памяти (RAM),
 * так и в программной памяти (PROGMEM/Flash) — это позволяет экономить SRAM
 * для статических списков.
 * 
 * Используется для отображения меню, списков опций и других предопределённых наборов строк.
 * 
 * @code
 * // Массив в PROGMEM
 * const char item1[] PROGMEM = "Пункт 1";
 * const char item2[] PROGMEM = "Пункт 2";
 * PGM_P const menuItems[] PROGMEM = { item1, item2 };
 * 
 * ArrayList *list = form->addArrayList(10, 10, 100, 40);
 * list->setProgmemItems(menuItems, 2);
 * @endcode
 */
class ArrayList : public List
{
public:
    /**
     * @brief Конструктор списка массива
     * @param left X-координата левого верхнего угла
     * @param top Y-координата левого верхнего угла
     * @param width Ширина списка в пикселях
     * @param height Высота списка в пикселях
     */
    ArrayList(uint8_t left, uint8_t top, uint8_t width, uint8_t height);

    /**
     * @brief Устанавливает источник данных как массив строк из RAM
     * 
     * Элементы списка будут читаться напрямую из указателей в оперативной памяти.
     * 
     * @param items Массив указателей на C-строки в RAM
     * @param count Количество элементов в массиве
     */
    void setRamItems(const char *const *items, uint16_t count);
    
    /**
     * @brief Устанавливает источник данных как массив строк из PROGMEM
     * 
     * Элементы списка будут читаться из Flash памяти с использованием pgm_read_ptr()
     * и strncpy_P(). Это экономит SRAM для статических списков.
     * 
     * @param items Массив указателей PGM_P на строки в PROGMEM
     * @param count Количество элементов в массиве
     */
    void setProgmemItems(PGM_P const *items, uint16_t count);

protected:
    /**
     * @brief Получает текст элемента по индексу из массива
     * 
     * Автоматически определяет, откуда читать данные (RAM или PROGMEM)
     * и использует соответствующие методы доступа.
     * 
     * @param index Индекс элемента в массиве
     * @param[out] buffer Буфер для записи текста
     * @param bufferSize Размер буфера
     * @return true если элемент прочитан успешно
     */
    bool getItemText(uint16_t index, char *buffer, uint8_t bufferSize) override;

private:
    const void *items;  ///< Указатель на массив элементов (типизация зависит от source)
    uint8_t source;     ///< Тип источника данных (ARRAYLIST_SOURCE_RAM или ARRAYLIST_SOURCE_PROGMEM)
};

#endif
