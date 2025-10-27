#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TEST_DIR "./tests_unpacked/tests"
#define MAX_PATH 512
#define MAX_TESTS 64

// Función para comparar archivos línea a línea
int compare_files(const char *f1, const char *f2) {
    FILE *a = fopen(f1, "r");
    FILE *b = fopen(f2, "r");
    if (!a || !b) {
        if (a) fclose(a);
        if (b) fclose(b);
        return -1;
    }

    int diff = 0;
    char lineA[1024], lineB[1024];
    while (fgets(lineA, sizeof(lineA), a)) {
        if (!fgets(lineB, sizeof(lineB), b) || strcmp(lineA, lineB) != 0) {
            diff = 1;
            break;
        }
    }
    if (fgets(lineB, sizeof(lineB), b)) diff = 1; // Extra line in b
    fclose(a);
    fclose(b);
    return diff;
}

// Ejecuta un test y devuelve 1 si pasa, 0 si falla
int run_test(int num) {
    char in_file[MAX_PATH], out_expected[MAX_PATH], err_expected[MAX_PATH];
    char out_temp[MAX_PATH], err_temp[MAX_PATH];

    snprintf(in_file, sizeof(in_file), "%s/%d.in", TEST_DIR, num);
    snprintf(out_expected, sizeof(out_expected), "%s/%d.out", TEST_DIR, num);
    snprintf(err_expected, sizeof(err_expected), "%s/%d.err", TEST_DIR, num);

    snprintf(out_temp, sizeof(out_temp), "tmp_out_%d.txt", num);
    snprintf(err_temp, sizeof(err_temp), "tmp_err_%d.txt", num);

    int pid = fork();
    if (pid == 0) {
        int fd_in = open(in_file, O_RDONLY);
        int fd_out = open(out_temp, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        int fd_err = open(err_temp, O_CREAT | O_WRONLY | O_TRUNC, 0666);
        if (fd_in < 0 || fd_out < 0 || fd_err < 0) {
            perror("open");
            exit(1);
        }

        dup2(fd_in, STDIN_FILENO);
        dup2(fd_out, STDOUT_FILENO);
        dup2(fd_err, STDERR_FILENO);
        close(fd_in);
        close(fd_out);
        close(fd_err);

        execlp("../bin/wish_victory", "wish_victory", NULL);
        perror("execlp");
        exit(1);
    }

    int status;
    waitpid(pid, &status, 0);

    int out_diff = compare_files(out_temp, out_expected);
    int err_diff = compare_files(err_temp, err_expected);

    unlink(out_temp);
    unlink(err_temp);

    return (out_diff == 0 && err_diff == 0);
}

int main(void) {
    printf("====================================\n");
    printf("🧪 Verificador de tests — wish_victory\n");
    printf("====================================\n\n");

    int passed = 0, total = 0;

    for (int i = 1; i <= 22; i++) {
        char test_file[MAX_PATH];
        snprintf(test_file, sizeof(test_file), "%s/%d.in", TEST_DIR, i);
        if (access(test_file, F_OK) != 0) continue;

        total++;
        printf("🔹 TEST %02d: ", i);
        fflush(stdout);

        int ok = run_test(i);
        if (ok) {
            printf("✅ PASÓ\n");
            passed++;
        } else {
            printf("❌ FALLÓ\n");
        }
    }

    printf("\n====================================\n");
    printf("🏁 RESULTADO FINAL: %d/%d tests superados (%.2f%%)\n",
           passed, total, (100.0 * passed / total));
    printf("====================================\n");

    return 0;
}
