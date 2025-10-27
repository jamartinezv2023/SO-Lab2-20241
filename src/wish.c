/*
 * wish.c – Shell básico tipo WISH (v2.0: redirección de salida)
 * - Comandos internos: exit, cd, path
 * - Ejecución de externos con fork/execv/wait
 * - Redirección '>' (stdout y stderr al MISMO archivo)
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "builtins.h"

#define MAX_INPUT 2048
#define MAX_ARGS  128

/* Mensaje de error estándar (función en builtins.c) */
extern void print_error(void);

/* Estructura para el resultado del parseo */
typedef struct {
    char *argv[MAX_ARGS];  // argumentos del comando (argv[0] = cmd)
    int   argc;
    int   has_redir;       // 1 si hay '>'
    char *redir_path;      // nombre del archivo de salida
} ParsedLine;

/* Añade un argumento si hay espacio */
static int push_arg(ParsedLine *pl, char *tok) {
    if (pl->argc >= MAX_ARGS - 1) return -1;
    pl->argv[pl->argc++] = tok;
    pl->argv[pl->argc] = NULL;
    return 0;
}

/* Extrae redirección desde un token que CONTIENE '>' (p. ej., "arg>out" o ">out") */
static int handle_token_with_gt(ParsedLine *pl, char *tok) {
    char *gt = strchr(tok, '>');
    if (!gt) return 0;

    if (pl->has_redir) return -1; // segundo '>' → error

    // Partes: izquierda (antes de '>') y derecha (después de '>')
    *gt = '\0';
    char *left  = tok;
    char *right = gt + 1;

    // Si hay algo a la izquierda, es un argumento normal
    if (*left != '\0') {
        if (push_arg(pl, left) < 0) return -1;
    }

    // Marcamos que hay redirección
    pl->has_redir = 1;

    // Si la parte derecha no está vacía, ahí viene el nombre del archivo
    if (*right != '\0') {
        pl->redir_path = right;
    } else {
        // Si está vacía, el nombre del archivo debe venir en el PRÓXIMO token
        pl->redir_path = NULL;
    }
    return 1; // indica que sí había '>'
}

/* Parser general: soporta '>', 'arg>out', '>out', 'arg > out' (un solo '>') */
static int parse_line(char *line, ParsedLine *pl) {
    memset(pl, 0, sizeof(*pl));

    // Tokenizamos por espacios/tab/nl
    char *saveptr = NULL;
    for (char *tok = strtok_r(line, " \t\r\n", &saveptr);
         tok != NULL;
         tok = strtok_r(NULL, " \t\r\n", &saveptr)) {

        // ¿El token contiene '>'?
        if (strchr(tok, '>') != NULL) {
            if (handle_token_with_gt(pl, tok) < 0) return -1;

            // Si la ruta no vino pegada (ej. "arg>"), esperamos próximo token
            if (pl->has_redir && pl->redir_path == NULL) {
                tok = strtok_r(NULL, " \t\r\n", &saveptr);
                if (!tok) return -1; // faltó el nombre del archivo
                if (strchr(tok, '>') != NULL) return -1; // no se permite ">>" ni más '>'
                pl->redir_path = tok;
            }
        } else {
            // Token normal (argumento del comando)
            if (push_arg(pl, tok) < 0) return -1;
        }
    }

    // Validaciones finales
    if (pl->argc == 0) return 0;                // línea vacía
    if (pl->has_redir && (pl->redir_path == NULL)) return -1; // '>' sin archivo

    return 1; // línea válida para ejecutar
}

/* Ejecuta un comando (interno o externo), con redirección si aplica */
static void ejecutar(ParsedLine *pl, PathList *path_list) {
    if (pl->argc == 0) return; // nada que ejecutar

    char *cmd = pl->argv[0];

    // Built-ins: (opcional) rechazamos redirección sobre built-ins
    if (strcmp(cmd, "exit") == 0) {
        if (pl->has_redir) { print_error(); return; }
        exit(0);
    } else if (strcmp(cmd, "cd") == 0) {
        if (pl->has_redir) { print_error(); return; }
        builtin_cd(pl->argv);
        return;
    } else if (strcmp(cmd, "path") == 0) {
        if (pl->has_redir) { print_error(); return; }
        builtin_path(pl->argv, path_list);
        return;
    }

    // Si no hay PATH configurado, no se puede ejecutar externo
    if (path_list->count == 0) { print_error(); return; }

    pid_t pid = fork();
    if (pid < 0) { print_error(); return; }

    if (pid == 0) {
        // Hijo
        int fd = -1;
        if (pl->has_redir) {
            fd = open(pl->redir_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { print_error(); exit(1); }
            if (dup2(fd, STDOUT_FILENO) < 0) { print_error(); exit(1); }
            if (dup2(fd, STDERR_FILENO) < 0) { print_error(); exit(1); }
            close(fd);
        }

        // Buscar el binario en cada directorio del PATH
        for (int i = 0; i < path_list->count; i++) {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s", path_list->dirs[i], cmd);
            if (access(path, X_OK) == 0) {
                execv(path, pl->argv);
                // Si retorna, hubo error de exec (improbable si access fue OK)
                print_error();
                exit(1);
            }
        }
        // No se encontró el ejecutable
        print_error();
        exit(1);
    } else {
        // Padre
        wait(NULL);
    }
}

int main(void) {
    PathList path_list;
    init_path(&path_list);

    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("wish> ");
        fflush(stdout);

        if (getline(&line, &len, stdin) == -1) break; // EOF

        // Parsear y ejecutar
        ParsedLine pl;
        int r = parse_line(line, &pl);
        if (r < 0) {
            print_error();      // error de sintaxis en redirección
            continue;
        } else if (r == 0) {
            continue;           // línea vacía
        }

        ejecutar(&pl, &path_list);
    }

    free(line);
    return 0;
}
