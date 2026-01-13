#ifndef TABLE_MEM_H
#define TABLE_MEM_H

#include <stddef.h>

// Коды ошибок
#define TM_OK            0  // успешно
#define TM_ERR_EXISTS    1  // элемент с таким ключом уже существует
#define TM_ERR_FULL      2  // таблица заполнена
#define TM_ERR_NOT_FOUND 3  // элемент не найден
#define TM_ERR_INVALID   4  // неверные параметры

// Структура элемента в памяти
typedef struct {
    int   busy;    // признак занятости: 0 (свободно) / 1 (занято)
    int   key;     // ключ элемента, != 0
    int   par;     // ключ родителя: 0 или существующий ключ
    char *info;    // строка с информацией
} Item;

// Структура самой таблицы
typedef struct {
    Item *items;   // массив элементов
    int   capacity;// максимальное число элементов
    int   count;   // текущее число занятых элементов
} Table;

// Инициализация таблицы (выделение памяти)
// SIZE - максимальное число элементов
void tm_init(Table *t, int SIZE);

// Вставка нового элемента
// key   - ключ (> 0 и уникален)
// par   - ключ родителя (0 или существующий key)
// info  - C-строка (не NULL)
// Возвращает код ошибки TM_ERR_* или TM_OK
int tm_insert(Table *t, int key, int par, const char *info);

// Удаление элемента по ключу и рекурсивное удаление всех потомков
// Возвращает TM_OK или TM_ERR_NOT_FOUND
int tm_remove(Table *t, int key);

// Поиск всех элементов с заданным ключом родителя;
// Возвращает новый объект Table* с копиями найденных элементов
Table* tm_search(const Table *t, int par);

// Вывод всей таблицы в stdout
void tm_print(const Table *t);

// Освобождение всех ресурсов таблицы
void tm_free(Table *t);

/*
 * Преобразует код ошибки памяти в человекочитаемую строку.
 */
const char *tm_errstr(int code);

void tm_export_dot(const Table *t, const char *filename);

#endif // TABLE_MEM_H