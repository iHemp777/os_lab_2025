CC=gcc
CFLAGS=-I.

# Основная цель - сборка всех программ (только существующих)
all: sequential_min_max parallel_min_max fork_exec_seq

# Цель для сборки последовательной версии
sequential_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)

# Цель для сборки параллельной версии  
parallel_min_max: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Цель - программа с fork+exec
fork_exec_seq: 
	$(CC) -o fork_exec_seq fork_exec_seq.c $(CFLAGS)

# Компиляция utils.c в объектный файл
utils.o: utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

# Компиляция find_min_max.c в объектный файл
find_min_max.o: utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

# Очистка скомпилированных файлов
clean:
	rm -f utils.o find_min_max.o sequential_min_max parallel_min_max fork_exec_seq

# Флаг для предотвращения конфликтов с одноименными файлами
.PHONY: all clean
