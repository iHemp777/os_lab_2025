#include "find_min_max.h"
#include <limits.h>
#include <stddef.h>

struct MinMax GetMinMax(int *array, unsigned int begin, unsigned int end) {
  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Проверка на корректность входных данных
  if (array == NULL || begin >= end) {
    min_max.min = 0;
    min_max.max = 0;
    return min_max;
  }

  // Поиск минимального и максимального элементов на промежутке [begin, end)
  for (unsigned int i = begin; i < end; i++) {
    if (array[i] < min_max.min) {
      min_max.min = array[i];
    }
    if (array[i] > min_max.max) {
      min_max.max = array[i];
    }
  }

  return min_max;
}