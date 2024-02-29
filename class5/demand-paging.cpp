#include <unistd.h>
#include <bits/stdc++.h>
#include <err.h>
#include <time.h>

constexpr int BUFFER_SIZE = 100 * 1024 * 1024;
constexpr int NCYCLE = 10;
constexpr int PAGE_SIZE = 4096;

int main() {
	time_t t = time(NULL);char *s = ctime(&t);
	// 避免输出换行
	printf("[%.*s]: before allocation, please press Enter key\n", (int)(strlen(s) - 1), s);
	getchar();

	char *p = (char*) malloc(BUFFER_SIZE);// malloc 按照字节分配内存
	if (!p) err(EXIT_FAILURE, "malloc failed");

	t = time(NULL), s = ctime(&t);
	printf("[%.*s]: allocated %dMB, please Enter key\n", (int)(strlen(s) - 1), s,\
		       	BUFFER_SIZE / (1024 * 1024));
	getchar();
	
	// char 一个位置一个字节
	for (int i = 0;i < BUFFER_SIZE;i ++ ) {
		p[i] = 0;
		int cycle = i / (BUFFER_SIZE / NCYCLE);
		if (cycle != 0 && i % (BUFFER_SIZE / NCYCLE) == 0) {
			t = time(NULL), s = ctime(&t);
			printf("[%.*s]: touched %dMB\n", (int)(strlen(s) - 1), s, i / (1024 * 1024));
			sleep(1);// 休眠 1s 
		}
	}
	
	t = time(NULL), s = ctime(&t);
	printf("[%.*s]: touched %dMB, please press ENTER key\n", (int)(strlen(s) - 1), s,\
		       	BUFFER_SIZE / (1024 * 1024));
	getchar(), free(p);
	return 0;

}
