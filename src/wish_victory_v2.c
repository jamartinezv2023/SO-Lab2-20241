/*
 * wish_victory_v2.c — Shell WISH final para laboratorio
 * - Built-ins: exit, cd, path (validaciones de argumentos)
 * - PATH dinámico (inicial: /bin)
 * - Comandos externos con fork/execv + access(X_OK)
 * - Redirección '>' (stdout y stderr al MISMO archivo) — un único archivo
 * - Paralelismo '&' (lanza todos y luego wait() a todos)
 * - Modo interactivo (con prompt) y batch (sin prompt, usando argv[1])
 * - ÚNICO mensaje de error: "An error has occurred\n" a stderr
 * - Sin system(); usa getline(), strsep(), fork(), execv(), waitpid(), dup2(), open(), access()
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

#define MAX_PATHS   128
#define MAX_ARGS    128

static const char ERRMSG[] = "An error has occurred\n";

/* --------------------- Utilidades --------------------- */

static void print_error(void) {
    write(STDERR_FILENO, ERRMSG, strlen(ERRMSG));
}

/* Convierte una línea colocando espacios alrededor de '>' y '&'
   para permitir parseo robusto con o sin espacios en el input original. */
static char *normalize_ops(const char *line) {
    size_t n = strlen(line);
    /* Peor caso: cada char es un operador, necesitamos ~3x */
    size_t cap = n * 3 + 1;
    char *out = (char *)malloc(cap);
    if (!out) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
        char c = line[i];
        if (c == '>' || c == '&') {
            out[j++] = ' ';
            out[j++] = c;
            out[j++] = ' ';
        } else {
            out[j++] = c;
        }
        if (j + 4 >= cap) { /* ampliar por seguridad */
            cap *= 2;
            out = (char *)realloc(out, cap);
            if (!out) return NULL;
        }
    }
    out[j] = '\0';
    return out;
}

/* --------------------- PATH --------------------- */

typedef struct {
    char *dirs[MAX_PATHS];
    int count;
} PathList;

static void path_init(PathList *pl) {
    pl->count = 1;
    pl->dirs[0] = strdup("/bin");
}

static void path_clear(PathList *pl) {
    for (int i = 0; i < pl->count; i++) {
        free(pl->dirs[i]);
        pl->dirs[i] = NULL;
    }
    pl->count = 0;
}

/* path sobrescribe completamente la lista actual con 0..N argumentos */
static void path_set(PathList *pl, char **argv) {
    /* argv[0] = "path"; a partir de argv[1] vienen los directorios */
    path_clear(pl);
    for (int i = 1; argv[i] != NULL && pl->count < MAX_PATHS; i++) {
        pl->dirs[pl->count++] = strdup(argv[i]);
    }
}

/* --------------------- Parseo de subcomandos --------------------- */

typedef struct {
    char *argv[MAX_ARGS];  /* argumentos (argv[0] = comando) */
    int   argc;
    int   has_redir;
    char *redir_file;      /* nombre del archivo si has_redir */
} Cmd;

/* Tokeniza por espacios usando strtok_r */
static int split_ws(char *s, char **toks, int maxtoks) {
    int n = 0;
    char *save = NULL;
    for (char *tok = strtok_r(s, " \t\r\n", &save);
         tok != NULL && n < maxtoks - 1;
         tok = strtok_r(NULL, " \t\r\n", &save)) {
        toks[n++] = tok;
    }
    toks[n] = NULL;
    return n;
}

/* Parsea un "segmento" (una sub-orden separada por '&'):
   - Extrae argv[]
   - Maneja un único '>' con exactamente 1 filename
   Retorna:
   -1: error de sintaxis
    0: suborden vacía (ignorar)
    1: OK
*/
static int parse_command(char *segment, Cmd *cmd) {
    memset(cmd, 0, sizeof(*cmd));

    /* Contar '>' tras normalizar ayuda, pero aquí contamos directo */
    int gt_count = 0;
    for (char *p = segment; *p; p++) if (*p == '>') gt_count++;
    if (gt_count > 1) return -1;

    /* Separar izquierda/derecha por '>' si existe */
    char *right = NULL;
    if (gt_count == 1) {
        right = strchr(segment, '>');
        *right = '\0';
        right++;
    }

    /* Tokenizar izquierda (comando + args) */
    cmd->argc = split_ws(segment, cmd->argv, MAX_ARGS);
    if (cmd->argc == 0) return 0; /* suborden vacía */

    /* Si hay redirección, la parte derecha debe tener EXACTAMENTE un token */
    if (gt_count == 1) {
        cmd->has_redir = 1;
        if (!right) return -1;
        char *tmp = strdup(right);
        char *rtoks[4];
        int m = split_ws(tmp, rtoks, 4);
        if (m != 1) { free(tmp); return -1; }
        cmd->redir_file = strdup(rtoks[0]);
        free(tmp);
    }
    return 1;
}

static int is_builtin(const char *cmd) {
    return (!strcmp(cmd, "exit") || !strcmp(cmd, "cd") || !strcmp(cmd, "path"));
}

/* --------------------- Built-ins --------------------- */

