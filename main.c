#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

static const char *STUDENT_NAME  = "Victor Eduardo Perez Aguilar";
static const char *STUDENT_ID    = "A01796394";
static const char *STUDENT_EMAIL = "A01796394@tec.mx";

// Colores
#define C_RESET   "\x1b[0m"
#define C_BOLD    "\x1b[1m"
#define C_DIM     "\x1b[2m"
#define C_RED     "\x1b[31m"
#define C_GREEN   "\x1b[32m"
#define C_YELLOW  "\x1b[33m"
#define C_BLUE    "\x1b[34m"
#define C_MAGENTA "\x1b[35m"
#define C_CYAN    "\x1b[36m"

static void banner(void) {
    printf(C_CYAN C_BOLD "\n==============================================\n" C_RESET);
    printf(C_CYAN C_BOLD "   SUMA PARALELA DE ARREGLOS (OpenMP)\n" C_RESET);
    printf(C_CYAN C_BOLD "==============================================\n" C_RESET);
    printf(C_DIM "Alumno: " C_RESET "%s\n", STUDENT_NAME);
    printf(C_DIM "Matricula: " C_RESET "%s\n", STUDENT_ID);
    printf(C_DIM "Correo: " C_RESET "%s\n", STUDENT_EMAIL);
    printf(C_CYAN "----------------------------------------------\n" C_RESET);
}

static int read_int(const char *prompt, int defaultValue, int minValue, int maxValue) {
    char buf[128];
    while (1) {
        printf("%s%s%s [%d]: ", C_YELLOW, prompt, C_RESET, defaultValue);
        if (!fgets(buf, sizeof(buf), stdin)) return defaultValue;
        if (buf[0] == '\n') return defaultValue;

        int val = 0;
        if (sscanf(buf, "%d", &val) == 1) {
            if (val < minValue || val > maxValue) {
                printf(C_RED "Valor fuera de rango (%d..%d). Intenta de nuevo.\n" C_RESET, minValue, maxValue);
                continue;
            }
            return val;
        }
        printf(C_RED "Entrada invalida. Escribe un numero.\n" C_RESET);
    }
}

static void fill_random_int(int *arr, int n, int maxVal) {
    for (int i = 0; i < n; i++) arr[i] = rand() % maxVal;
}

static void print_slice(const char *name, const int *arr, int n, int k) {
    if (k <= 0) return;

    int limit = (n < k) ? n : k;
    printf(C_BLUE C_BOLD "%s" C_RESET " (primeros %d): ", name, limit);
    for (int i = 0; i < limit; i++) {
        printf("%d%s", arr[i], (i == limit - 1) ? "" : ", ");
    }
    printf("\n");

    if (n > 2 * k) {
        printf(C_BLUE C_BOLD "%s" C_RESET " (ultimos %d):  ", name, k);
        for (int i = n - k; i < n; i++) {
            printf("%d%s", arr[i], (i == n - 1) ? "" : ", ");
        }
        printf("\n");
    }
}

static int verify_sum(const int *A, const int *B, const int *C, int n) {
    for (int i = 0; i < n; i++) {
        if (C[i] != A[i] + B[i]) return 0;
    }
    return 1;
}

// Obtiene cuántos threads realmente se están usando
static int threads_used_now(void) {
    int used = 1;
    #pragma omp parallel
    {
        #pragma omp single
        used = omp_get_num_threads();
    }
    return used;
}

// Ejecuta suma serial y retorna tiempo (segundos)
static double sum_serial(const int *A, const int *B, int *C, int n) {
    double t1 = omp_get_wtime();
    for (int i = 0; i < n; i++) C[i] = A[i] + B[i];
    double t2 = omp_get_wtime();
    return t2 - t1;
}

// Ejecuta suma paralela y retorna tiempo (segundos)
static double sum_parallel(const int *A, const int *B, int *C, int n) {
    double t1 = omp_get_wtime();
    #pragma omp parallel for
    for (int i = 0; i < n; i++) C[i] = A[i] + B[i];
    double t2 = omp_get_wtime();
    return t2 - t1;
}

