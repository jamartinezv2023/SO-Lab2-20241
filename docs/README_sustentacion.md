# 🧠 Laboratorio 2 — Implementación del Shell `wish_v2`

**Estudiante:** José Alfredo Martínez Valdés  
**Profesor:** Henry Alberto Arcila Ramírez 
**Curso:** Sistemas Operativos — UdeA 2025-2 
**Institución:** Universidad de Antioquia  
**Repositorio:** [SO-Lab2-20241](https://github.com/jamartinezv2023/SO-Lab2-20241)  
**Ruta del proyecto:**  
`/mnt/c/Users/iesaf/OneDrive/Documentos/UdeA2025_2/SISTEMAS OPERATIVOS/laboratorio2/entrega_laboratorio2_25102025/src/wish_v2.c`

---

## 1. Descripción general de la práctica

El presente laboratorio tiene como propósito implementar un **intérprete de comandos** o **mini shell** denominado `wish_v2`, con las siguientes funcionalidades principales:

- Ejecución de comandos internos y externos.
- Redirección de salida estándar mediante `>` (sobrescribir) y `>>` (añadir).
- Ejecución paralela de procesos con el operador `&`.
- Soporte para modo **interactivo** y **batch** (lectura desde archivo de comandos).
- Implementación modular conforme a principios de **claridad, robustez y documentación docente**.

<details>
<summary>🗣️ Nota de orador</summary>

> Este laboratorio se ha desarrollado utilizando wsl en Ubunto utilizando Microsoft Windows "Este laboratorio busca comprender cómo un sistema operativo ejecuta procesos mediante llamadas al sistema como `fork()` y `execv()`."

</details>

---

## 2. Objetivos de aprendizaje

- Comprender la creación y manejo de procesos en Linux.
- Implementar correctamente **redirección de flujo estándar (stdout)**.
- Gestionar **ejecuciones concurrentes** usando `fork()` y `waitpid()`.
- Interpretar y tokenizar comandos del usuario.
- Diseñar código limpio, modular y documentado.

---

## 3. Arquitectura y algoritmos del shell

### 3.1. Estructura general del programa

El archivo principal es:

```
/mnt/c/Users/iesaf/OneDrive/Documentos/UdeA2025_2/SISTEMAS OPERATIVOS/laboratorio2/entrega_laboratorio2_25102025/src/wish_v2.c
```

El flujo principal se resume así:

```c
int main(int argc, char *argv[]) {
    PathList path_list;
    init_path_list(&path_list);
    if (argc == 1) {
        interactive_mode(&path_list);
    } else if (argc == 2) {
        batch_mode(argv[1], &path_list);
    } else {
        print_error();
        exit(1);
    }
    return 0;
}
```

**Explicación:**
- Si el usuario no pasa parámetros, entra en modo **interactivo** (`wish>`).
- Si pasa un archivo, se ejecuta en modo **batch** leyendo comandos secuencialmente.
- Se inicializa una lista interna `PATH` con las rutas por defecto (`/bin`, `/usr/bin`, etc.).

---

### 3.2. Algoritmo de ejecución de comandos

```c
pid_t pid = fork();
if (pid == 0) {
    execv(cmd_path, args);
    print_error();
    exit(1);
} else if (pid > 0) {
    if (!background)
        waitpid(pid, NULL, 0);
} else {
    print_error();
}
```

**Lógica:**
1. `fork()` crea un nuevo proceso hijo.
2. En el hijo, se reemplaza la imagen del proceso con el comando solicitado.
3. En el padre, se espera (si no se usa `&`) a que el hijo finalice.

<details>
<summary>🗣️ ¿Cómo lo explico?</summary>

> Aquí explico la diferencia entre los espacios de memoria del padre e hijo y muestro un diagrama simple con el flujo de `fork → exec → wait`.

</details>

---

### 3.3. Redirección `>` y `>>`

```c
int fd;
if (append_mode)
    fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
else
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(fd, STDOUT_FILENO);
close(fd);
```

**Explicación paso a paso:**
- Se detecta el operador `>` o `>>`.
- Se abre el archivo con los flags adecuados (`O_TRUNC` para sobrescribir, `O_APPEND` para añadir).
- Se usa `dup2()` para redirigir `stdout` al archivo.
- La salida del comando se escribe directamente en el archivo destino.

---

### 3.4. Ejecución en paralelo `&`

Cuando un comando termina con `&`, se evita el `waitpid()` en el proceso padre.

```c
if (background == 0) {
    waitpid(pid, NULL, 0);
}
```

Esto permite que múltiples procesos hijos se ejecuten simultáneamente.

---

## 4. Pruebas automáticas ejecutadas (`make test`)

| **Test** | **Comando / Escenario** | **Resultado esperado** | **Estado** |
|-----------|--------------------------|--------------------------|-------------|
| test_basic | `ls -l` | Lista el contenido del directorio actual | ✅ PASA |
| test_redirect | `echo "Hola Mundo" > salida.txt` | Archivo creado con texto | ✅ PASA |
| test_append | `echo "Nueva línea" >> salida.txt` | Archivo actualizado | ✅ PASA |
| test_parallel | `sleep 2 & ls` | Ambos procesos se ejecutan | ✅ PASA |
| test_batch | `./wish_v2 batch.txt` | Ejecuta todos los comandos del archivo | ✅ PASA |

---

## 5. Mensajes de error controlados

Ejemplo de advertencia:
```
../src/wish_v2.c:335:56: warning: unused parameter ‘interactive’ [-Wunused-parameter]
```

**Interpretación:**  
El parámetro `interactive` no se usa dentro de la función `process_line`.  
Se puede eliminar o convertir en variable global según el diseño.

---

## 6. Guía de sustentación oral

<details>
<summary>🎤 Notas para exposición</summary>

- **Inicio:** “Implementé el shell `wish_v2` que soporta redirección, ejecución paralela y modo batch.”  
- **Código:** “Aquí se ve cómo `fork()` crea un proceso hijo para ejecutar el comando.”  
- **Pruebas:** “Al ejecutar `make test`, todos los casos pasan correctamente.”  
- **Cierre:** “El laboratorio demuestra comprensión de las llamadas al sistema de UNIX y la sincronización de procesos.”

</details>

---

## 7. Conclusiones

- Se implementó un shell funcional conforme a las especificaciones del laboratorio.  
- Se comprendió el uso de **`fork()`**, **`execv()`**, **`dup2()`**, **`open()`** y **`waitpid()`**.  
- Se documentaron todas las funciones y se integraron pruebas automáticas.  

---

## 8. Referencias (IEEE)

[1] R. Arpaci-Dusseau and A. Arpaci-Dusseau, *Operating Systems: Three Easy Pieces*, 2020.  
[2] GNU Project, “man execv,” *GNU/Linux Manual Pages*, 2024.  
[3] Microsoft, “Using Windows Subsystem for Linux (WSL) on Windows 11,” *MS Docs*, 2024.  
[4] IEEE Computer Society, *IEEE Citation Style Guide*, 2024.  

---

🧩 **Versión del documento:** 1.0  
📅 **Fecha:** 26 de octubre de 2025  
