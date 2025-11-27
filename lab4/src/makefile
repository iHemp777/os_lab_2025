CC=gcc
CFLAGS=-I.
OLD_CFLAGS=-I. -w -Wno-implicit-int -Wno-format -Wno-return-type
PTHREAD_FLAGS=-pthread

# Основная цель - сборка всех программ lab4
all: parallel_min_max zombie_demo process_memory parallel_sum

# Цель для сборки параллельной версии с таймаутом (задание 1)
parallel_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Цель для демонстрации зомби-процессов (задание 2)
zombie_demo: 
	$(CC) -o zombie_demo zombie_demo.c $(CFLAGS)




# Цель для process_memory (задание 3) - БЕЗ ИЗМЕНЕНИЙ КОДА
process_memory:
	$(CC) $(OLD_CFLAGS) -o process_memory process_memory.c

# Цель для parallel_sum (задание 5)
parallel_sum: utils.o sum_utils.o
	$(CC) -o parallel_sum parallel_sum.c utils.o sum_utils.o $(CFLAGS) $(PTHREAD_FLAGS)

# Компиляция sum_utils.c
sum_utils.o: sum_utils.h
	$(CC) -c sum_utils.c $(CFLAGS) $(PTHREAD_FLAGS)

# Компиляция utils.c в объектный файл
utils.o: utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

# Компиляция find_min_max.c в объектный файл
find_min_max.o: utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

# Очистка скомпилированных файлов
clean:
	rm -f utils.o find_min_max.o sum_utils.o parallel_min_max zombie_demo zombie_army process_memory parallel_sum

# Флаг для предотвращения конфликтов с одноименными файлами
.PHONY: all clean