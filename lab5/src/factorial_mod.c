#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

// Структура для передачи данных в поток
typedef struct {
    int start;
    int end;
    int mod;
    unsigned long long* result;
    pthread_mutex_t* mutex;
} ThreadData;

// Функция, выполняемая каждым потоком
void* calculate_partial_factorial(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    unsigned long long partial_result = 1;
    
    // Вычисляем часть факториала
    for (int i = data->start; i <= data->end; i++) {
        partial_result = (partial_result * (i % data->mod)) % data->mod;
    }
    
    // Защищаем обновление общего результата мьютексом
    pthread_mutex_lock(data->mutex);
    *(data->result) = (*(data->result) * (partial_result % data->mod)) % data->mod;
    pthread_mutex_unlock(data->mutex);
    
    return NULL;
}

// Функция для разбора аргументов командной строки
int parse_arguments(int argc, char* argv[], int* k, int* pnum, int* mod) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
            *k = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            *pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            *mod = atoi(argv[i] + 6);
        } else {
            fprintf(stderr, "Неизвестный параметр: %s\n", argv[i]);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char* argv[]) {
    int k = 0, pnum = 0, mod = 0;
    
    // Парсим аргументы командной строки
    if (!parse_arguments(argc, argv, &k, &pnum, &mod) || k <= 0 || pnum <= 0 || mod <= 0) {
        printf("Использование: %s -k <число> --pnum=<кол-во потоков> --mod=<модуль>\n", argv[0]);
        printf("Пример: %s -k 10 --pnum=4 --mod=10\n", argv[0]);
        return 1;
    }
    
    // Если количество потоков больше k, уменьшаем его
    if (pnum > k) {
        pnum = k;
    }
    
    // Инициализируем мьютекс и результат
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    unsigned long long result = 1;
    
    // Создаем массив потоков и данных для них
    pthread_t threads[pnum];
    ThreadData thread_data[pnum];
    
    // Распределяем работу между потоками
    int elements_per_thread = k / pnum;
    int remainder = k % pnum;
    int start = 1;
    
    // Создаем и запускаем потоки
    for (int i = 0; i < pnum; i++) {
        int end = start + elements_per_thread - 1;
        if (i < remainder) {
            end++;
        }
        
        thread_data[i].start = start;
        thread_data[i].end = end;
        thread_data[i].mod = mod;
        thread_data[i].result = &result;
        thread_data[i].mutex = &mutex;
        
        if (pthread_create(&threads[i], NULL, calculate_partial_factorial, &thread_data[i]) != 0) {
            perror("Ошибка создания потока");
            return 1;
        }
        
        start = end + 1;
    }
    
    // Ожидаем завершения всех потоков
    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Ошибка ожидания потока");
            return 1;
        }
    }
    
    // Выводим результат
    printf("%d! mod %d = %llu\n", k, mod, result);
    
    // Очищаем ресурсы
    pthread_mutex_destroy(&mutex);
    
    return 0;
}
