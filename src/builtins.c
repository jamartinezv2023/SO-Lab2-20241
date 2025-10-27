/*
 * builtins.c – Implementación de los comandos internos del shell WISH
 * Autor: José Alfredo Martínez Valdés
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "builtins.h"

/* Mensaje de error estándar */
void print_error(void) {
    const char *error_message = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

/* Inicializa la lista de directorios PATH */
void init_path(PathList *path_list) {
    path_list->count = 1;
    path_list->dirs[0] = strdup("/bin");
}

/* Implementación del comando interno cd */
void builtin_cd(char **args) {
    if (args[1] == NULL || chdir(args[1]) != 0) {
        print_error();
    }
}

/* Implementación del comando interno path */
void builtin_path(char **args, PathList *path_list) {
    // Liberar directorios previos
    for (int i = 0; i < path_list->count; i++) {
        free(path_list->dirs[i]);
    }

    path_list->count = 0;

    // Si no hay argumentos, vaciamos el path
    if (args[1] == NULL) return;

    // Agregar nuevos directorios
    for (int i = 1; args[i] != NULL; i++) {
        path_list->dirs[path_list->count++] = strdup(args[i]);
    }
}
