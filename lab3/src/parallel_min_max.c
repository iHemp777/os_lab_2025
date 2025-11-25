#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
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

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  // Создаем структуры для обмена данными
  int *pipe_fds = NULL;
  char **filenames = NULL;
  
  if (!with_files) {
    // Создаем pipe'ы для каждого процесса
    pipe_fds = malloc(2 * pnum * sizeof(int));
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipe_fds + 2*i) == -1) {
        perror("pipe failed");
        return 1;
      }
    }
  } else {
    // Генерируем имена файлов для каждого процесса
    filenames = malloc(pnum * sizeof(char*));
    for (int i = 0; i < pnum; i++) {
      filenames[i] = malloc(20 * sizeof(char));
      sprintf(filenames[i], "min_max_%d.txt", i);
    }
  }

  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Создаем дочерние процессы
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        
        // Вычисляем границы для текущего процесса
        int chunk_size = array_size / pnum;
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
        
        // Ищем min/max в своей части массива
        struct MinMax local_min_max = GetMinMax(array, start, end);
        
        if (with_files) {
          // Используем файлы для передачи данных
          FILE *file = fopen(filenames[i], "w");
          if (file == NULL) {
            perror("fopen failed");
            exit(1);
          }
          fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
          fclose(file);
        } else {
          // Используем pipe для передачи данных
          close(pipe_fds[2*i]); // закрываем чтение
          write(pipe_fds[2*i + 1], &local_min_max.min, sizeof(int));
          write(pipe_fds[2*i + 1], &local_min_max.max, sizeof(int));
          close(pipe_fds[2*i + 1]);
        }
        
        free(array);
        if (with_files) {
          for (int j = 0; j < pnum; j++) free(filenames[j]);
          free(filenames);
        } else {
          free(pipe_fds);
        }
        exit(0);
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Родительский процесс ждет завершения всех дочерних
  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Собираем результаты от всех процессов
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // читаем из файлов
      FILE *file = fopen(filenames[i], "r");
      if (file == NULL) {
        perror("fopen failed");
        return 1;
      }
      fscanf(file, "%d %d", &min, &max);
      fclose(file);
      // удаляем временный файл
      remove(filenames[i]);
    } else {
      // читаем из pipes
      close(pipe_fds[2*i + 1]); // закрываем запись
      read(pipe_fds[2*i], &min, sizeof(int));
      read(pipe_fds[2*i], &max, sizeof(int));
      close(pipe_fds[2*i]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  if (with_files) {
    for (int i = 0; i < pnum; i++) free(filenames[i]);
    free(filenames);
  } else {
    free(pipe_fds);
  }

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}