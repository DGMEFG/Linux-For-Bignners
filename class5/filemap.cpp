#include <unistd.h>
#include <sys/mman.h>
#include <err.h>
#include <iostream>
#include <fcntl.h> // open() close() 函数归属处
#include <cstring>

constexpr int BUFFER_SIZE = 1000;
constexpr int ALLOC_SIZE = 100 * 1024 * 1024;

char command[BUFFER_SIZE], file_contents[BUFFER_SIZE], overwrite_data[] = "HELLO";

int main() {
	pid_t pid = getpid();
	snprintf(command, BUFFER_SIZE, "cat /proc/%d/maps", pid);

	std::cout << "Mem map before mapping file" << std::endl;
	system(command);

	int fd = open("testfile", O_RDWR);
	if (fd == -1)
		err(EXIT_FAILURE, "open() falied");
	// 这里必须强转
	char *file_contents = (char*)mmap(NULL, ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (file_contents == (void *) -1) {
			warn("mmap() failed");
			if (close(fd) == -1) 
				warn("close() falied");
			return 0;
	}

	std::cout << std::endl;
	printf("*** succeeded to map file : address = %p; size = 0x%x ***\n", file_contents, ALLOC_SIZE);
	std::cout << std::endl;

	std::cout << "*** Mem map after mapping ***" << std::endl;
	system(command);
	std::cout << std::endl;

	printf("*** file contents before overwrite mapped region: %s", file_contents);

	// 改写映射文件
	memcpy(file_contents, overwrite_data, strlen(overwrite_data));
	std::cout << std::endl;

	printf("*** overwriten mappped region with : %s\n", file_contents);

	if (munmap(file_contents, ALLOC_SIZE) == -1)
			warn("munmap() failed");
	if (close(fd) == -1) 
		warn("close() failed");
	return 0;
	
}

