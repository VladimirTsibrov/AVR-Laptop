#ifndef UI_ELEMENTS_ALL_H
#define UI_ELEMENTS_ALL_H

/**
 * @file All.h
 * @brief Сборный заголовочный файл всех UI элементов
 * 
 * Подключает все доступные элементы UI системы minibook2560 для удобства.
 * Вместо многочисленных #include можно использовать один #include "ui/elements/All.h"
 * 
 * Включает:
 * - Базовые классы: Form, FormEx
 * - Кнопки: Button, IconButton, CheckBox
 * - Индикаторы: ProgressBar, Label
 * - Поля ввода: TextBox, DateEdit, TimeEdit
 * - Текстовый редактор: Memo
 * - Списки: List, ArrayList, Menu, FileList
 * 
 * @see UIElement, Form
 */

#include "../FormEx.h"
#include "Button.h"
#include "IconButton.h"
#include "CheckBox.h"
#include "ProgressBar.h"
#include "Label.h"
#include "TextBox.h"
#include "DateEdit.h"
#include "TimeEdit.h"
//#include "Memo.h"
#include "List.h"
#include "ArrayList.h"
#include "Menu.h"
#include "FileList.h"

#endif
