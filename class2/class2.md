## 用户模式
OS 并非仅仅由内核(内核模式里运行的程序)构成，还包含许多在用户模式运行的程序。这些程序有的以库的形式存在，有的作为单独的进程而运行：

![](https://pic.imgdb.cn/item/65d1b5609f345e8d03ab0604.jpg)


### 1. 系统调用

进程如果要执行需要依赖于内核的处理(操作硬件)，需要通过系统调用向内核发出请求，一般有如下类型：

* 进程控制(创建和删除)
* 内存管理(分配和释放)
* 进程间通信
* 网络管理
* 文件系统操作
* 文件操作(访问设备)

用户模式下系统调用向内核发送请求，CPU 会发生中断，从用户模式切换到内核模式，处理完系统调用后，切换回用户模式。

![](https://pic.imgdb.cn/item/65d1b7b79f345e8d03b3f1fd.jpg)

> 期间，内核会检查来自进程的请求是否合理，不合理则系统调用失败；用户进程必须经过系统调用才能切换到 CPU 运行模式

#### 1.1 查看系统调用

使用 `strace` 命令可以对一个进程进行追踪。

例如，首先按照参考书，我们写一个简单的 hello.cpp:

![](https://pic.imgdb.cn/item/65d1b9ac9f345e8d03bbaed5.jpg)

接着编译 ` g++ -o hello hello.cpp` 得到可执行文件

依次执行：

* `./hello` : 直接执行
* `strace -o hello.log ./hello`: 执行并且使用 strace 追踪系统调用
* `cat hello.log` : 显示 hello.log 里的内容

![](https://pic.imgdb.cn/item/65d1bc039f345e8d03c3edbe.jpg)

strace 的运行结果的每一行对应一个系统调用，其中最直白且关键的内容是：

`write(1, "hello, world!\n", 14)         = 14`

> 该命令使用 write() 系统调用将 hello world\n 打印到屏幕上，14 代表写入了14字节的数据

接下来，使用 `python` 完成类似的操作，使用 `python` 编写一个输出 `hello world` 的程序，接着使用：

`strace -o hello.py.log python3 ./hello.py`

![](https://pic.imgdb.cn/item/65d1e0c69f345e8d0336b8d0.jpg)

然后注意到也出现了 : `write(1, "hello, world!\n", 14)         = 14`

![](https://pic.imgdb.cn/item/65d1e1139f345e8d03379abe.jpg)

> strace 追踪 python 的运行结果事实上比追踪 c++ 的运行结果要多很多。

> 对于具体某个系统调用的作用，可以使用 `man 2 write`(2 指定是查询系统调用)

### 2. 实验

`sar` 命令可以获取进程在用户模式下与内核模式下的运行时间占比，给出每个 CPU 核心运行的大致内容

`sar -P ALL 1`: 每隔 1 s显示每个CPU核心的使用率的统计信息，直到按下 `Ctrl + C`退出，并显示已采集的所有数据的平均值。

![](https://pic.imgdb.cn/item/65d1e5599f345e8d0345b89a.jpg)

* all 字段表示 20 个 CPU 核心各项数据的平均值。
* **%user** 以及 **%nice** 字段值相加是用户模式运行时间占比。
* **%system** 为 CPU 核心执行系统调用等处理占用的时间比例。
* 其它字段之后解释。

`sar -P ALL 1 1`: 第 4 个参数可以指定采样的次数。

#### Test One 
---

我们可以写一段不发起任何系统调用的仅执行循环的程序来观察其用户模式运行时间与内核模式运行时间占比。

![](https://pic.imgdb.cn/item/65d2feab9f345e8d0333fa37.jpg)

编译运行，然后后台挂起进程，如下图这里的进程编号为 `457`：

![](https://pic.imgdb.cn/item/65d2ff169f345e8d0335384e.jpg)

观察 `sar` 指令的输出:

![](https://pic.imgdb.cn/item/65d2ff8f9f345e8d0336bf04.jpg)

可以看到，该进程(只进行循环)，用户模式的占比达到 **100%**;实验结束后，使用 `kill id` 杀掉该程序的进程，比如我这里是 `kill 457`，杀掉该进程后，用户模式占比为 **0**.

#### Test Two
---

首先介绍 **<unistd.h>** 库(C/C++)里面的系统调用函数 `getppid()`，用于获取 **父进程** 的进程 ID。

> 一段运行于 shell 下的程序，其父进程为运行该 shell 的进程，因此，getppid() 可以得到 shell 的进程 ID。


**Small Test**

首先你可以在 shell 里使用 `echo $$` 获取当前 shell 的 ID，然后编写一段 C/C++ 程序，大致为：

![](https://pic.imgdb.cn/item/65d3032a9f345e8d0342ddba.jpg)

> 证毕，这为我们接下来的实验提供了一点基础。

接下来，我们编写一段用户模式和内核模式交替的程序，运行挂起后并使用 `sar` 指令查看 CPU 情况：

![](https://pic.imgdb.cn/item/65d30d589f345e8d0363703c.jpg)

> 注意，不要使用 `g++ ppidloop.cpp -o ppidloop && ./ppidloop &` ，因为这会创建一个新的 bash 进程来运行 ppidloop 程序，这样之后，它将返回新创建的 bash 进程的 ID，你需要使用 `ps` 命令查看其它多余进程并将其杀掉。

在第 7 块 CPU 核心上：
* 运行 ppidloop 程序本身占用 36% 的运行时间
* getppid() 这一系统调用占用了 64% 的运行时间
* %idle 字段为 0

![](https://pic.imgdb.cn/item/65d3095c9f345e8d03565511.jpg)

> 如果系统系调用很高，说明出现了类似如上这种导致系统负载过高的程序。

#### Tip

同时 `strace` 命令不仅可以追踪程序的系统调用，还可以得到其系统调用的时间：

![](https://pic.imgdb.cn/item/65d30f0b9f345e8d0369019c.jpg)

![](https://pic.imgdb.cn/item/65d30f2c9f345e8d036975f5.jpg)

可以看到调用 `write` 输出14字节的数据花了 40 微秒

### 3. 系统调用的包装函数

事实上对于 C/C++ 这种高级语言，并不能直接发起系统调用，只能通过汇编代码发起系统调用，对于 x86_64 架构，getppid() 通过如下代码发起：

```
mov $0x6e.%eax
syscall
```
> 首先将系统调用编号 0x6e 传递给 eax寄存器，然后 syscall 命令切换到内核模式，接下来执行负责处理 getppid 的内核代码

OS 提供了一些列被称为 **系统调用的包装函数** 的函数，从而避免了程序员必须为每一个系统调用编写相应的汇编代码，然后从高级语言调用这些代码，从而更好地提升了程序的可移植性，程序员只需要专注高级编程语言的书写即可。

![](https://pic.imgdb.cn/item/65d317ee9f345e8d0383eaaa.jpg)
 