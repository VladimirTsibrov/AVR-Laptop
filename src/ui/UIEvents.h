#ifndef UI_EVENTS_H
#define UI_EVENTS_H

/**
 * @file UIEvents.h
 * @brief Определения событий для UI элементов
 * 
 * Содержит константы событий, которые генерируют UI элементы
 * и обрабатывают формы через метод Form::onEvent().
 */

/**
 * @brief Событие клика/активации элемента (нажатие Enter на кнопке, выбор пункта меню)
 */
#define EVENT_CLICK         0x01

/**
 * @brief Событие получения фокуса элементом
 */
#define EVENT_FOCUS         0x02

/**
 * @brief Событие потери фокуса элементом
 */
#define EVENT_LOST_FOCUS    0x04

/**
 * @brief Событие изменения значения элемента (изменение текста, состояния чекбокса и т.д.)
 */
#define EVENT_VALUE_CHANGED 0x08

/**
 * @brief Событие нажатия клавиши (для кастомной обработки в onEvent)
 */
#define EVENT_KEY           0x10

/**
 * @brief Событие закрытия элемента (для меню, диалогов)
 */
#define EVENT_CLOSE         0x20

/**
 * @brief Алиас события клика для обратной совместимости с кнопками
 */
#define EVENT_BUTTON_CLICK  EVENT_CLICK

#endif // UI_EVENTS_H
