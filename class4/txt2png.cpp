#include <bits/stdc++.h>

int main(int args, char* argv[]) {
	
	// txt2png tmp1.txt res1.png
	if (args != 3) {
		std::cerr << "Usage expected: txt2png xxx[string] xxx[string]" << std::endl;
		exit(EXIT_FAILURE);
	}

	// windows 平台下已经将所有的 popen/pclose 换成 _popen/_pclose
	// 以管道的形式 使用 popen() 打开一个 gnuplot 的进程
	FILE *pipe = popen("gnuplot", "w");
	if (!pipe) {
		std::cerr << "Error executing gnuplot" << std::endl;
		exit(EXIT_FAILURE);
	}

	// 通过管道传输命令
	
	fprintf(pipe, "set terminal png\n");
	fprintf(pipe, "set output '%s'\n", argv[2]);
	fprintf(pipe, "plot '%s' with points\n", argv[1]);
	
	pclose(pipe);
	exit(EXIT_SUCCESS);
}