int main(void) {
    banner();

    // Parámetros con default
    int N       = read_int("Ingresa N (tamanio del arreglo)", 10000000, 10, 100000000);
    int MAXVAL  = read_int("Ingresa MAXVAL (valores 0..MAXVAL-1)", 100, 2, 1000000);
    int PRINT_K = read_int("Cuantos elementos imprimir (K)", 10, 0, 200);

    // Importante: capturar threads detectados ANTES de fijar T
    int detectedMaxThreads = omp_get_max_threads();
    int T = read_int("Cuantos threads usar (OpenMP)", detectedMaxThreads, 1, 256);

    // Repeticiones para promediar tiempos (evita mediciones ridículas)
    int REPS = read_int("Repeticiones para promediar tiempos", 10, 1, 50);

    printf("\n" C_MAGENTA C_BOLD "Config:\n" C_RESET);
    printf("  N       = %d\n", N);
    printf("  MAXVAL  = %d\n", MAXVAL);
    printf("  K       = %d\n", PRINT_K);
    printf("  Threads detectados (max) = %d\n", detectedMaxThreads);
    printf("  Threads solicitados      = %d\n", T);
    printf("  Repeticiones (promedio)  = %d\n\n", REPS);

    // Fijar threads solicitados
    omp_set_num_threads(T);

    // Semilla random
    srand((unsigned)time(NULL));

    // Reservar memoria
    int *A = (int *)malloc(sizeof(int) * (size_t)N);
    int *B = (int *)malloc(sizeof(int) * (size_t)N);
    int *C_serial   = (int *)malloc(sizeof(int) * (size_t)N);
    int *C_parallel = (int *)malloc(sizeof(int) * (size_t)N);

    if (!A || !B || !C_serial || !C_parallel) {
        printf(C_RED "Error: no se pudo asignar memoria. (Prueba con un N mas chico)\n" C_RESET);
        free(A); free(B); free(C_serial); free(C_parallel);
        return 1;
    }

    printf(C_CYAN "Generando arreglos con valores aleatorios...\n" C_RESET);
    fill_random_int(A, N, MAXVAL);
    fill_random_int(B, N, MAXVAL);

    // Mostrar threads reales
    int used = threads_used_now();
    printf(C_CYAN "Threads usados (real): %d\n" C_RESET, used);

    // Warm-up (reduce ruido de medición)
    printf(C_CYAN "Warm-up...\n" C_RESET);
    (void)sum_serial(A, B, C_serial, N);
    (void)sum_parallel(A, B, C_parallel, N);

    // Medición con promedio
    printf(C_CYAN "Midiendo tiempos (promedio de %d reps)...\n" C_RESET, REPS);

    double serialSum = 0.0;
    double parallelSum = 0.0;

    for (int r = 0; r < REPS; r++) {
        serialSum += sum_serial(A, B, C_serial, N);
        parallelSum += sum_parallel(A, B, C_parallel, N);
    }

    double serialTime = serialSum / REPS;
    double parallelTime = parallelSum / REPS;

    // Imprimir muestra para validar (solo K)
    printf("\n" C_GREEN C_BOLD "Muestra de datos (para comprobar):\n" C_RESET);
    print_slice("A", A, N, PRINT_K);
    print_slice("B", B, N, PRINT_K);
    print_slice("C (paralelo)", C_parallel, N, PRINT_K);

    // Verificación
    int okSerial   = verify_sum(A, B, C_serial, N);
    int okParallel = verify_sum(A, B, C_parallel, N);

    printf("\n" C_MAGENTA C_BOLD "Verificacion:\n" C_RESET);
    printf("  Serial:   %s%s%s\n", okSerial ? C_GREEN : C_RED, okSerial ? "OK" : "FALLO", C_RESET);
    printf("  Paralelo: %s%s%s\n", okParallel ? C_GREEN : C_RED, okParallel ? "OK" : "FALLO", C_RESET);

    // Resultados de tiempo + speedup
    printf("\n" C_MAGENTA C_BOLD "Rendimiento (promedio):\n" C_RESET);
    printf("  Tiempo serial:   %.9f s\n", serialTime);
    printf("  Tiempo paralelo: %.9f s\n", parallelTime);

    if (parallelTime > 0.0) {
        double speedup = serialTime / parallelTime;
        printf("  Speedup (serial/paralelo): %.3fx\n", speedup);

        if (speedup >= 1.0) {
            printf(C_GREEN "  Paralelizar valio la pena.\n" C_RESET);
        } else {
            printf(C_YELLOW "  Con N chico, overhead y/o limites de memoria, paralelo puede no ganar.\n" C_RESET);
        }
    }

    printf(C_CYAN "\n==============================================\n" C_RESET);

    free(A); free(B); free(C_serial); free(C_parallel);
    return 0;
}
