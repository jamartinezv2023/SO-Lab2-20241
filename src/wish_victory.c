/*
 * wish_victory.c — Shell WISH “todo en uno” listo para tests
 * Cumple: built-ins (exit, cd, path), ejecución externa, redirección (>),
 * paralelismo (&), modo interactivo/batch, mensaje único de error.
 * No usa system(). Implementa execv + fork + wait. PATH por defecto: /bin
 */

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>


#define MAX_TOKENS  128
#define MAX_PATHS   128

static const char ERRMSG[] = "An error has occurred\n";

/* -------- Utilidades de error -------- */
static void print_error(void) {
    write(STDERR_FILENO, ERRMSG, strlen(ERRMSG));
}

/* -------- PATH del shell -------- */
typedef struct {
    char *dirs[MAX_PATHS];
    int count;
} PathList;

static void path_init(PathList *pl) {
    pl->count = 1;
    pl->dirs[0] = strdup("/bin");
}

static void path_set(PathList *pl, char **args) {
    // liberar anteriores
    for (int i = 0; i < pl->count; i++) {
        free(pl->dirs[i]);
    }
    pl->count = 0;
    // args[0] = "path", desde args[1] vienen los directorios (0 o más)
    for (int i = 1; args[i] != NULL && pl->count < MAX_PATHS; i++) {
        pl->dirs[pl->count++] = strdup(args[i]);
    }
}

/* -------- Tokenizado general -------- */
static int split_whitespace(char *s, char **tokens, int max) {
    int n = 0;
    char *save = NULL;
    for (char *tok = strtok_r(s, " \t\r\n", &save);
         tok != NULL && n < max - 1;
         tok = strtok_r(NULL, " \t\r\n", &save)) {
        tokens[n++] = tok;
    }
    tokens[n] = NULL;
    return n;
}

/* -------- Parseo de una “suborden” (entre &): args + redirección -------- */
typedef struct {
    char *argv[MAX_TOKENS];
    int argc;
    int has_redir;
    char *redir_file; // si has_redir = 1, debe existir exactamente 1 filename
} Cmd;

static int parse_command(char *segment, Cmd *cmd) {
    memset(cmd, 0, sizeof(*cmd));

    // 1) Validar que no haya más de un '>'
    int gt_count = 0;
    for (char *p = segment; *p; p++) if (*p == '>') gt_count++;
    if (gt_count > 1) return -1;

    // 2) Separar en "izquierda" (comando y args) y "derecha" (posible filename)
    char *right = NULL;
    if (gt_count == 1) {
        right = strchr(segment, '>');
        *right = '\0';
        right++;
    }

    // 3) Tokenizar la parte izquierda en argv
    cmd->argc = split_whitespace(segment, cmd->argv, MAX_TOKENS);
    if (cmd->argc == 0) {
        // Suborden vacía (p.ej., " & ls"): la ignoramos como no válida
        return 0;
    }

    // 4) Si hay redirección, la parte derecha debe tener EXACTAMENTE un token
    if (gt_count == 1) {
        cmd->has_redir = 1;
        if (!right) return -1;
        char *tmp = strdup(right);
        char *toks[4];
        int m = split_whitespace(tmp, toks, 4);
        if (m != 1) { free(tmp); return -1; }
        cmd->redir_file = strdup(toks[0]);
        free(tmp);
    }

    return 1; // OK
}

/* -------- Built-ins -------- */
static int is_builtin(const char *cmd) {
    return (!strcmp(cmd, "exit") || !strcmp(cmd, "cd") || !strcmp(cmd, "path"));
}

static int builtin_exit(char **argv) {
    // exit no acepta argumentos
    if (argv[1] != NULL) {
        print_error();
        return 0; // no salir
    }
    exit(0);
}

static void builtin_cd(char **argv) {
    // cd acepta exactamente 1 argumento
    if (argv[1] == NULL || argv[2] != NULL) {
        print_error();
        return;
    }
    if (chdir(argv[1]) != 0) {
        print_error();
    }
}

static void builtin_path(char **argv, PathList *pl) {
    // path sobrescribe el PATH con 0 o más directorios
    path_set(pl, argv);
}

