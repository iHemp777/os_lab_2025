CC = gcc
CFLAGS = -Wall -Wextra

# Цели по умолчанию
all: tcpclient tcpserver udpclient udpserver

# TCP клиент
tcpclient: tcpclient.c
	$(CC) $(CFLAGS) -o tcpclient tcpclient.c

# TCP сервер
tcpserver: tcpserver.c
	$(CC) $(CFLAGS) -o tcpserver tcpserver.c

# UDP клиент
udpclient: udpclient.c
	$(CC) $(CFLAGS) -o udpclient udpclient.c

# UDP сервер
udpserver: udpserver.c
	$(CC) $(CFLAGS) -o udpserver udpserver.c

# Очистка
clean:
	rm -f tcpclient tcpserver udpclient udpserver

# Запуск примеров
run-tcp: tcpclient tcpserver
	@echo "Запустите в разных терминалах:"
	@echo "1. ./tcpserver 8080 1024"
	@echo "2. ./tcpclient 127.0.0.1 8080 1024"

run-udp: udpclient udpserver
	@echo "Запустите в разных терминалах:"
	@echo "1. ./udpserver 9090 1024"
	@echo "2. ./udpclient 127.0.0.1 9090 1024"

.PHONY: all clean run-tcp run-udp