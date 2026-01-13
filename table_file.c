#include "table_file.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int read_metadata(FTable *ft) {
    long meta_size = sizeof(int) + (long)ft->size * sizeof(FItem);

    if (fseek(ft->f, 0, SEEK_END) != 0) return TMF_ERR_READ;
    long file_size = ftell(ft->f);

    if (file_size < meta_size) {
        // initialize metadata for a new file
        ft->count = 0;
        for (int i = 0; i < ft->size; i++) ft->records[i].busy = 0;
        rewind(ft->f);
        // write initial state to disk
        if (fwrite(&ft->count, sizeof(int), 1, ft->f) != 1 ||
            fwrite(ft->records, sizeof(FItem), ft->size, ft->f) != (size_t)ft->size) {
            return TMF_ERR_WRITE;
        }
        fflush(ft->f);
    } else {
        // read existing index from file into memory
        rewind(ft->f);
        if (fread(&ft->count, sizeof(int), 1, ft->f) != 1 ||
            fread(ft->records, sizeof(FItem), ft->size, ft->f) != (size_t)ft->size) {
            return TMF_ERR_READ;
        }
    }
    return TMF_OK;
}

int tf_open(FTable *ft, const char *filename, int size) {
    ft->fname = strdup(filename); // allocate memory for filename
    if (!ft->fname) return TMF_ERR_OPEN;
    
    ft->size = size;
    ft->records = calloc(size, sizeof(FItem)); // allocate index array
    if (!ft->records) {
        free(ft->fname);
        return TMF_ERR_OPEN;
    }

    ft->f = fopen(filename, "r+b"); // try opening existing file
    if (!ft->f) {
        ft->f = fopen(filename, "w+b"); // create file if it doesn't exist
        if (!ft->f) {
            free(ft->records);
            free(ft->fname);
            return TMF_ERR_OPEN;
        }
    }
    return read_metadata(ft);
}

void tf_close(FTable *ft) {
    if (!ft || !ft->f) return;
    rewind(ft->f);
    // save final metadata state before closing
    fwrite(&ft->count, sizeof(int), 1, ft->f);
    fwrite(ft->records, sizeof(FItem), ft->size, ft->f);
    fflush(ft->f);
    fclose(ft->f);
    free(ft->records);
    free(ft->fname);
}

static void remove_recursive(FTable *ft, int key) {
    for (int i = 0; i < ft->size; i++) {
        if (ft->records[i].busy && ft->records[i].key == key) {
            ft->records[i].busy = 0; // mark record as inactive
            ft->count--;
            int deleted_key = ft->records[i].key;
            // recursively find and remove child nodes
            for (int j = 0; j < ft->size; j++) {
                if (ft->records[j].busy && ft->records[j].par == deleted_key) {
                    remove_recursive(ft, ft->records[j].key);
                }
            }
            break;
        }
    }
}

int tf_remove(FTable *ft, int key) {
    for (int i = 0; i < ft->size; i++) {
        if (ft->records[i].busy && ft->records[i].key == key) {
            remove_recursive(ft, key); // start deletion chain
            return TMF_OK;
        }
    }
    return TMF_ERR_NOT_FOUND;
}

int tf_insert(FTable *ft, int key, int par, const char *info) {
    if (key <= 0 || !info) return TMF_ERR_INVALID;

    int free_idx = -1;
    // verify key uniqueness and find empty index slot
    for (int i = 0; i < ft->size; i++) {
        if (ft->records[i].busy && ft->records[i].key == key) return TMF_ERR_INVALID;
        if (free_idx == -1 && !ft->records[i].busy) free_idx = i;
    }

    // ensure parent node exists
    if (par != 0) {
        int found = 0;
        for (int i = 0; i < ft->size; i++) {
            if (ft->records[i].busy && ft->records[i].key == par) { found = 1; break; }
        }
        if (!found) return TMF_ERR_INVALID;
    }

    if (free_idx < 0) return TMF_ERR_WRITE; // table capacity reached

    int len = (int)strlen(info) + 1;
    long off = -1;

    // check for available space from deleted records (fragmentation reuse)
    for (int i = 0; i < ft->size; i++) {
        if (!ft->records[i].busy && ft->records[i].offset > 0 && (ft->records[i].length + 1) >= len) {
            off = ft->records[i].offset;
            break;
        }
    }

    if (off == -1) {
        fseek(ft->f, 0, SEEK_END); // append to file if no reusable space found
        off = ftell(ft->f);
    } else {
        fseek(ft->f, off, SEEK_SET); // reuse existing space in file
    }

    if (fwrite(info, 1, len, ft->f) != (size_t)len) return TMF_ERR_WRITE;
    fflush(ft->f);

    // update index entry
    ft->records[free_idx].busy = 1;
    ft->records[free_idx].key = key;
    ft->records[free_idx].par = par;
    ft->records[free_idx].offset = off;
    ft->records[free_idx].length = len - 1;
    ft->count++;
    return TMF_OK;
}

FItem *tf_search(FTable *ft, int par, int *out_count) {
    int cnt = 0;
    // count children to allocate result array
    for (int i = 0; i < ft->size; i++) {
        if (ft->records[i].busy && ft->records[i].par == par) cnt++;
    }
    if (cnt == 0) { *out_count = 0; return NULL; }

    FItem *res = malloc(cnt * sizeof(FItem));
    if (!res) return NULL;
    // populate result array with found items
    for (int i = 0, j = 0; i < ft->size; i++) {
        if (ft->records[i].busy && ft->records[i].par == par) res[j++] = ft->records[i];
    }
    *out_count = cnt;
    return res;
}

void tf_print(FTable *ft) {
    for (int i = 0; i < ft->size; i++) {
        FItem *r = &ft->records[i];
        if (!r->busy) continue;
        char *buf = malloc(r->length + 1);
        if (!buf) continue;
        fseek(ft->f, r->offset, SEEK_SET); // jump to data location in file
        if (fread(buf, 1, r->length, ft->f) == (size_t)r->length) {
            buf[r->length] = '\0';
            printf("key=%d par=%d info=%s\n", r->key, r->par, buf);
        }
        free(buf);
    }
}

const char* tf_errstr(int code) {
    switch (code) {
        case TMF_OK: return "OK";
        case TMF_ERR_OPEN: return "File open error";
        case TMF_ERR_WRITE: return "File write error";
        case TMF_ERR_READ: return "File read error";
        case TMF_ERR_INVALID: return "Invalid input";
        case TMF_ERR_NOT_FOUND: return "Record not found";
        default: return "Unknown error";
    }
}

void tf_export_dot(const FTable *ft, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    fprintf(f, "digraph G {\n");
    for (int i = 0; i < ft->size; i++) {
        const FItem *r = &ft->records[i];
        if (!r->busy) continue;
        char *buf = malloc(r->length + 1);
        if (buf) {
            fseek(ft->f, r->offset, SEEK_SET);
            fread(buf, 1, r->length, ft->f);
            buf[r->length] = '\0';
            fprintf(f, "  \"%d\" [label=\"%d: %s\"];\n", r->key, r->key, buf);
            free(buf);
        }
        if (r->par != 0) fprintf(f, "  \"%d\" -> \"%d\";\n", r->par, r->key); // draw hierarchy edge
    }
    fprintf(f, "}\n");
    fclose(f);
}