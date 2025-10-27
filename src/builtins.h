/*
 * builtins.h – Definiciones de comandos internos del shell WISH
 * Autor: José Alfredo Martínez Valdés
 */

#ifndef BUILTINS_H
#define BUILTINS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATHS 100

typedef struct {
    char *dirs[MAX_PATHS];
    int count;
} PathList;

/* Prototipos de funciones */
void print_error(void);
void init_path(PathList *plist);
void builtin_cd(char **args);
void builtin_path(char **args, PathList *plist);

#endif
