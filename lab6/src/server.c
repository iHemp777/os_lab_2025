#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

struct FactorialArgs {
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
};

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;
    
    if (args->begin == 0 || args->end == 0) {
        return 1;
    }
    
    for (uint64_t i = args->begin; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }
    
    return ans;
}

void *ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs *)args;
    uint64_t *result = malloc(sizeof(uint64_t));
    if (result) {
        *result = Factorial(fargs);
    }
    return result;
}

int main(int argc, char **argv) {
    int tnum = -1;
    int port = -1;

    while (true) {
        static struct option options[] = {
            {"port", required_argument, 0, 0},
            {"tnum", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1)
            break;

        switch (c) {
        case 0: {
            switch (option_index) {
            case 0:
                port = atoi(optarg);
                break;
            case 1:
                tnum = atoi(optarg);
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
        } break;

        case '?':
            printf("Unknown argument\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Usage: %s --port 20001 --tnum 4\n", argv[0]);
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Can not create server socket");
        return 1;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Can not bind to socket");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 128) < 0) {
        perror("Could not listen on socket");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port %d with %d threads\n", port, tnum);

    while (true) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

        if (client_fd < 0) {
            perror("Could not establish new connection");
            continue;
        }

        printf("New connection accepted\n");

        while (true) {
            unsigned int buffer_size = sizeof(uint64_t) * 3;
            char from_client[buffer_size];
            int read_bytes = recv(client_fd, from_client, buffer_size, 0);

            if (read_bytes == 0) {
                printf("Client disconnected\n");
                break;
            }
            
            if (read_bytes < 0) {
                perror("Client read failed");
                break;
            }
            
            if (read_bytes < (int)buffer_size) {
                fprintf(stderr, "Client sent wrong data format\n");
                break;
            }

            uint64_t begin = 0, end = 0, mod = 0;
            memcpy(&begin, from_client, sizeof(uint64_t));
            memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
            memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

            printf("Received task: factorial(%lu, %lu) mod %lu\n", 
                   (unsigned long)begin, (unsigned long)end, (unsigned long)mod);

            pthread_t threads[tnum];
            struct FactorialArgs args[tnum];
            
            uint64_t range = (begin <= end) ? (end - begin + 1) : 0;
            if (range == 0) {
                uint64_t total = 1;
                char buffer[sizeof(total)];
                memcpy(buffer, &total, sizeof(total));
                send(client_fd, buffer, sizeof(total), 0);
                continue;
            }
            
            uint64_t chunk_size = range / tnum;
            uint64_t remainder = range % tnum;
            uint64_t current_start = begin;
            
            for (int i = 0; i < tnum; i++) {
                args[i].begin = current_start;
                args[i].mod = mod;
                
                uint64_t current_end = current_start + chunk_size - 1;
                if (i < (int)remainder) {
                    current_end++;
                }
                if (current_end > end) current_end = end;
                
                args[i].end = current_end;
                current_start = current_end + 1;
                
                if (pthread_create(&threads[i], NULL, ThreadFactorial, &args[i]) != 0) {
                    perror("pthread_create failed");
                    break;
                }
                
                printf("Thread %d: processing [%lu-%lu]\n", i, 
                       (unsigned long)args[i].begin, (unsigned long)args[i].end);
            }

            uint64_t total = 1;
            for (int i = 0; i < tnum; i++) {
                uint64_t* thread_result = NULL;
                pthread_join(threads[i], (void**)&thread_result);
                
                if (thread_result) {
                    total = MultModulo(total, *thread_result, mod);
                    free(thread_result);
                }
            }

            printf("Computed result: %lu\n", (unsigned long)total);

            char buffer[sizeof(total)];
            memcpy(buffer, &total, sizeof(total));
            
            if (send(client_fd, buffer, sizeof(total), 0) < 0) {
                perror("Cannot send data to client");
                break;
            }
            
            printf("Result sent to client\n");
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}