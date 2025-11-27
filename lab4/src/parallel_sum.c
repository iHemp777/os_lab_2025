#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <getopt.h>
#include <unistd.h>
#include <stdbool.h>
#include "utils.h"  // Из lab3 для GenerateArray
#include "sum_utils.h"

int main(int argc, char **argv) {
    int threads_num = -1;
    int seed = -1;
    int array_size = -1;

    // Обработка аргументов командной строки
    while (true) {
        static struct option options[] = {
            {"threads_num", required_argument, 0, 0},
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        threads_num = atoi(optarg);
                        if (threads_num <= 0) {
                            printf("threads_num must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 1:
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 2:
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be a positive number\n");
                            return 1;
                        }
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (threads_num == -1 || seed == -1 || array_size == -1) {
        printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
        return 1;
    }

    // Генерация массива (не входит в замер времени)
    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);

    // Начало замера времени
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Создание потоков и данных для них
    pthread_t *threads = malloc(threads_num * sizeof(pthread_t));
    ThreadData *thread_data = malloc(threads_num * sizeof(ThreadData));

    int chunk_size = array_size / threads_num;

    // Создание потоков
    for (int i = 0; i < threads_num; i++) {
        thread_data[i].array = array;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == threads_num - 1) ? array_size : (i + 1) * chunk_size;

        if (pthread_create(&threads[i], NULL, calculate_partial_sum, &thread_data[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }

    // Ожидание завершения всех потоков
    for (int i = 0; i < threads_num; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failed");
            return 1;
        }
    }

    // Сбор результатов
    long total_sum = 0;
    for (int i = 0; i < threads_num; i++) {
        total_sum += thread_data[i].partial_sum;
    }

    // Конец замера времени
    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    // Вывод результатов
    printf("Total sum: %ld\n", total_sum);
    printf("Elapsed time: %fms\n", elapsed_time);

    // Вывод частичных сумм для отладки
    printf("Partial sums: ");
    for (int i = 0; i < threads_num; i++) {
        printf("%ld ", thread_data[i].partial_sum);
    }
    printf("\n");

    // Очистка памяти
    free(array);
    free(threads);
    free(thread_data);

    return 0;
}