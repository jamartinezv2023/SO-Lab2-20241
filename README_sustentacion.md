# 🧪 Sustentación Laboratorio 2 — Shell WISH (Sistemas Operativos, UdeA 2025-2)

## 📖 Descripción General
Este laboratorio tiene como objetivo implementar un **shell Unix simplificado** llamado `wish` (Wisconsin Shell). El ejercicio permite comprender el funcionamiento de los procesos, el ciclo de vida de un intérprete de comandos y la ejecución de programas en Linux.

El desarrollo se realizó en **Ubuntu sobre WSL2 (Windows 11)**, siguiendo las especificaciones del profesor **Remzi Arpaci-Dusseau** y adaptando el entorno con **GCC 13.3.0** y **GNU Make 4.3**.

---

## ⚙️ Objetivos Específicos
1. Comprender la creación y gestión de procesos en Linux mediante `fork()`, `execv()`, y `waitpid()`.
2. Implementar comandos integrados (`cd`, `exit`, `path`).
3. Incorporar redirección de salida (`>`) y ejecución paralela (`&`).
4. Validar el funcionamiento mediante el script de pruebas `test_wish.sh`.

---

## 🧩 Estructura del Proyecto
```bash
/root/so_lab2/
 ├── src/
 │   └── wish.c
 ├── tests/
 │   ├── batch.txt
 │   ├── expected_output/
 │   └── test_wish.sh
 ├── docs/
 │   └── README_sustentacion.md
 ├── setup_lab2.sh
 ├── Makefile
 └── report_lab2.log
```

---

## 💻 Compilación y Ejecución
```bash
cd ~/so_lab2
chmod +x setup_lab2.sh
./setup_lab2.sh
```

Esto:
- Compila `wish.c` usando `gcc`.
- Ejecuta automáticamente las pruebas.
- Genera un informe `report_lab2.log` con los resultados detallados.

---

## 🧠 Algoritmos Fundamentales

### 1. Ciclo principal del shell (`main`)
```c
while (1) {
  printf("wish> ");
  getline(&line, &len, stdin);
  parse_and_execute(line);
}
```

### 2. Creación de procesos
```c
pid_t pid = fork();
if (pid == 0) {
  execv(path, args);
} else {
  waitpid(pid, NULL, 0);
}
```

### 3. Redirección de salida
```c
int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
dup2(fd, STDOUT_FILENO);
dup2(fd, STDERR_FILENO);
```

---

## 🧾 Resultados Esperados

| Prueba | Descripción | Resultado |
|---------|--------------|-----------|
| `exit` | Termina el shell | ✅ PASS |
| `cd` | Cambia directorio actual | ✅ PASS |
| `path` | Configura rutas de búsqueda | ✅ PASS |
| `>` | Redirección de salida | ✅ PASS |
| `&` | Ejecución en paralelo | ✅ PASS |

---

## 📚 Referencias (Formato IEEE)
1. R. Arpaci-Dusseau and A. Arpaci-Dusseau, *Operating Systems: Three Easy Pieces*, 2021. [Online]. Available: https://pages.cs.wisc.edu/~remzi/OSTEP/
2. IEEE Std 1003.1-2017, “POSIX.1-2017 - Standard for Information Technology—Portable Operating System Interface (POSIX),” IEEE, 2018.
3. The Linux Foundation, “Linux Manual Pages,” [Online]. Available: https://man7.org/linux/man-pages/.

---

## 🧩 Conclusión
La práctica permitió comprender a fondo los mecanismos de creación de procesos, el rol del kernel en la ejecución de programas y la interacción entre las llamadas al sistema `fork()`, `execv()` y `waitpid()`.  
El script `setup_lab2.sh` garantiza reproducibilidad y facilita la evaluación automática, cumpliendo los estándares docentes y técnicos.

---

**Autor:** José Martínez V.  
**Institución:** Universidad de Antioquia  
**Periodo:** 2025-2  
**Repositorio:** [github.com/jamartinezv2023/SO-Lab2-20241](https://github.com/jamartinezv2023/SO-Lab2-20241)
