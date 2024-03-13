## 文件系统

Linux 在访问外部存储器的数据时，通常不会直接访问而是通过文件系统进行访问。

外部存储器的功能，即将规定大小的数据写入外部存储器中指定的地址上；如果没有文件系统，这样的话如果你在内存中批量生成了 `10GB` 的数据写入外部存储器的指定地址上(下次你需要查阅的时候，你需要知道这个地址)，不仅要记录具体地址，大小以及这份数据的用处。

当需要保存多份数据，除了记录各个数据的相关信息，还需要管理可用区域：

![](https://pic.imgdb.cn/item/65ee7b609f345e8d03c98499.jpg)

有了 **文件系统** ，我们就可以避免这些繁杂的处理。

文件系统以文件为单位管理所有对用户有实际意义的数据块，并且为这些数据快添加上名称，位置和大小等辅助信息。它会规范了数据结果，以确定文件应该保存到什么位置，内核中的文件系统将依据该规范处理数据。**用户只需要记住文件的名称即可	**

**一个简单的文件系统**

* 文件清单从 `0GB` 的位置开始记录
* 记录每个文件的名称，位置和大小等信息

![](https://pic.imgdb.cn/item/65ee7d7f9f345e8d03d2c0b9.jpg)

用户只需要通过读取文件的系统调用，指定文件名，文件的偏移量以及文件大小，负责操作文件系统的处理即可找到相关数据给用户：

![](https://pic.imgdb.cn/item/65ee7e379f345e8d03d5f794.jpg)

### 1, Linux 的文件系统

Linux 设置了一种专门收纳其它文件的文件，**目录** ，即文件以及目录都可以放在一个目录下，不同目录下的多个文件可以取相同的名字：

![](https://pic.imgdb.cn/item/65ee7f429f345e8d03da679d.jpg)

Linux 能使用的文件系统不止一种，`ext4`，`XF4`,`Btrfs` 等不同文件系统可以共存于 Linux 上，这些文件系统在外部存储器中的数据结构以及用于处理数据的程序各不相同。各种文件系统所支持的文件大小以及各种文件操作 (创建，删除和读写) 的速度也不相同。

不管是哪个文件系统，面向用户的访问接口统一为以下系统调用：

* 创建与删除文件：`create(), unlink()`
* 打开与关闭文件：`open(), close()`
* 从已经打开的文件中读取数据：`read()`
* 往已打开的文件中写入数据：`write()`
* 将已打开的文件移动到指定的位置：`lseek()`
* 除了以上这些操作以外的依赖于文件系统的特殊处理：`ioctl()`

在请求这些系统调用时，将按照下列流程读取文件中的数据：

* 执行内核中全部文件系统的通用处理，并判断作为操作对象的文件保存在哪个系统上
* 调用文件系统专有的处理，执行与请求的系统调用相对应的处理
* 读写数据时，调用设备驱动程序执行操作
* 使用设备驱动程序执行数据的读写操作

![](https://pic.imgdb.cn/item/65ee82569f345e8d03e8e44b.jpg)

### 3. 数据与元数据

文件系统上存在两种数据类型，分别为 **数据** 与 **元数据**

* **数据**：用户创建的文档，图片，视频和程序等数据内容
* **元数据**：文件的名称，文件在外存的位置，文件大小等辅助信息

元数据分为以下几种：

* 种类：用于判断文件是保存数据的普通文件，还是目录或其他类型的文件信息
* 时间信息：包括文件的创建时间，最后一次访问的时间，以及最后一次修改的时间
* 权限信息，表明该文件允许哪个用户访问 (`chmod` 添加/减少权限)

通过 `df` 命令可以得到文件系统所用的存储空间，不但包括大家在文件系统上创建的所有文件所占用的空间，还包括所有元数据所占用的空间：

```bash
root@syz:/mnt# df
Filesystem      1K-blocks      Used Available Use% Mounted on
/dev/sdc       1055762868   2573964 999485432   1% /

root@syz:/mnt# for ((i=0;i<10000;i++)) ; do mkdir $i ; done
root@syz:/mnt# df
Filesystem      1K-blocks      Used Available Use% Mounted on
/dev/sdc       1055762868   2613964 999445432   1% /
root@syz:/mnt# for ((i=0;i<10000;i++)) ; do rm -rf $i ; done
root@syz:/mnt# df
Filesystem      1K-blocks      Used Available Use% Mounted on
/dev/sdc       1055762868   2575216 999484180   1% /
```

> /dev/sdc 文件系统挂载在根目录，可以监测在 /mnt 目录下创建文件/删除文件 的存储变化值

### 3. 容量限制

当系统被用于多种用途，如果存在某种用途占用文件系统的容量没有被限制，尤其是在 `root` 权限下运行的进程，当系统管理相关的处理所需的容量不足时，系统将无法正常运行。

为了避免这种情况，可以通过 **磁盘配额** 功能来限制各种用途的文件系统的容量，磁盘配额有以下几种类型：

* 用户配额：限制作为文件的使用者用户的可用容量。例如防止某个用户用光 `/home` 目录的存储空间
* 目录配额：限制特定目录的可用容量。例如限制项目成员共用的项目目录的可用容量。
* 子卷配额：限制文件系统名为子卷的单元的可用容量。大致上与目录配额的使用方式相同。

### 4. 文件系统不一致

只要使用系统，那么文件系统的内容就可能 **不一致** (数据，元数据以及各种关系与实际不符合)，比如，对外存读写文件被强制切断电源的情况就是一个典型的例子

首先我们观察 **移动目录**

![](https://pic.imgdb.cn/item/65eefbc29f345e8d03765773.jpg)

具体来说整个流程首先：

* 将 `bar` 连接到 `foo` 的下面
* 删除 `root` 与 `bar` 的链接

![](https://pic.imgdb.cn/item/65eefc669f345e8d037b87b1.jpg)

并且你会发现上面的每一步操作都是必须按照特定顺序执行的，将 `bar` 链接到 `foo` 的下面之后，却没有执行之后的删除操作，文件系统就会变成不一致的状态：

![](https://pic.imgdb.cn/item/65eefda19f345e8d0384c4e7.jpg)

我们实际上是想将 `bar` 归入 `foo` 目录下，然而文件系统却将 `bar` 与根目录以及 `foo` 目录都链接。

当文件系统发生不一致的情况时，这种不一致可能会导致文件系统在挂载或访问过程中出现问题：

1. **挂载时检测到不一致**：如果在挂载文件系统时检测到不一致，操作系统可能会拒绝挂载该文件系统，因为无法保证文件系统的完整性和数据一致性。这样做是为了避免进一步损坏文件系统或者导致数据丢失。
2. **访问过程中检测到不一致**：如果在文件系统被挂载后，在访问过程中检测到不一致，操作系统可能会以只读模式重新挂载该文件系统。这意味着用户只能读取文件而不能进行写操作，以防止进一步的数据损坏或错误。
3. **可能导致系统错误**：在最坏的情况下，文件系统的严重不一致可能导致系统崩溃或错误。如果文件系统中的关键数据结构严重损坏或不一致，可能会导致系统无法正常工作，甚至导致数据丢失或文件系统无法修复。

防止文件系统不一致的技术有很多，例如 **日志** 与 **写时复制**。`ext4` 与 `XFS` 使用日志，而 `Btrfs` 使用写时复制。

### 5. 日志

文件系统中提供了一个名为 **日志区域** 的特殊区域 (用户无法识别的元数据)。文件系统的更新将按照以下步骤进行：

* 把更新所需要的原子操作的概要暂时写入日志区域
* 基于日志区域的内容，进行文件系统的更新

例如：

![](https://pic.imgdb.cn/item/65eeffd79f345e8d0395c1e7.jpg)

一方面，如果更新日志记录的过程中被强制切断电源，只需要丢弃日志区域的数据即可，数据本身依旧没有改变。

另一方面，如果在实际执行数据更新的过程中被强制切断电源，只需要按照日志记录从头开始执行一边操作(重复操作不影响结果的正确)，即可。

### 6. 写时复制

在 `ext4` 和 `XFS` 等传统的文件系统上，文件一旦被创建，其位置原则上都不会改变

```bash
syz@syz:~$ df -T
Filesystem     Type           1K-blocks      Used Available Use% Mounted on
/dev/sdc       ext4          1055762868   2593748 999465648   1% /
```

与此相对，在 `Btrfs` 等利用写时复制的文件系统上，创建文件后的每一次更新处理都会把 **更新后的数据** 写入不同的位置。

### 7. 文件系统不一致的对策

各文件系统提供了一个通用的 `fsck` 命令，可以在没有定期备份的时候，利用该命令恢复。但是该命令有一些问题：

* 它会暴力遍历整个文件系统，这非常消耗时间
* `fsck` 命令只是将发生数据不一致的文件系统强行改变到可以挂载，这个过程中，一切不一致的数据与元数据都会被强制删除。

![](https://pic.imgdb.cn/item/65f026979f345e8d03468db2.png)

### 8. 文件的种类

除了之前提到的保存用户数据的普通文件，以及保存其它文件的目录，还有一种文件被称为 **设备文件**。

Linux 会把自身所处的硬件系统上几乎所有的设备呈现为文件形式，因此在 Linux 上，设备如同文件一般，可以通过 `open()`，`read()`，

`write()` 等系统调用进行访问。在需要执行设备特有的复杂操作时，就要使用 `ioctl()` 系统调用。通常情况下只有 `root` 用户才能访问设备文件。

Linux 主要将以文件形式存在的设备分为两种类型，分别为 **字符设备** 和 **块设备**，所有设备都保存在 `/dev` 目录下，设备的元数据一般保存以下信息：

* 文件的种类(字符设备和块设备)
* 设备的主设备号
* 设备的次设备号

```bash
syz@syz:~$ ls -l /dev
total 0
...
crw-rw-rw- 1 root tty       5,   0 Mar 12 17:49 tty
...
brw-rw---- 1 root disk      8,   0 Mar 12 17:49 sda
...
```

> 输出中行首字母为 `c` 表示为字符设备，`b` 为块设备；第 5，6 个字段分别为主设备号，与次设备号。
>
> 其它的形如：`lrwxrwxrwx` 以及 `drwxr-xr-x` 不属于设备。

### 9. 字符设备

字符设备虽然能执行读写操作，但是无法自行确定读取数据的位置，下面是一些具有代表性的字符设备：

* 终端
* 键盘
* 鼠标

以终端为例，我们可以对其设备文件执行下列操作：

* `write()`  系统调用，向终端输入数据
* `read()` 系统调用，从终端输入数据

下面考虑实际执行这些操作。首先寻找当前进程的终端，以及该终端对应的设备文件：

`ps ax` 的第 2 个字段即可知道该进程对应的设备文件。

```bash
syz@syz:~$ ps ax | grep bash
211 ?        Ss     0:00 /bin/bash /snap/ubuntu-desktop-installer/1286/bin/subiquity-server
357 pts/0    Ss     0:00 -bash
398 pts/1    S+     0:00 -bash
6524 pts/0    S+     0:00 grep --color=auto bash
syz@syz:~$ echo $$
357
```

因此，`bash` 关联的终端对应设备文件为 `/dev/pts/0` ，接着向该文件写入一个字符串

```bash
syz@syz:~$ sudo su
[sudo] password for syz:
root@syz:/home/syz# echo -e "hello world\r" >/dev/pts/0
hello world
```

接下来，可以尝试开启另一个终端，使用 `echo $$` 以及 `ps ax | grep bash` 得到相关信息，在 `root` 权限的终端往另一个 `bash` 的设备文件写入 `echo hello >/dev/pts/[pid]` ，然后打开另一个终端，发现命令行上出现了 `hello`。

一般的应用程序都是调用 Linux 提供的 shell 程序或者库，事实上这些操作来自于更底层对于设备文件的操作。

### 10. 块设备

块设备除了能执行普通的读写操作，还能进行随机访问，具有代表性的设备是 `HDD` 和 `SDD` 等外部存储器。只需要像读写文件一样读写块设备的数据，即可访问外部存储器中指定的数据。

正如之前提及，通常不会直接访问块设备，而是在设备上创建一个文件系统并将其挂载，然后通过文件系统进行访问，但是在以下几种情况下，需要直接操作块设备。

* 更新分区表 (利用 `parted` 命令)
* 块设备级别的数据备份与还原(利用 `dd` 命令)
* 创建文件系统 (利用各个文件系统的 `mkfs` 命令等)
* 挂载文件系统 (利用 `mount` 命令)
* `fsck`



### 11. 基于内存的文件系统

`tmpfs` 是一种创建与内存上的文件系统，虽然这个文件系统中保存的数据会在切断电源后消失，但由于该文件系统不需要访问外部存储器，所以能提高访问速度。

```bash
root@syz:/home/syz/projects# mount | grep ^tmpfs
tmpfs on /sys/fs/cgroup type tmpfs (ro,nosuid,nodev,noexec,size=4096k,nr_inodes=1024,mode=755)
tmpfs on /mnt/wslg/run/user/0 type tmpfs (rw,nosuid,nodev,relatime,size=794260k,nr_inodes=198565,mode=700)
tmpfs on /run/user/0 type tmpfs (rw,nosuid,nodev,relatime,size=794260k,nr_inodes=198565,mode=700)
```

`tmpfs` 创建于挂载的时候，通过 `size` 选项指定最大容量，不过，并不是说一开始就使用指定内存量，而是在访问文件系统的区域，以页为单位申请相应大小的内存。通过查看 `free` 命令中输出的 `shared` 字段的值，可以得知 `tmpfs` 实际占用内存量。

```bash
syz@syz:/$ free
               total        used        free      shared  buff/cache   available
Mem:         7942608      554416     6875796        3132      512396     7146112
Swap:        2097152           0     2097152
```

> 3MB

### 12. 网络文件系统

**网络文件系统** 可以通过网络来访问远程主机的文件：

![](https://pic.imgdb.cn/item/65f04a209f345e8d03f05dd1.png)

基本上，在访问 `Windows` 主机上的文件，使用名为 `cifs` 的文件系统，在访问搭载 `Linux` 等类 `UNIX` 主机上的文件，则使用 `nfs` 文件系统

### 13. 虚拟文件系统

系统上存在着各种各样的文件系统，用于获取内核中的各种信息，以及更改内核的行为。

#### `procfs`

`procfs` 用于获取系统上的所有进程的信息，通常被挂载载 `/proc` 目录下。通过访问 `/proc/[pid]` 目录下的文件，即可获取各个进程的信息。

```bash
syz@syz:~$ ls /proc/$$ # 用于获取当前 bash 进程相关的信息
arch_status  coredump_filter  gid_map    mountinfo   oom_score_adj  sessionid     status          wchan
attr         cpuset           io         mounts      pagemap        setgroups     syscall
auxv         cwd              limits     mountstats  personality    smaps         task
cgroup       environ          loginuid   net         projid_map     smaps_rollup  timens_offsets
clear_refs   exe              map_files  ns          root           stack         timers
cmdline      fd               maps       oom_adj     sched          stat          timerslack_ns
comm         fdinfo           mem        oom_score   schedstat      statm         uid_map
```

其中：

* `/proc/pid/maps` :进程的内存映射
* `/proc/pid/cmdline`:进程的命令行参数
* `/proc/pid/stat`:进程的状态，CPU 时间，有限级，和内存使用量。

#### `sysfs`

Linux 引入 `procfs` 后，越来越多信息塞入其中，导致出现许多与进程无关的信息，为了防止 `procfs` 被滥用，Linux 引入了一个名为 `sysfs` 的文件系统，用来存放那些信息。`sysfs` 通常挂在与 `/sys` 目录下。

>**挂载**
>
>当一个文件系统被挂载（mount）到 `/xxx` 目录下时，意味着该文件系统的内容将被映射到系统中的 `/xxx` 这个目录中，使得该文件系统的内容可以在 `/xxx` 目录下访问和操作。
>
>具体来说，挂载是将一个存储设备（如硬盘、分区、光盘等）或者虚拟文件系统（比如tmpfs）连接到文件系统中某个目录的过程。一旦挂载完成，该存储设备或文件系统的内容就会在指定的挂载点（这里是 `/xxx` 目录）显示出来，用户就可以通过这个挂载点来访问和管理文件系统中的数据。
>
>举个例子，如果你将一个外部硬盘挂载到 `/media/external` 目录下，那么外部硬盘的内容就会在 `/media/external` 中可见，你就可以在这个目录下查看、读取或写入外部硬盘中的文件了。

`sysfs` 包括以下文件：

* `/sys/devices` 目录下的文件：搭载于系统上的设备相关信息
* `/sys/fs` 目录下的文件：系统上的各种文件系统的相关信息

#### `cgroupfs`

`cgroupfs` 用于限制单个进程或者多个进程组成的群组的资源使用量，需要通过文件系统 `cgroupfs` 来操作。另外只有 `root` 用户可以操作 `cgroup` 。`cgroupfs` 通常挂载到 `/sys/fs/cgroup`

通过 `cgroup` 添加的限制资源有很多种，举例如下：

* `CPU` 设置能够使用的比例，比如令某群组只能使用 CPU 资源的 50%。通过读写 `/sys/fs/cgroup/cpu` 目录下的文件进行控制
* `内存`限制群组的物理内存使用量，比如令某群组最多只能够使用 `1GB` 内存，通过读写 `/sys/fs/cgroup/memory` 目录下的文件进行控制

> 在 Linux 系统中，cgroup（Control Groups）是一种内核功能，用于限制、控制和监视一个或多个进程的资源使用。cgroup可以将一组进程组织在一起，并为这个组指定特定的资源限制，以控制它们对系统资源（如CPU、内存、磁盘IO等）的访问。
>
> 群组（group）在cgroup中指的就是这样一组相关的进程，这些进程共享同样的资源限制。通过将进程分组，我们可以更方便地对一组进程应用相同的资源控制策略。

比如：我们要限制一些进程每 100ms 只能使用 20ms CPU 运行时：

进入`/sys/fs/cgroup/cpu` 下创建子目录 `cgroup-cpu`：

```bash
root@syz:/sys/fs/cgroup/cpu# mkdir cgroup-cpu
root@syz:/sys/fs/cgroup/cpu# ls
cgroup-cpu             cgroup.sane_behavior  cpu.cfs_quota_us  cpu.rt_runtime_us  notify_on_release
cgroup.clone_children  cpu.cfs_burst_us      cpu.idle          cpu.shares         release_agent
cgroup.procs           cpu.cfs_period_us     cpu.rt_period_us  cpu.stat           tasks

root@syz:/sys/fs/cgroup/cpu# cd cgroup-cpu/
root@syz:/sys/fs/cgroup/cpu/cgroup-cpu# ls
cgroup.clone_children  cpu.cfs_burst_us   cpu.cfs_quota_us  cpu.rt_period_us   cpu.shares  notify_on_release
cgroup.procs           cpu.cfs_period_us  cpu.idle          cpu.rt_runtime_us  cpu.stat    tasks
```

> 该目录下会自动将 cgroupfs 文件系统的 subsystem cpu 挂载到子目录 `cgroup-cpu` 下

执行：

```bash
root@syz:/sys/fs/cgroup/cpu/cgroup-cpu# while : ; do : ; done &
[1] 54355
```

观察资源占用：

```bash
root@syz:/sys/fs/cgroup/cpu/cgroup-cpu# sar -P ALL 1 1
Linux 5.15.146.1-microsoft-standard-WSL2 (syz)  03/12/24        _x86_64_        (20 CPU)

21:32:39        CPU     %user     %nice   %system   %iowait    %steal     %idle
...
21:32:40          2    100.00      0.00      0.00      0.00      0.00      0.00
...
```

接着使用 `cgroups ` 提供的功能，限制其资源占用：

```
root@syz:/sys/fs/cgroup/cpu/cgroup-cpu# echo 54355 > ./tasks # 将脚本进程编号写入群组
root@syz:/sys/fs/cgroup/cpu/cgroup-cpu# echo 20000 > ./cpu.cfs_quota_us # 每 100000 us 只能占用 20000 us
root@syz:/sys/fs/cgroup/cpu/cgroup-cpu# sar -P ALL 1 1 
Linux 5.15.146.1-microsoft-standard-WSL2 (syz)  03/12/24        _x86_64_        (20 CPU)

21:35:10        CPU     %user     %nice   %system   %iowait    %steal     %idle
...
21:35:11          2     20.00      0.00      0.00      0.00      0.00     80.00
...
```

接着，删除该群组(不能直接递归删除对应目录，因为目录中的文件是虚拟的，递归删除时会报错)

使用，然后又是占用 `100%`：

```bash
root@syz:/sys/fs/cgroup/cpu# cgdelete cpu::cgroup-cpu
root@syz:/sys/fs/cgroup/cpu# sar -P ALL 1 1
Linux 5.15.146.1-microsoft-standard-WSL2 (syz)  03/12/24        _x86_64_        (20 CPU)

21:37:54        CPU     %user     %nice   %system   %iowait    %steal     %idle
...
21:37:55          2    100.00      0.00      0.00      0.00      0.00      0.00
...
```

最后关闭循环进程 `kill`

#### `Btrfs`

`ext4` 与 `XFS` 存在些许差别，两者都是从 `UNIX` 诞生之初就存在的文件系统。接下来介绍新一点的文件系统 `Btrfs`

**多物理卷**

在 `ext4` 和 `XFS` 上需要为每个分区创建文件系统，但 `Btrfs` 不需要这样。`Btrfs` 可以创建一个包含多个外部存储器或分区的存储池，然后再存储池上创建可挂载的区域，**子卷**。

存储池相当于 `LVM` 的卷组，而子卷类似 `LVM` 种逻辑卷与文件系统的融合。

![](https://pic.imgdb.cn/item/65f160ca9f345e8d03ca8d94.png)

 `Btrfs` 甚至可以向现存的文件系统添加，删除以及替换外部存储器，即使这些操作导致容量发生变化，也无须调整文件系统的大小，另外，这些操作**可以在文件系统已经挂载的状态下**进行，无需卸载文件系统：

![](https://pic.imgdb.cn/item/65f161c29f345e8d03ce8b8c.png)

**快照**

`Btrfs` 可以以子卷为单位创建快照，创建快照不需要复制所有数据，只需要根据数据创建其元数据，并且回写快照内的脏页即可。而且，由于快照与子卷共享数据，创建快照的存储空间成本也很低。

![](https://pic.imgdb.cn/item/65f162de9f345e8d03d36418.png)

快照其实只创建了一个新的目录节点，然后添加了指向下一层的节点的链。

**RAID**

`Btrfs` 可以在文件系统级别上配置 `RAID` ，支持的 `RAID` 级别有 `RAID0`，...，`RAID6` 以及 `dup` 。在配置 `RAID` 不是以子卷为单位，而是以整个 `Btrfs` 文件系统为单位。

首先观察没有配置 `RAID` 的结构，假设一个仅由 `sda` 构成的单个设备 `Btrfs` 文件系统上存在一个子卷 `A` 。这种情况下，一旦 `sda` 出现错误，子卷 `A` 上所有数据就会丢失。

![](https://pic.imgdb.cn/item/65f164569f345e8d03da37b5.png)

如果配置了 `RAID1` ，那么所有数据就会被写入两台外部存储器 (这里记为 `sda` 和 `sdb`) ，即便 `sda` 挂掉了，`sdb` 也还保留着一份子卷 `A` 的数据

![](https://pic.imgdb.cn/item/65f164bf9f345e8d03dc13b6.png)

**数据损坏的检测与修复**

当外部存储器的部分数据损坏时，`Btrfs` 能够检测出来，如果配置了某些级别的 `RAID`  ，还能修复这些数据。在那些哪些没有这类功能的文件系统中，即便写入外存的数据因为比特差错等损坏了，也无法检测出这些损坏的数据，而是在损坏的情况下继续运行。

![](https://pic.imgdb.cn/item/65f165ed9f345e8d03e0e133.png)

`Btrfs` 拥有一种称为 **校验和** 的机制，用于检测数据与元数据的完整性，这种机制可以检测出数据损坏。在读取数据或者元数据出现校验和错误，会丢弃这部分数据，并通知发出请求的进程出现了 I/O 异常。

![](https://pic.imgdb.cn/item/65f166929f345e8d03e3a232.png)

检测到损坏后，如果配置的是 `RAID` 级别是 `RAID1` ,`RAID10` ，`RAID5`，`RAID6` 之一，`Btrfs` 就能通过校验和正确的另一份数据副本修复破损的数据。

虽然 ext4 与 XFS 可以通过为元数据附加校验和来检测并丢弃损坏的元数据，但是对数据的检测、丢弃与修复功能则只有 Btrfs 提供
