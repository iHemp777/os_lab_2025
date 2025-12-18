#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

void* thread1_function(void* arg) {
    pthread_mutex_lock(&mutex1);
    printf("Поток 1: Захватил мьютекс 1\n");
    
    sleep(1);
    
    printf("Поток 1: Жду мьютекс 2...\n");
    pthread_mutex_lock(&mutex2);
    printf("Поток 1: Захватил мьютекс 2 → Работа завершена\n");
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

void* thread2_function(void* arg) {
    pthread_mutex_lock(&mutex2);  // РАЗНЫЙ порядок! → DEADLOCK
    printf("Поток 2: Захватил мьютекс 2\n");
    
    sleep(1);
    
    printf("Поток 2: Жду мьютекс 1...\n");
    pthread_mutex_lock(&mutex1);
    printf("Поток 2: Захватил мьютекс 1 → Работа завершена\n");
    
    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);
    return NULL;
}

// БЕЗОПАСНАЯ версия: ОДИНАКОВЫЙ порядок
void* thread_safe_function(void* arg) {
    int thread_id = *(int*)arg;
    
    pthread_mutex_lock(&mutex1);  // СНАЧАЛА мьютекс 1
    printf("Поток %d: Захватил мьютекс 1\n", thread_id);
    
    sleep(1);
    
    printf("Поток %d: Захватываю мьютекс 2...\n", thread_id);
    pthread_mutex_lock(&mutex2);  // ПОТОМ мьютекс 2
    printf("Поток %d: Захватил мьютекс 2 → Работа завершена\n", thread_id);
    
    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);
    return NULL;
}

int main(int argc, char* argv[]) {
    pthread_t thread1, thread2;
    
    printf("=== ДЕМОНСТРАЦИЯ DEADLOCK ===\n\n");
    
    if (argc > 1 && strcmp(argv[1], "--safe") == 0) {
        printf("БЕЗОПАСНЫЙ РЕЖИМ: одинаковый порядок захвата\n\n");
        
        int id1 = 1, id2 = 2;
        
        pthread_create(&thread1, NULL, thread_safe_function, &id1);
        pthread_create(&thread2, NULL, thread_safe_function, &id2);
        
        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);
        
        printf("\nОба потока завершились успешно!\n");
        
    } else {
        printf("РЕЖИМ С DEADLOCK: разный порядок захвата\n\n");
        
        pthread_create(&thread1, NULL, thread1_function, NULL);
        pthread_create(&thread2, NULL, thread2_function, NULL);
        
        sleep(3);
        printf("\nDEADLOCK! Программа зависла (Ctrl+C для выхода)\n");
        
        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);
    }
    
    pthread_mutex_destroy(&mutex1);
    pthread_mutex_destroy(&mutex2);
    return 0;
}
