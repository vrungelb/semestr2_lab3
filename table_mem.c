#include "table_mem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdio.h>



void tm_init(Table *t, int SIZE) {
    t->items    = calloc(SIZE, sizeof(Item));
    t->capacity = SIZE;
    t->count    = 0;
}


int tm_insert(Table *t, int key, int par, const char *info) {
    if (key <= 0 || info == NULL) return TM_ERR_INVALID;
    if (t->count >= t->capacity) return TM_ERR_FULL;
    // проверка уникальности ключа
    for (int i = 0; i < t->capacity; i++) {
        if (t->items[i].busy && t->items[i].key == key)
            return TM_ERR_EXISTS;
    }
    // проверка валидности родителя
    if (par != 0) {
        int found = 0;
        for (int i = 0; i < t->capacity; i++) {
            if (t->items[i].busy && t->items[i].key == par) {
                found = 1;
                break;
            }
        }
        if (!found) return TM_ERR_INVALID;
    }
    // вставка в первую свободную ячейку
    for (int i = 0; i < t->capacity; i++) {
        if (!t->items[i].busy) {
            t->items[i].busy = 1;
            t->items[i].key  = key;
            t->items[i].par  = par;
            t->items[i].info = strdup(info);
            t->count++;
            return TM_OK;
        }
    }
    return TM_ERR_FULL;
}


static void remove_rec(Table *t, int key) {
    for (int i = 0; i < t->capacity; i++) {
        if (t->items[i].busy && t->items[i].par == key) {
            int child = t->items[i].key;
            remove_rec(t, child);
            free(t->items[i].info);
            t->items[i].busy = 0;
            t->count--;
        }
    }
}


int tm_remove(Table *t, int key) {
    for (int i = 0; i < t->capacity; i++) {
        if (t->items[i].busy && t->items[i].key == key) {
            remove_rec(t, key);
            free(t->items[i].info);
            t->items[i].busy = 0;
            t->count--;
            return TM_OK;
        }
    }
    return TM_ERR_NOT_FOUND;
}


Table* tm_search(const Table *t, int par) {
    Table *res = malloc(sizeof(Table));
    res->capacity = t->capacity;
    res->count    = 0;
    res->items    = calloc(res->capacity, sizeof(Item));
    for (int i = 0; i < t->capacity; i++) {
        if (t->items[i].busy && t->items[i].par == par) {
            res->items[res->count].busy = 1;
            res->items[res->count].key  = t->items[i].key;
            res->items[res->count].par  = par;
            res->items[res->count].info = strdup(t->items[i].info);
            res->count++;
        }
    }
    return res;
}


void tm_print(const Table *t) {
    printf("Table (count=%d):\n", t->count);
    for (int i = 0; i < t->capacity; i++) {
        if (t->items[i].busy) {
            printf(" key=%d par=%d info='%s'\n",
                   t->items[i].key, t->items[i].par,
                   t->items[i].info);
        }
    }
}


void tm_free(Table *t) {
    for (int i = 0; i < t->capacity; i++) {
        if (t->items[i].busy) free(t->items[i].info);
    }
    free(t->items);
    t->items    = NULL;
    t->capacity = 0;
    t->count    = 0;
}


const char* tm_errstr(int code) {
    switch (code) {
        case TM_OK:                 return "OK!";
        case TM_ERR_FULL:           return "Table is full";
        case TM_ERR_NOT_FOUND:      return "No record with this key was found";
        case TM_ERR_INVALID:        return "Parent key should be >= 0\n"
                                           "Key should be unique and > 0\n"
                                           "Info should not be NULL and must be a C-string only";
        default:                    return "Unknown error";
    }
}


void tm_export_dot(const Table *t, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    fprintf(f, "digraph G {\n");
    for (int i = 0; i < t->capacity; i++) {
        if (!t->items[i].busy) continue;
        int k = t->items[i].key;
        fprintf(f, "    \"%d\" [label=\"%d: %s\"];\n",
                k, k, t->items[i].info);
        if (t->items[i].par != 0) {
            fprintf(f, "    \"%d\" -> \"%d\";\n",
                    t->items[i].par, k);
        }
    }
    fprintf(f, "}\n");
    fclose(f);
}
