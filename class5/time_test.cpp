#include <time.h>
#include <iostream>

int main() {
	time_t now = time(NULL);
	std::cout << "Time: " << now << std::endl;
	
	char *str = ctime(&now);
	std::cout << str << std::endl;
	return 0;
}
