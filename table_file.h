#ifndef TABLE_FILE_H
#define TABLE_FILE_H

#include <stdio.h>

#define TMF_OK            0 // успешно
#define TMF_ERR_OPEN      1 // ошибка открытия/создания файла
#define TMF_ERR_WRITE     2 // ошибка записи в файл
#define TMF_ERR_READ      3 // ошибка чтения/записи файла
#define TMF_ERR_INVALID   4 // неверные параметры (ключ <= 0, info == NULL и т.д.)
#define TMF_ERR_NOT_FOUND 5 // запись с таким ключом не найдена

typedef struct {
    int   busy;     // 0 – свободно, 1 – запись существует
    int   key;      // ключ элемента, != 0
    int   par;      // ключ родителя: 0 или существующий ключ
    long  offset;   // смещение данных info в файле
    int   length;   // длина данных info в байтах (без '\0')
} FItem;

typedef struct {
    FItem *records; // массив метаданных длины size
    int    size;    // максимальный размер таблицы
    int    count;   // текущее число занятых записей
    FILE  *f;       // файловый дескриптор
    char   *fname;  // имя файла (для записи при закрытии)
} FTable;           // ft - имя переменной, указывающей на структуру FTable

/*
 * Открыть (или создать) файл и считать metadata.
 * size — максимальное число записей.
 * Возвращает TMF_OK или код ошибки.
 */
int tf_open(FTable *ft, const char *filename, int size);

/*
 * Закрыть файл: записать count и metadata, освободить память.
 */
void tf_close(FTable *ft);

/*
 * Вставить новый элемент: дописать info в конец файла,
 * добавить/обновить FItem в памяти.
 * Возвращает TMF_OK или код ошибки.
 */
int tf_insert(FTable *ft, int key, int par, const char *info);

/*
 * Пометить busy=0 для записи с данным key и всех её потомков.
 * Возвращает TMF_OK или код ошибки.
 */
int tf_remove(FTable *ft, int key);

/*
 * Найти все FItem с par == заданному.
 * out_count — по адресу вернуть число найденных записей.
 * Возвращает динамический массив FItem* или NULL, если ничего не найдено.
 */
FItem *tf_search(FTable *ft, int par, int *out_count);

/*
 * Вывести в stdout все busy=1 записи:
 * для каждой — metadata и содержимое info.
 */
void tf_print(FTable *ft);

/*
 * Преобразует код ошибки файловой таблицы в человекочитаемую строку.
 */
const char *tf_errstr(int code);

void tf_export_dot(const FTable *ft, const char *filename);

#endif // TABLE_FILE_H
