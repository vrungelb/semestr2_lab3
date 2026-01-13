#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table_mem.h"
#include "table_file.h"

#define SIZE 100

#define COLOR_RED    "\x1B[1;31m"
#define COLOR_RESET  "\x1B[0m"
#define COLOR_BLUE  "\x1B[1;34m"
#define COLOR_YELLOW "\x1B[1;33m"



int menu() {
    int cmd;
    printf(COLOR_YELLOW "\nChoose command:\n" COLOR_RESET);
    printf("1 - Insert\n");
    printf("2 - Remove\n");
    printf("3 - Search by parent key\n");
    printf("4 - Print all\n");
    printf("5 - Export to Graphviz DOT\n");
    printf(COLOR_BLUE "0 - Exit\n" COLOR_RESET);
    printf("> ");
    if (scanf("%d", &cmd) != 1) {
        printf(COLOR_RED "Invalid input. Please enter a number." COLOR_RESET "\n");
        while (getchar() != '\n');
        return -1;
    }
    return cmd;
}

int main(int argc, char *argv[]) {
    int mode;
    printf(COLOR_YELLOW "Select mode:\n" COLOR_RESET);
    printf("1 - Internal (Memory)\n");
    printf("2 - External (File)\n");
    printf("> ");
    if (scanf("%d", &mode) != 1 || (mode != 1 && mode != 2)) {
        printf(COLOR_RED "Wrong mode. Exiting." COLOR_RESET "\n");
        return 1;
    }

    Table  tmem;
    FTable tfile;

    if (mode == 1) {
        tm_init(&tmem, SIZE);
    } else {
        const char *fname = (argc > 1 ? argv[1] : "table.dat");
        int res = tf_open(&tfile, fname, SIZE);
        if (res != TMF_OK) {
            printf("File opening error '%s': " COLOR_RED "%s" COLOR_RESET "\n", fname, tf_errstr(res));
            return 1;
        }
    }

    while (1) {
        int cmd = menu();
        if (cmd < 0) continue;
        if (cmd == 0) break;

        int key, par, scanned, ret;
        char info[256];

        if (mode == 1) {
            switch (cmd) {
                case 1:
                    printf("Enter key, parent key, info: ");
                    scanned = scanf(" %d , %d , %255s", &key, &par, info);
                    if (scanned != 3) {
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tm_errstr(TMF_ERR_INVALID));
                        while (getchar() != '\n');
                        break;
                    }
                    ret = tm_insert(&tmem, key, par, info);
                    if (ret != TM_OK)
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tm_errstr(ret));
                    break;

                case 2:
                    printf("Enter key to remove: ");
                    if (scanf("%d", &key) != 1) {
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tm_errstr(TMF_ERR_INVALID));
                        while (getchar() != '\n');
                        break;
                    }
                    ret = tm_remove(&tmem, key);
                    if (ret != TM_OK)
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tm_errstr(ret));
                    break;

                case 3: {
                    printf("Enter parent key to search: ");
                    if (scanf("%d", &par) != 1) {
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tm_errstr(TMF_ERR_INVALID));
                        while (getchar() != '\n');
                        break;
                    }
                    Table *res = tm_search(&tmem, par);
                    tm_print(res);
                    tm_free(res);
                    free(res);
                    break;
                }

                case 4:
                    tm_print(&tmem);
                    break;

                case 5: {
                    char dotfile[256];
                    printf("Enter output DOT filename: ");
                    if (scanf(" %255s", dotfile) != 1) break;
                    if (mode == 1)
                        tm_export_dot(&tmem, dotfile);
                    else
                        tf_export_dot(&tfile, dotfile);
                    printf("DOT saved to '%s'\n", dotfile);
                    break;
                }

                default:
                    printf(COLOR_RED "%s" COLOR_RESET "\n", tm_errstr(TMF_ERR_INVALID));
            }

        } else {
            switch (cmd) {
                case 1:
                    printf("Enter key, parent key, info: ");
                    scanned = scanf(" %d , %d , %255s", &key, &par, info);
                    if (scanned != 3) {
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tf_errstr(TMF_ERR_INVALID));
                        while (getchar() != '\n');
                        break;
                    }
                    ret = tf_insert(&tfile, key, par, info);
                    if (ret != TMF_OK)
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tf_errstr(ret));
                    break;

                case 2:
                    printf("Enter key to remove: ");
                    if (scanf("%d", &key) != 1) {
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tf_errstr(TMF_ERR_INVALID));
                        while (getchar() != '\n');
                        break;
                    }
                    ret = tf_remove(&tfile, key);
                    if (ret != TMF_OK)
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tf_errstr(ret));
                    break;

                case 3: {
                    printf("Enter parent key to search: ");
                    if (scanf("%d", &par) != 1) {
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tf_errstr(TMF_ERR_INVALID));
                        while (getchar() != '\n');
                        break;
                    }
                    int out_cnt;
                    FItem *found = tf_search(&tfile, par, &out_cnt);
                    if (!found) {
                        printf(COLOR_RED "%s" COLOR_RESET "\n", tf_errstr(TMF_ERR_NOT_FOUND));
                    } else {
                        for (int i = 0; i < out_cnt; i++) {
                            printf("key=%d par=%d info=", found[i].key, found[i].par);
                            char *buf = malloc(found[i].length + 1);
                            fseek(tfile.f, found[i].offset, SEEK_SET);
                            fread(buf, 1, found[i].length, tfile.f);
                            buf[found[i].length] = '\0';
                            printf("%s\n", buf);
                            free(buf);
                        }
                        free(found);
                    }
                    break;
                }

                case 4:
                    tf_print(&tfile);
                    break;

                case 5: {
                    char dotfile[256];
                    printf("Enter output DOT filename: ");
                    if (scanf(" %255s", dotfile) != 1) break;
                    if (mode == 1)
                        tm_export_dot(&tmem, dotfile);
                    else
                        tf_export_dot(&tfile, dotfile);
                    printf("DOT saved to '%s'\n", dotfile);
                    break;
                }

                default:
                    printf(COLOR_RED "%s" COLOR_RESET "\n", tf_errstr(TMF_ERR_INVALID));
            }
        }
    }

    if (mode == 1) {
        tm_free(&tmem);
    } else {
        tf_close(&tfile);
    }

    return 0;
}
