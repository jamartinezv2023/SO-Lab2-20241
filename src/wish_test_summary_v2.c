/*
 * wish_test_summary_v2.c — Verificador de tests del shell WISH
 *
 * Este programa ejecuta ../bin/wish_victory_v2 sobre los tests ubicados en
 * ../tests_unpacked/tests/, comparando:
 *   - salida estándar (.out)
 *   - salida de error (.err)
 *   - código de retorno (.rc)
 *
 * Imprime el resultado ✅ o ❌ para cada test con su descripción (.desc)
 * y al final muestra el porcentaje total de tests superados.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_PATH 256
#define TEST_COUNT 22
#define BIN_PATH "../bin/wish_victory_v2"
#define TEST_DIR "../tests_unpacked/tests"
#define ERRMSG "An error has occurred\n"

static void print_separator(void) {
    printf("====================================\n");
}

/* Función auxiliar: compara dos archivos línea a línea */
static int compare_files(const char *f1, const char *f2) {
    FILE *a = fopen(f1, "r");
    FILE *b = fopen(f2, "r");
    if (!a || !b) {
        if (a) fclose(a);
        if (b) fclose(b);
        return 0;
    }

    int c1, c2;
    while (1) {
        c1 = fgetc(a);
        c2 = fgetc(b);
        if (c1 == EOF && c2 == EOF) break;
        if (c1 != c2) {
            fclose(a);
            fclose(b);
            return 0; // Diferencia encontrada
        }
    }

    fclose(a);
    fclose(b);
    return 1; // Archivos idénticos
}

/* Ejecuta un test individual */
static int run_test(int num) {
    char base[MAX_PATH];
    snprintf(base, sizeof(base), "%s/%d", TEST_DIR, num);

    char in_file[MAX_PATH], out_expected[MAX_PATH], err_expected[MAX_PATH];
    char desc_file[MAX_PATH], rc_file[MAX_PATH];

    snprintf(in_file, sizeof(in_file), "%s.in", base);
    snprintf(out_expected, sizeof(out_expected), "%s.out", base);
    snprintf(err_expected, sizeof(err_expected), "%s.err", base);
    snprintf(desc_file, sizeof(desc_file), "%s.desc", base);
    snprintf(rc_file, sizeof(rc_file), "%s.rc", base);

    char desc[256] = "(sin descripción)";
    FILE *df = fopen(desc_file, "r");
    if (df) {
        fgets(desc, sizeof(desc), df);
        desc[strcspn(desc, "\n")] = 0;
        fclose(df);
    }

    printf("🔹 TEST %02d: %s\n", num, desc);

    /* Archivos temporales para capturar la salida */
    char out_tmp[] = "/tmp/wish_out_XXXXXX";
    char err_tmp[] = "/tmp/wish_err_XXXXXX";
    int fd_out = mkstemp(out_tmp);
    int fd_err = mkstemp(err_tmp);

    if (fd_out < 0 || fd_err < 0) {
        perror("mkstemp");
        return 0;
    }

    pid_t pid = fork();
    if (pid == 0) {
        /* Redirigir stdout y stderr a los archivos temporales */
        dup2(fd_out, STDOUT_FILENO);
        dup2(fd_err, STDERR_FILENO);
        close(fd_out);
        close(fd_err);

        execlp(BIN_PATH, BIN_PATH, in_file, NULL);
        perror("exec");
        exit(127);
    }

    close(fd_out);
    close(fd_err);

    int status;
    waitpid(pid, &status, 0);
    int rc = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

    /* Leer código esperado */
    int rc_expected = 0;
    FILE *frc = fopen(rc_file, "r");
    if (frc) {
        fscanf(frc, "%d", &rc_expected);
        fclose(frc);
    }

    int ok_out = compare_files(out_tmp, out_expected);
    int ok_err = compare_files(err_tmp, err_expected);
    int ok_rc  = (rc == rc_expected);

    unlink(out_tmp);
    unlink(err_tmp);

    if (ok_out && ok_err && ok_rc) {
        printf("✅ TEST %02d superado.\n\n", num);
        return 1;
    } else {
        printf("❌ TEST %02d falló.\n\n", num);
        return 0;
    }
}

/* ---------------- MAIN ---------------- */
int main(void) {
    print_separator();
    printf("🧪 Verificador de tests — wish_victory_v2\n");
    print_separator();
    printf("\n");

    int passed = 0;
    for (int i = 1; i <= TEST_COUNT; i++) {
        passed += run_test(i);
    }

    print_separator();
    double pct = (double)passed / TEST_COUNT * 100.0;
    printf("🏁 RESULTADO FINAL: %d/%d tests superados (%.2f%%)\n", passed, TEST_COUNT, pct);
    print_separator();

    return 0;
}
