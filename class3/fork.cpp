#include <unistd.h>
#include <err.h>
#include <bits/stdc++.h>

static void child() {
	std::cout << "[Child] pid is " << getpid() << '\n';
	exit(EXIT_SUCCESS);
}

static void parent(pid_t pid) {
	std::cout << "[Parent] pid is " << getpid() << " [Child] pid is " << pid << '\n';
	exit(EXIT_SUCCESS);
}

int main() {
	pid_t ret = fork();
	if (ret == -1) 
		err(EXIT_FAILURE, "fork() failed");
	if (ret == 0) {
		child();
	} else {
		parent(ret);
	}
	err(EXIT_FAILURE, "shouldn't reach here");
}


