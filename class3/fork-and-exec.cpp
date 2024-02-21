#include <unistd.h>
#include <bits/stdc++.h>
#include <err.h>

// int execve(const char *filename, char *const argv[], char *const envp[]);
// filename : 新程序的路径和文件名
// argv 包含了要传递给新程序的命令行参数，其中第一个参数通常是程序名。
// envp 包含了新程序的环境变量。

static void child() {
	char *args[] = {"/bin/echo/", "hello", NULL};
	std::cout << "[child] pid is " << getpid() << std::endl;
	execve("/bin/echo", args, NULL);
	// echo 自带退出功能，因此不会往下执行了。
	err(EXIT_FAILURE, "exec() failed");
}

static void parent(pid_t pid_c) {
	std::cout << "[parent] pid is " << getpid() << " and [child] pid is " << pid_c << std::endl;
	exit(EXIT_SUCCESS);
}

int main() {
	pid_t ret = fork();
	if (ret == -1) 
		err(EXIT_FAILURE, "fork() failed");
	if (ret == 0) child();
	else parent(ret);

	err(EXIT_FAILURE, "shouldn't reach here");
}


