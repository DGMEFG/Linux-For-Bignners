#include <sys/types.h>
#include <sys/wait.h>
#include <bits/stdc++.h>
#include <unistd.h>
#include <err.h>
#include <time.h>
#include <memory>

constexpr uint64_t NLOOP_FOR_ESTIMATION = 1000000000ULL;
constexpr uint64_t NSECS_PER_MSEC = 1000000ULL;
constexpr uint64_t NSECS_PER_SEC = 1000000000ULL;

// timespec {tv_sec, tv_nsec} 秒数，纳秒

constexpr uint64_t calc(timespec t) {
	return t.tv_sec * NSECS_PER_SEC + t.tv_nsec;
}

static inline int64_t diff_nsec(timespec before, timespec after) {
	return calc(after) - calc(before);
}

static uint64_t loops_per_msec() {
	timespec before, after;
	clock_gettime(CLOCK_MONOTONIC, &before);
	
	// 执行空循环测试循环指定次数，需要的 CPU 运行时间
	for (uint64_t i = 0;i < NLOOP_FOR_ESTIMATION;i ++);

	clock_gettime(CLOCK_MONOTONIC, &after);
	
	// 返回每 ms 运行的循环的次数
	return NLOOP_FOR_ESTIMATION * NSECS_PER_MSEC / diff_nsec(before, after);
}

static inline void load(uint64_t nloop) {
	for (uint64_t i = 0;i < nloop;i ++ );
}

FILE *out1, *out2;
static void child_fn(int id, std::shared_ptr<timespec[]> buf, int nrecord, uint64_t nloop_per_resol, \
		timespec start, FILE* out1, FILE* pic2) {

	for (int i = 0;i < nrecord;i ++ ) {
		timespec ts;
		load(nloop_per_resol);// 模拟每个 resol 收集一次数据
		clock_gettime(CLOCK_MONOTONIC, &ts);
		buf[i] = ts;
	}
	// 进程id & 每个resol距开始的 ms 数 & 运行进度
	for (int i = 0;i < nrecord;i ++ ) {
		// printf("%d\t%ld\t%d\n", id, diff_nsec(start, buf[i]) / NSECS_PER_MSEC, \
		//     (i + 1) * 100 / nrecord);
		int finish_rate = (i + 1) * 100 / nrecord;
		uint64_t cost_time = diff_nsec(start, buf[i]) / NSECS_PER_MSEC;
		fprintf(out2, "%llu %d\n", cost_time, finish_rate);
		fprintf(out1, "%llu %d\n", cost_time, id);
	}
	exit(EXIT_SUCCESS);
}

std::shared_ptr<pid_t[]> pids;


// args = 命令行参数 + 1
int main(int args, char *argv[]) {
	out1 = fopen("tmp1.txt", "w"); // 打开名为tmp1.txt的文件，以写入模式打开
    	out2 = fopen("tmp2.txt", "w"); // 打开名为tmp2.txt的文件，以写入模式打开
	if (args !=  4) {
		fprintf(stderr, "usage: %s <nproc> <total[ms]> <resolution[ms]>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int nproc = atoi(argv[1]);
	int total = atoi(argv[2]);
	int resol = atoi(argv[3]);
	
	// std::cout << "pars: " << nproc << " " << total << " " << resol << std::endl;
	// 下面是对于命令行参数错误使用的输出
	
	std::vector<std::string> names = {"nproc", "total", "resol"};
	for (int i = 0;i < 3;i ++ ) {
		int x = atoi(argv[i + 1]);
		if (x < 1) {
			fprintf(stderr, "<%s>(%d) should be >= 1\n", names[i].c_str(), x);
			exit(EXIT_FAILURE);
		}
	}

	if (total % resol) {
		fprintf(stderr, "<total>(%d) should multiple of <resolution>(%d)\n", total, resol);
		exit(EXIT_FAILURE);
	}
	
	int nrecord = total / resol; // 收集信息的次数
	// 申请动态的 timespec 结构体指针
	std::shared_ptr<timespec[]> logbuf(new timespec[nrecord]);

	if (!logbuf) {
		err(EXIT_FAILURE, "new(logbuf) failed");
	}

	puts("estimating workload which takes just one milisecond");
	uint64_t nloop_per_resol = loops_per_msec() * resol;
	puts("end estimation");
	fflush(stdout);

	pids = std::shared_ptr<pid_t[]>(new pid_t[nproc]);
	if (!pids) {
		warn("new(pids) failed");
	}

	timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (int i = 0;i < nproc;i ++ ) {
		pids[i] = fork();
		if (pids[i] < 0) {
			// fork() 失败，代表实验失败，父进程杀掉所有子进程
			for (int j = 0;j < i;j ++ ) 
				if (kill(pids[i], SIGINT) < 0) 
					warn("kill (%d) failed", pids[i]);
			// 等待子进程全部退出，确保父进程回收所有子进程的资源
			// 正常来说
			for (int j = 0;j < i;j ++ ) 
				if (wait(NULL) < 0) 
					warn("wait() failed.");
			exit(EXIT_FAILURE);
		} else if (pids[i] == 0) {
			// 子进程
			child_fn(i, logbuf, nrecord, nloop_per_resol, start, out1, out2);
			// 此前该个子进程已经结束，要么在 wait(NULL) 后被收回资源，要么等待父进程收回
		}

	}

	// 运行到这，说明成功创建了 nproc 个并行子进程
	for (int i = 0;i < nproc;i ++ ) 
		if (wait(NULL) < 0) 
			warn("wait() failed.");
	;
	exit(EXIT_SUCCESS);
}



	
