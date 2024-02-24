#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        // fork failed
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // child process
        printf("Child process is running with PID: %d\n", getpid());
        sleep(2); // 模拟子进程执行一段时间
        printf("Child process is exiting\n");
        exit(0);
    } else {
        // parent process
        printf("Parent process is running with child PID: %d\n", pid);
        sleep(10); // 等待一段时间，确保子进程已经结束
        int result = kill(pid, SIGUSR1); // 向子进程发送SIGUSR1信号
        if (result == 0) {
            printf("Signal successfully sent to child process\n");
        } else {
            perror("kill");
        }
        wait(NULL); // 等待子进程结束
        printf("Parent process is exiting\n");
    }

    return 0;
}

