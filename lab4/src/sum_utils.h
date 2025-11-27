#ifndef SUM_UTILS_H
#define SUM_UTILS_H

#include <pthread.h>

// Структура для передачи данных в поток
typedef struct {
    int* array;
    int start;
    int end;
    long partial_sum;
} ThreadData;

// Функция для подсчета частичной суммы
void* calculate_partial_sum(void* arg);

#endif