#include <sys/types.h>
#include <unistd.h>

int main() {
	while (true) getppid();
}
