#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

struct Server {
    char ip[255];
    int port;
};

struct ThreadData {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;
    int index;
};

bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }
    if (errno != 0)
        return false;
    *val = i;
    return true;
}

void* ProcessServer(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->server.port);
    
    if (inet_pton(AF_INET, data->server.ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", data->server.ip);
        data->result = 0;
        pthread_exit(NULL);
    }

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        perror("Socket creation failed");
        data->result = 0;
        pthread_exit(NULL);
    }

    if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "Connection to %s:%d failed\n", 
                data->server.ip, data->server.port);
        data->result = 0;
        close(sck);
        pthread_exit(NULL);
    }

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &data->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &data->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &data->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < 0) {
        perror("Send failed");
        data->result = 0;
        close(sck);
        pthread_exit(NULL);
    }

    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        perror("Receive failed");
        data->result = 0;
    } else {
        memcpy(&data->result, response, sizeof(uint64_t));
    }

    close(sck);
    printf("Server %d (%s:%d) processed [%lu-%lu] -> %lu\n", 
           data->index, data->server.ip, data->server.port, 
           (unsigned long)data->begin, (unsigned long)data->end, 
           (unsigned long)data->result);
    pthread_exit(NULL);
}

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    char servers_file[255] = {'\0'};

    while (true) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        if (!ConvertStringToUI64(optarg, &k)) {
                            fprintf(stderr, "Invalid k value\n");
                            return 1;
                        }
                        break;
                    case 1:
                        if (!ConvertStringToUI64(optarg, &mod)) {
                            fprintf(stderr, "Invalid mod value\n");
                            return 1;
                        }
                        break;
                    case 2:
                        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case '?':
                printf("Arguments error\n");
                break;
            default:
                fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == 0 || mod == 0 || !strlen(servers_file)) {
        fprintf(stderr, "Usage: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(servers_file, "r");
    if (!file) {
        perror("Cannot open servers file");
        return 1;
    }

    struct Server servers[100];
    int servers_count = 0;
    char line[255];
    
    while (fgets(line, sizeof(line), file) && servers_count < 100) {
        line[strcspn(line, "\n")] = 0;
        
        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            strncpy(servers[servers_count].ip, line, sizeof(servers[servers_count].ip) - 1);
            servers[servers_count].port = atoi(colon + 1);
            if (servers[servers_count].port > 0) {
                servers_count++;
            }
        }
    }
    fclose(file);

    if (servers_count == 0) {
        fprintf(stderr, "No valid servers found in file\n");
        return 1;
    }

    printf("Found %d servers\n", servers_count);

    pthread_t threads[servers_count];
    struct ThreadData thread_data[servers_count];
    
    uint64_t chunk_size = k / servers_count;
    uint64_t remainder = k % servers_count;
    uint64_t current_start = 1;
    
    for (int i = 0; i < servers_count; i++) {
        thread_data[i].server = servers[i];
        thread_data[i].mod = mod;
        thread_data[i].index = i;
        thread_data[i].begin = current_start;
        
        uint64_t current_end = current_start + chunk_size - 1;
        if (i < (int)remainder) {
            current_end++;
        }
        if (current_end > k) current_end = k;
        
        thread_data[i].end = current_end;
        current_start = current_end + 1;
        
        printf("Server %d (%s:%d) will process [%lu-%lu]\n", 
               i, servers[i].ip, servers[i].port, 
               (unsigned long)thread_data[i].begin, 
               (unsigned long)thread_data[i].end);
        
        if (pthread_create(&threads[i], NULL, ProcessServer, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    uint64_t total_result = 1;
    for (int i = 0; i < servers_count; i++) {
        pthread_join(threads[i], NULL);
        
        if (thread_data[i].result != 0) {
            total_result = MultModulo(total_result, thread_data[i].result, mod);
        } else {
            printf("Warning: Server %d failed or returned 0\n", i);
        }
    }

    printf("\n=================================\n");
    printf("Result: %lu! mod %lu = %lu\n", 
           (unsigned long)k, (unsigned long)mod, (unsigned long)total_result);
    printf("=================================\n");
    
    return 0;
}