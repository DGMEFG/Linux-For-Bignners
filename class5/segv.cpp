#include <iostream>

int main() {
	int *p = nullptr;
	puts("before invalid access");
	*p = 0;
	puts("after invalid access");
	return 0;
}