/* -------- Ejecución de externos -------- */
static void exec_external(Cmd *cmd, PathList *pl) {
    if (pl->count == 0) {
        // PATH vacío → ningún programa externo debe ejecutarse
        print_error();
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return;
    }

    if (pid == 0) {
        // Hijo
        int redir_fd = -1;
        if (cmd->has_redir) {
            redir_fd = open(cmd->redir_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (redir_fd < 0) { print_error(); exit(1); }
            if (dup2(redir_fd, STDOUT_FILENO) < 0) { print_error(); exit(1); }
            if (dup2(redir_fd, STDERR_FILENO) < 0) { print_error(); exit(1); }
            close(redir_fd);
        }

        // Buscar ejecutable en el PATH
        for (int i = 0; i < pl->count; i++) {
            char full[1024];
            snprintf(full, sizeof(full), "%s/%s", pl->dirs[i], cmd->argv[0]);
            if (access(full, X_OK) == 0) {
                execv(full, cmd->argv);
                // si execv retorna, hubo error al ejecutar
                print_error();
                exit(1);
            }
        }
        // No encontrado en ningún directorio
        print_error();
        exit(1);
    } else {
        // Padre: no espera aquí (para permitir paralelismo); espera al final de la línea
        // Guardar PID si se quisiera granularidad; aquí no hace falta, esperaremos a "cualquiera".
        // Usaremos un contador externo en la función que procesa la línea.
        // (La espera real se hace fuera, sobre todos los hijos lanzados).
    }
}

/* -------- Procesar una línea completa: paralelismo (&) -------- */
static void process_line(char *line, PathList *pl) {
    // Dividir por '&'
    char *save = NULL;
    char *seg = NULL;
    int launched = 0;

    for (seg = strsep(&line, "&"); seg != NULL; seg = strsep(&line, "&")) {
        // Hacer una copia editable (parse_command altera el string)
        char *copy = strdup(seg);

        Cmd cmd;
        int r = parse_command(copy, &cmd);
        if (r < 0) {
            print_error();
            free(copy);
            continue;
        }
        if (r == 0) { // suborden vacía: ignorar
            free(copy);
            continue;
        }

        // Built-ins (no soportamos redirección a built-ins)
        if (is_builtin(cmd.argv[0])) {
            if (cmd.has_redir) {
                print_error();
                free(copy);
                if (cmd.redir_file) free(cmd.redir_file);
                continue;
            }
            if (!strcmp(cmd.argv[0], "exit")) { builtin_exit(cmd.argv); /* no retorna */ }
            else if (!strcmp(cmd.argv[0], "cd")) { builtin_cd(cmd.argv); }
            else if (!strcmp(cmd.argv[0], "path")) { builtin_path(cmd.argv, pl); }
            free(copy);
            if (cmd.redir_file) free(cmd.redir_file);
            continue;
        }

        // Externos
        exec_external(&cmd, pl);
        launched++;

        free(copy);
        if (cmd.redir_file) free(cmd.redir_file);
    }

    // Esperar a que terminen todos los hijos lanzados para esta línea
    for (int i = 0; i < launched; i++) {
        wait(NULL);
    }
}

/* -------- main: modo interactivo / batch -------- */
int main(int argc, char *argv[]) {
    FILE *input = stdin;

    if (argc > 2) {
        print_error();
        exit(1);
    }
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            print_error();
            exit(1);
        }
    }

    PathList pl;
    path_init(&pl);

    char *line = NULL;
    size_t cap = 0;

    while (1) {
        if (input == stdin) {
            printf("wish> ");
            fflush(stdout);
        }
        ssize_t n = getline(&line, &cap, input);
        if (n == -1) break; // EOF → salir

        // Ignorar líneas vacías o solo espacios
        int only_ws = 1;
        for (ssize_t i = 0; i < n; i++) {
            if (line[i] != ' ' && line[i] != '\t' && line[i] != '\n' && line[i] != '\r')
            { only_ws = 0; break; }
        }
        if (only_ws) continue;

        // Procesar la línea (paralelismo incluido)
        process_line(line, &pl);
    }

    if (input != stdin) fclose(input);
    free(line);
    return 0;
}
