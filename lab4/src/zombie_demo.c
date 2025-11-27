#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    printf("=== Демонстрация зомби-процессов ===\n");
    printf("Родительский процесс: PID = %d\n", getpid());
    
    pid_t child_pid = fork();
    
    if (child_pid < 0) {
        perror("Ошибка при создании процесса");
        return 1;
    } else if (child_pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс: PID = %d, PPID = %d\n", getpid(), getppid());
        exit(0); // Дочерний процесс завершается сразу
    } else {
        // Родительский процесс
        printf("Родительский создал дочерний с PID = %d\n", child_pid);
       
        // Дочерний процесс завершится, но родитель не заберет его статус
        // Он станет зомби на 30 секунд
        printf(" ~30 секунд...\n");
        
        sleep(30); // Зомби висит 30 секунд
        
        printf("Забираем статус дочернего процесса...\n");
        int status;
        waitpid(child_pid, &status, 0);
        printf("Зомби ликвидирован\n");
    }
    
    return 0;
}