## 进程调度器

对于调度器，一般概括为：

* 一个 CPU 同时只运行一个进程
* 在同时运行多个进程时，每个进程都会获得适当的时长，**轮流**在 CPU 上执行 (轮流的具体方式下面会通过实验的方式得到)

这里指出，Linux 会将多核 CPU 的每个核心是做一个 CPU (这里称呼其为 **逻辑CPU**)

### 1. 设计实验程序

在同时运行一个或多个一味消耗 CPU 时间执行处理的进程时，采集：

* 在某一时间点运行在逻辑 CPU 上的进程是哪一个
* 每个进程的运行进度

> 实验要求：

* 命令行参数
  * 第 1 个参数 (n)：同时运行的进程数量
  * 第 2 个参数 (total)：程序运行的总时长（单位：毫秒）
  * 第 3 个参数 (resol)：采集统计信息的间隔（单位：毫秒）
  * 令 n 个进程同时运行，然后在全部进程都结束后结束程序的运行。各个进程按照以下要求运行：
    * 在消耗 total 毫秒的 CPU 时间后结束运行
    * 每 resol 毫秒记录以下 3 个数值：
      * 每个进程唯一的 ID（0 ~ n - 1 各个进程独有的编号）
      * 从程序开始运行到记录的时间点为止经过的时间（单位：毫秒）
      * 进程的进度（单位：%）
    * 在结束运行后，把所有统计信息用制表符分隔并逐行输出

> 实验代码

```c++
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
```

编译并且运行后，产生两个文件，分别存储