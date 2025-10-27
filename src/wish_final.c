// Archivo: wish_final.c
// Agenda de sustentación, 29 de octubre de 2025 de Laboratorio SO
// Autor: Estudiante José Alfredo Martínez Valdés

#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>



#define TESTS_DIR "./tests_unpacked/tests"
#define MAX_TESTS 22
#define BUF_SIZE 1024

// Leer el contenido completo de un archivo
char *read_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);
    char *buffer = malloc(len + 1);
    fread(buffer, 1, len, fp);
    buffer[len] = '\0';
    fclose(fp);
    return buffer;
}

// Comparar dos archivos (salidas esperadas vs. reales)
int compare_files(const char *f1, const char *f2) {
    FILE *fp1 = fopen(f1, "r");
    FILE *fp2 = fopen(f2, "r");
    if (!fp1 || !fp2) return 0;

    char b1[BUF_SIZE], b2[BUF_SIZE];
    while (fgets(b1, sizeof(b1), fp1) && fgets(b2, sizeof(b2), fp2)) {
        if (strcmp(b1, b2) != 0) {
            fclose(fp1);
            fclose(fp2);
            return 0;
        }
    }
    int result = (feof(fp1) && feof(fp2));
    fclose(fp1);
    fclose(fp2);
    return result;
}

// Ejecutar un test individual
int run_test(int num) {
    char desc_path[128], in_path[128], out_path[128], err_path[128];
    snprintf(desc_path, sizeof(desc_path), "%s/%d.desc", TESTS_DIR, num);
    snprintf(in_path, sizeof(in_path), "%s/%d.in", TESTS_DIR, num);
    snprintf(out_path, sizeof(out_path), "%s/%d.out", TESTS_DIR, num);
    snprintf(err_path, sizeof(err_path), "%s/%d.err", TESTS_DIR, num);

    char tmp_out[] = "/tmp/outXXXXXX";
    char tmp_err[] = "/tmp/errXXXXXX";
    int fd_out = mkstemp(tmp_out);
    int fd_err = mkstemp(tmp_err);

    char *desc = read_file(desc_path);
    if (!desc) desc = strdup("(sin descripción)");

    printf("\n🔹 TEST %02d: %s\n", num, desc);
    free(desc);

    pid_t pid = fork();
    if (pid == 0) {
        // Redirigir salida estándar y error
        dup2(fd_out, STDOUT_FILENO);
        dup2(fd_err, STDERR_FILENO);
        close(fd_out);
        close(fd_err);

        // Abrir archivo de entrada y redirigir stdin
        int fd_in = open(in_path, O_RDONLY);
        if (fd_in < 0) {
            perror("No se pudo abrir archivo de entrada");
            exit(1);
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);

        // Ejecutar el shell
        execlp("../bin/wish", "../bin/wish", NULL);
        perror("Error ejecutando wish");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("Error en fork()");
        return 0;
    }

    close(fd_out);
    close(fd_err);

    // Comparar salidas
    int ok_out = compare_files(tmp_out, out_path);
    int ok_err = compare_files(tmp_err, err_path);

    if (ok_out && ok_err)
        printf("✅ TEST %02d superado correctamente.\n", num);
    else
        printf("❌ TEST %02d falló (salida diferente).\n", num);

    unlink(tmp_out);
    unlink(tmp_err);
    return (ok_out && ok_err);
}

int main(void) {
    printf("====================================\n");
    printf("🧪 Laboratorio SO — Test Runner Wish\n");
    printf("====================================\n");

    int passed = 0;
    for (int i = 1; i <= MAX_TESTS; i++) {
        passed += run_test(i);
    }

    printf("\n====================================\n");
    printf("🧾 RESULTADO FINAL: %d/%d tests superados.\n", passed, MAX_TESTS);
    printf("====================================\n");
    return 0;
}
