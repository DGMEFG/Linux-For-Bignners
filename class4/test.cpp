#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>

int main() {
    // 创建6个子进程
    for (int i = 0; i < 6; ++i) {
        pid_t pid = fork();

        if (pid == 0) {
            // 子进程逻辑
            std::cout << "Child process " << getpid() << " is running." << std::endl;
            sleep(1);  // 模拟子进程执行一段时间
            std::cout << "Child process " << getpid() << " is exiting." << std::endl;
            return 0;
        }
    }

    // 父进程逻辑
    for (int i = 0; i < 7; ++i) {
        int status;
        pid_t terminated_child_pid = wait(&status);

        if (WIFEXITED(status)) {
            std::cout << "Child process " << terminated_child_pid << " exited with status " << WEXITSTATUS(status) << std::endl;
        } else if (WIFSIGNALED(status)) {
            std::cout << "Child process " << terminated_child_pid << " terminated by signal " << WTERMSIG(status) << std::endl;
        }
    }

    return 0;
}