static int builtin_exit(char **argv) {
    /* exit no acepta argumentos */
    if (argv[1] != NULL) {
        print_error();
        return 0; /* no salir */
    }
    exit(0);
}

static void builtin_cd(char **argv) {
    /* cd acepta exactamente 1 argumento */
    if (argv[1] == NULL || argv[2] != NULL) {
        print_error();
        return;
    }
    if (chdir(argv[1]) != 0) {
        print_error();
    }
}

static void builtin_path(char **argv, PathList *pl) {
    path_set(pl, argv);
}

/* --------------------- Ejecución de externos --------------------- */

static pid_t launch_external(Cmd *cmd, PathList *pl) {
    if (pl->count == 0) {
        /* PATH vacío: nada debe ejecutarse */
        print_error();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return -1;
    }

    if (pid == 0) {
        /* Hijo */
        int fd = -1;
        if (cmd->has_redir) {
            fd = open(cmd->redir_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) { print_error(); _exit(1); }
            if (dup2(fd, STDOUT_FILENO) < 0) { print_error(); _exit(1); }
            if (dup2(fd, STDERR_FILENO) < 0) { print_error(); _exit(1); }
            close(fd);
        }

        /* Buscar ejecutable en cada directorio del PATH */
        for (int i = 0; i < pl->count; i++) {
            char full[1024];
            snprintf(full, sizeof(full), "%s/%s", pl->dirs[i], cmd->argv[0]);
            if (access(full, X_OK) == 0) {
                execv(full, cmd->argv);
                /* Si retorna, error al ejecutar */
                print_error();
                _exit(1);
            }
        }
        /* No encontrado en ningún directorio */
        print_error();
        _exit(1);
    }

    /* Padre: devuelve PID para esperar luego */
    return pid;
}

/* --------------------- Procesar línea completa (paralelismo) --------------------- */

static void process_line(char *raw_line, PathList *pl) {
    /* Normalizar operadores para parseo robusto */
    char *line = normalize_ops(raw_line);
    if (!line) { print_error(); return; }

    /* Separar por '&' (comandos paralelos) */
    char *save = NULL;
    char *seg = NULL;
    pid_t pids[256];
    int    pcount = 0;

    for (seg = strtok_r(line, "&", &save);
         seg != NULL;
         seg = strtok_r(NULL, "&", &save)) {

        /* Copia editable para parsear redirección con cortes */
        char *copy = strdup(seg);
        if (!copy) { print_error(); free(line); return; }

        Cmd cmd;
        int r = parse_command(copy, &cmd);
        if (r < 0) {
            print_error();
            free(copy);
            continue;
        }
        if (r == 0) {
            /* suborden vacía: ignorar */
            free(copy);
            continue;
        }

        /* Built-ins (no redirección para built-ins) */
        if (is_builtin(cmd.argv[0])) {
            if (cmd.has_redir) {
                print_error();
                free(copy);
                if (cmd.redir_file) free(cmd.redir_file);
                continue;
            }

            if (!strcmp(cmd.argv[0], "exit")) {
                builtin_exit(cmd.argv); /* no retorna si OK */
            } else if (!strcmp(cmd.argv[0], "cd")) {
                builtin_cd(cmd.argv);
            } else if (!strcmp(cmd.argv[0], "path")) {
                builtin_path(cmd.argv, pl);
            }

            free(copy);
            if (cmd.redir_file) free(cmd.redir_file);
            continue;
        }

        /* Externos */
        pid_t cpid = launch_external(&cmd, pl);
        if (cpid > 0 && pcount < (int)(sizeof(pids)/sizeof(pids[0]))) {
            pids[pcount++] = cpid;
        }
        free(copy);
        if (cmd.redir_file) free(cmd.redir_file);
    }

    /* Esperar a todos los hijos lanzados en esta línea */
    for (int i = 0; i < pcount; i++) {
        waitpid(pids[i], NULL, 0);
    }

    free(line);
}

/* --------------------- main --------------------- */

int main(int argc, char *argv[]) {
    /* Validar número de argumentos */
    if (argc > 2) {
        print_error();
        exit(1);
    }

    /* Definir entrada y modo interactivo */
    FILE *input = stdin;
    int interactive = 1;  /* solo imprime prompt en modo interactivo real */

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            print_error();
            exit(1);
        }
        interactive = 0;   /* batch mode: NUNCA imprimir prompt */
    }

    PathList pl;
    path_init(&pl);

    char *line = NULL;
    size_t cap = 0;

    while (1) {
        if (interactive) {
            /* Solo en modo interactivo real */
            printf("wish> ");
            fflush(stdout);
        }

        ssize_t n = getline(&line, &cap, input);
        if (n == -1) break; /* EOF → salir normal */

        /* Ignorar líneas vacías o solo whitespace */
        int only_ws = 1;
        for (ssize_t i = 0; i < n; i++) {
            if (line[i] != ' ' && line[i] != '\t' &&
                line[i] != '\n' && line[i] != '\r') {
                only_ws = 0; break;
            }
        }
        if (only_ws) continue;

        process_line(line, &pl);
    }

    if (input != stdin) fclose(input);
    free(line);
    return 0;
}
