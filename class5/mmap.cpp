#include <unistd.h>
#include <sys/mman.h>
#include <bits/stdc++.h>
#include <err.h>

constexpr int BUFFER_SIZE = 1000;
constexpr int ALLOC_SIZE = 100 * 1024 * 1024;

static char command[BUFFER_SIZE];

int main() {
	pid_t pid = getpid();
	snprintf(command, BUFFER_SIZE, "cat /proc/%d/maps", pid);

	std::cout << "*** Mem map before allocation ***" << std::endl;
	system(command);

	void *new_memory = mmap(NULL, ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (new_memory == (void *) -1)
		err(EXIT_FAILURE, "mmap() failed");
	std::cout << std::endl;

	printf("*** succeeded to allocate memory: address = %p; size = 0x%x ***\n", new_memory, ALLOC_SIZE);
	std::cout << std::endl;

	std::cout << "*** Mem map after allocation ***" << std::endl;
	system(command);

	if (munmap(new_memory, ALLOC_SIZE) == -1)
			err(EXIT_FAILURE, "munmap() failed");
	return 0;
			
}
