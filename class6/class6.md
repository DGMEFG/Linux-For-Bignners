## 高速缓存

**页面缓存**

与 CPU 访问内存速度比起来，访问外部存储器的速度慢了几个数量级。内核用于填补这一速度差的机构称为页面缓存。

页面缓存，将外部存储器上的文件数据以页为单位缓存到内存上。

当进程读取文件数据时，内核不会直接把文件数据复制到进程的内存中，而是将数据先复制到内核上页面缓存区域，然后将这些数据复制到进程的内存中，同时内核自身内存中有一个管理区域用于保存页面缓存的文件以及这些文件的范围信息。

![](https://pic.imgdb.cn/item/65e533d59f345e8d03180ce5.jpg)

与直接从外部存储器读取文件相比，从页面缓存读取速度更快，而且由于页面缓存的数据位于内核的内存里，读取的进程也可以是其它进程。

当进程向文件写入数据后，内核会把数据写入缓存，此时管理区域中数据的对应条目会被添加一个标记，表明 “这些是脏数据，其内容比外部存储器中的新” ，这些被标记的页面称为 **脏页**。

一定时间后，脏页中的数据将在指定时间通过内核的后台处理放映到外部存储器，此时，脏标记去除

![](https://pic.imgdb.cn/item/65e5355c9f345e8d031ba8b6.jpg)

只要系统还存在可用内存，各个进程访问那些尚未读取到页面缓存的文件时，页面缓存的大小就会随之增大。

当系统内存不足时，内核将释放页面缓存以空出可用空间。此时首先丢弃脏页以外的页面，如果还是无法空出足够内存，就对脏页执行写回，然后继续释放页面，由于需要访问外部存储器，系统性能将会大幅度下降。

**同步写入**

在页面缓存中还存在脏页的状态下，如果系统出现了强制断电，会导致页面缓存的脏页丢失。如果不希望文件访问出现这种情况，可以在使用 `open()` 系统调用打开文件将 `flag` 参数设定为 `O_SYNC` ，这样，之后每对文件执行 `write()` 系统调用，都会往页面缓存写入数据，将数据同步写入外部存储器。



> 缓存区缓存：当跳过文件系统，通过设备文件直接访问外部存储器使用的区域。

**读取文件的实验**

为了验证页面缓存的效果，接下来将对同一个文件执行两次读取操作，并对比两次处理消耗的时间。

首先创建一个 1GB 用于实验的名为 `testfile` 文件。

```bash
dd if=/dev/zero of=testfile oflag=direct bs=1M count=1K
```

输出为：

```bash
syz@syz:~/projects/linux/class6$ dd if=/dev/zero of=testfile oflag=direct bs=1M count=1K
1024+0 records in
1024+0 records out
1073741824 bytes (1.1 GB, 1.0 GiB) copied, 0.208929 s, 5.1 GB/s
```

接下来读取 `testfile`，并在读取前后打印内存相关信息：

```bash
syz@syz:~/projects/linux/class6$ free
               total        used        free      shared  buff/cache   available
Mem:         7942512      506128     7088108        3304      348276     7200804
Swap:        2097152           0     2097152
syz@syz:~/projects/linux/class6$ time cat testfile >/dev/null

real    0m0.519s
user    0m0.000s
sys     0m0.157s
syz@syz:~/projects/linux/class6$ free
               total        used        free      shared  buff/cache   available
Mem:         7942512      506652     6036512        3300     1399348     7199092
Swap:        2097152           0     2097152
```

运行时间为 ：`0.519s` 执行时间为：`0.157s` ，可以看到有大部分时间都是在不经过 cpu 处理中进行的，这部分时间用于等待外部存储器的读取操作。

同时，`buff/cache`(缓存区缓存与页面缓存占用的内存)从 `348276KB` 提升到 `1399348KB` 约为 `1GB`

接着执行第 `2` 次读取操作：

```bash
syz@syz:~/projects/linux/class6$ time cat testfile >/dev/null

real    0m0.295s
user    0m0.000s
sys     0m0.295s
syz@syz:~/projects/linux/class6$ free
               total        used        free      shared  buff/cache   available
Mem:         7942512      517908     6025184        3304     1399420     7187828
Swap:        2097152           0     2097152
syz@syz:~/projects/linux/class6$
```

此时可以看到主要是 cpu 时间占据主导，说明第 2 次全是在内存里操作，同时 `buff/cache` 字段没有明显变化。

页面缓存的总量不但能通过 `free` 命令获取，还能通过 `sar -r` 命令的 `kbcached` 字段获取，如果需要每隔一定时间观测一次数据，后者更加方便。

```bash
syz@syz:~/projects/linux/class6$ sar -r 1 1
Linux 5.15.133.1-microsoft-standard-WSL2 (syz)  03/04/24        _x86_64_        (20 CPU)

11:16:37    kbmemfree   kbavail kbmemused  %memused kbbuffers  kbcached  kbcommit   %commit  kbactive   kbinact   kbdirty
11:16:38      6005552   7182760    452488      5.70     14768   1363048    781624      7.79   1065032    495684         0
Average:      6005552   7182760    452488      5.70     14768   1363048    781624      7.79   1065032    495684         0
```

实验结束后，记得删除这个 `1GB` 文件

接下来我们来确定执行上述操作时，系统上的统计信息，主要采集以下 `3` 种信息：

* 在从外部存储器往页面缓存读入数据时，总共执行了多少次页面调入？
* 在从页面缓存往外部存储器写入数据时，总共执行了多少次页面调出？
* 外部存储器的 I/O 吞吐量是多少？

> read-twice.sh:

```bash
#!/bin/bash

rm -f testfile

echo "$(date): start file creation"
dd if=/dev/zero of=testfile oflag=direct bs=1M count=1K
echo "$(date): end file creation"

echo "$(date):sleep 3 seconds"
sleep 3

echo "$(date): start 1st read"
cat testfile >/dev/null
echo "$(date): end 1st read"

echo "$(date): start 2nd read"
cat testfile >/dev/null
echo "$(date):end 2nd read"

rm -f testfile
```

接着使用 `chmod +x read-twice.sh` 赋予其可执行权限。

`sar -B` 命令可以采集页面调入，调出的相关信息。因此可以考虑后台运行 `sar -B`，然后当前终端运行 `read-twice.sh`:

```bash
[bash1]
syz@syz:~/projects/linux/class6$ ./read-twice.sh
Mon Mar  4 11:43:02 CST 2024: start file creation
1024+0 records in
1024+0 records out
1073741824 bytes (1.1 GB, 1.0 GiB) copied, 0.207611 s, 5.2 GB/s
Mon Mar  4 11:43:02 CST 2024: end file creation
Mon Mar  4 11:43:02 CST 2024:sleep 3 seconds
Mon Mar  4 11:43:05 CST 2024: start 1st read
Mon Mar  4 11:43:06 CST 2024: end 1st read
Mon Mar  4 11:43:06 CST 2024: start 2nd read
Mon Mar  4 11:43:06 CST 2024:end 2nd read

[bash2]
syz@syz:~$ sar -B 1
Linux 5.15.133.1-microsoft-standard-WSL2 (syz)  03/04/24        _x86_64_        (20 CPU)

11:42:57     pgpgin/s pgpgout/s   fault/s  majflt/s  pgfree/s pgscank/s pgscand/s pgsteal/s    %vmeff
11:42:58         0.00      0.00      0.00      0.00     31.00      0.00      0.00      0.00      0.00
11:42:59         0.00      0.00      1.00      0.00      0.00      0.00      0.00      0.00      0.00
11:43:00         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
11:43:01         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
11:43:02         0.00 1048576.00   1448.00      0.00    804.00      0.00      0.00      0.00      0.00 #(1)
11:43:03         0.00      0.00      0.00      0.00      1.00      0.00      0.00      0.00      0.00
11:43:04         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
11:43:05     91904.00      0.00    346.00      0.00    131.00      0.00      0.00      0.00      0.00 #(2)
11:43:06    956672.00      0.00    851.00      0.00 263052.00      0.00      0.00      0.00      0.00 #(3)
11:43:07         0.00      0.00      0.00      0.00      1.00      0.00      0.00      0.00      0.00
11:43:08         0.00     80.00      2.00      0.00     38.00      0.00      0.00      0.00      0.00
11:43:09         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
11:43:10         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
11:43:11         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
```

* 创建文件，即(1)时刻，调出的页面总量为 `1G`(尽管我们禁用了页面缓存，直接从外部存储器写入文件数据，这部分数据流量也会被计入页面调出字段的数值中)
* 初次读取文件，(2) + (3)时刻，此时页面调入了 `(91904 + 956672)KB = 1GB`



接下来确认外部存储器的 I/O 吞吐量，使用 `sar -d -p` 命令

首先，需要确认作为写入目标的文件系统的名称，获取根文件系统所在的外部存储器名称：

```bash
syz@syz:~/projects/linux/class6$ mount | grep "on / "
/dev/sdc on / type ext4 (rw,relatime,discard,errors=remount-ro,data=ordered)
```

然后，我又试着查看这块磁盘的大小：

```bash
syz@syz:~/projects/linux/class6$ df -h | egrep "File|sdc" # egrep 支持正则
Filesystem      Size  Used Avail Use% Mounted on
/dev/sdc       1007G  2.3G  954G   1% /
```

> 我的实际物理磁盘空间不到512GB，这里一块磁盘就有 1TB，查阅好像是虚拟磁盘相关的技术。

回到正题，首先一个 bash 运行 `sar -d -p 1` ，另一个 bash 运行 `read-twice.sh`

```bash
[bash1]
syz@syz:~/projects/linux/class6$ ./read-twice.sh
Mon Mar  4 12:27:16 CST 2024: start file creation
1024+0 records in
1024+0 records out
1073741824 bytes (1.1 GB, 1.0 GiB) copied, 0.275396 s, 3.9 GB/s
Mon Mar  4 12:27:16 CST 2024: end file creation
Mon Mar  4 12:27:16 CST 2024:sleep 3 seconds
Mon Mar  4 12:27:19 CST 2024: start 1st read
Mon Mar  4 12:27:20 CST 2024: end 1st read
Mon Mar  4 12:27:20 CST 2024: start 2nd read
Mon Mar  4 12:27:20 CST 2024:end 2nd read

[bash2]
syz@syz:~$ sar -d -p 1
Linux 5.15.133.1-microsoft-standard-WSL2 (syz)  03/04/24        _x86_64_        (20 CPU)

12:27:11          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:12         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:12         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:12         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

12:27:12          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:13         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:13         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:13         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

12:27:13          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:14         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:14         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:14         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

12:27:14          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:15         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:15         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:15         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

12:27:15          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:16         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:16         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:16         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

12:27:16          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:17         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:17         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:17      1024.00      0.00 1048576.00      0.00   1024.00      0.21      0.21     29.00 sdc # 创建文件

12:27:17          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:18         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:18         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:18         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

12:27:18          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:19         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:19         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:19         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

12:27:19          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:20         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:20         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:20      2655.00 679680.00      0.00      0.00    256.00      0.36      0.13     27.00 sdc# 读取(1)

12:27:20          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:21         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:21         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:21      1443.00 368896.00     76.00      0.00    255.70      0.24      0.17     18.00 sdc# 读取(2)

12:27:21          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:22         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:22         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:22         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

12:27:22          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
12:27:23         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
12:27:23         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
12:27:23         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc
```

解释各个字段：

- **tps：** 表示每秒钟的传输次数 (number of transfers per second)，也就是每秒钟完成的 I/O 操作数。
- **rkB/s：** 表示每秒钟从磁盘读取的数据量 (kilobytes read per second)，即每秒钟从磁盘读取的数据总量。
- **wkB/s：** 表示每秒钟写入磁盘的数据量 (kilobytes written per second)，即每秒钟写入磁盘的数据总量。
- **dkB/s：** 表示每秒钟磁盘操作的数据量 (kilobytes transferred per second)，包括读取和写入的总数据量。
- **areq-sz：** 表示平均每个 I/O 操作的数据大小 (average request size)，即每个 I/O 操作平均涉及的数据量。
- **aqu-sz：** 表示平均队列长度 (average queue length)，即平均每个设备的 I/O 请求队列的长度。
- **await：** 表示平均 I/O 操作的等待时间 (average I/O wait time)，即完成一个 I/O 操作所花费的平均时间。
- **%util：** 表示磁盘利用率 (disk utilization)，即磁盘的工作负荷。
- **DEV：** 表示设备名称，即对应的磁盘设备。

同时可以得出结论：

* 创建文件时向为外部存储器写入了 `1048576KB`
* 初次读取文件，总共读取了，`679680KB + 368896KB`
* 第2次读取时候，外部存储器没有被读入

**写入文件的实验**

首先观察不使用直写模式和使用直写模式的区别：

```bash
syz@syz:~/projects/linux/class6$ rm -rf testfile
syz@syz:~/projects/linux/class6$ time dd if=/dev/zero of=testfile oflag=direct bs=100K count=1K
1024+0 records in
1024+0 records out
104857600 bytes (105 MB, 100 MiB) copied, 0.0804999 s, 1.3 GB/s

real    0m0.082s
user    0m0.014s
sys     0m0.008s
syz@syz:~/projects/linux/class6$ rm -rf testfile
syz@syz:~/projects/linux/class6$ time dd if=/dev/zero of=testfile bs=100K count=1K
1024+0 records in
1024+0 records out
104857600 bytes (105 MB, 100 MiB) copied, 0.0479697 s, 2.2 GB/s

real    0m0.049s
user    0m0.010s
sys     0m0.040s
syz@syz:~/projects/
```

> 注意这里仅仅写入 100MB ，因为我的 wsl2 系统上可用内存只有 7GB 左右，如果文件过大，系统会在采用页面缓存的同时，将文件写入磁盘，当文件越大这种影响越明显

接下来我们需要采集写入期间的统计信息

> write.sh

```bash
#!/bin/bash

rm -f testfile

echo "$(date): start write (file creation)"
dd if=/dev/zero of=testfile bs=100K count=1K
echo "$(date): end write"

rm -f testfile
```

同时后台运行 `sar -B` 来得到页面调入，调出的次数：

```bash
[bash1]
Mon Mar  4 15:16:17 CST 2024: start write (file creation)
1024+0 records in
1024+0 records out
104857600 bytes (105 MB, 100 MiB) copied, 0.0337664 s, 3.1 GB/s
Mon Mar  4 15:16:17 CST 2024: end write

[bash2]
syz@syz:~$ sar -B 1
Linux 5.15.133.1-microsoft-standard-WSL2 (syz)  03/04/24        _x86_64_        (20 CPU)

15:16:12     pgpgin/s pgpgout/s   fault/s  majflt/s  pgfree/s pgscank/s pgscand/s pgsteal/s    %vmeff
15:16:13         0.00      0.00      4.00      0.00     32.00      0.00      0.00      0.00      0.00
15:16:14         0.00     44.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
15:16:15         0.00      0.00      0.00      0.00      1.00      0.00      0.00      0.00      0.00
15:16:16         0.00      0.00     75.00      0.00      0.00      0.00      0.00      0.00      0.00
15:16:17         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 # 没有页面换出
15:16:18         0.00      0.00   1045.00      0.00  53178.00      0.00      0.00      0.00      0.00
15:16:19         0.00      0.00      0.00      0.00   1110.00      0.00      0.00      0.00      0.00
15:16:20         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00
15:16:21         0.00     43.56      0.00      0.00      0.99      0.00      0.00      0.00      0.00
```

* 此时成功达到预期，这种模式下，当文件占用内存不大的时候，不会同时将文件写入硬盘，从而一定程度提升了性能

同时使用 `sar -d -p 1` 观察磁盘吞吐率：

```bash
[bash1]
syz@syz:~/projects/linux/class6$ ./write.sh
Mon Mar  4 15:18:38 CST 2024: start write (file creation)
1024+0 records in
1024+0 records out
104857600 bytes (105 MB, 100 MiB) copied, 0.0362064 s, 2.9 GB/s
Mon Mar  4 15:18:38 CST 2024: end write

[bash2]
syz@syz:~$ sar -d -p 1
Linux 5.15.133.1-microsoft-standard-WSL2 (syz)  03/04/24        _x86_64_        (20 CPU)

15:18:35          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
15:18:36         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
15:18:36         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
15:18:36         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

15:18:36          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
15:18:37         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
15:18:37         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
15:18:37         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc

15:18:37          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
15:18:38         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
15:18:38         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
15:18:38         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc # <-

15:18:38          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
15:18:39         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
15:18:39         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
15:18:39         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc # <-

15:18:39          tps     rkB/s     wkB/s     dkB/s   areq-sz    aqu-sz     await     %util DEV
15:18:40         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sda
15:18:40         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdb
15:18:40         0.00      0.00      0.00      0.00      0.00      0.00      0.00      0.00 sdc
```

可以看到此时没有对于存储器写入的操作 (`wkB/s`)

## 调优参数

Linux 提供了很多用于控制页面缓存的调优参数：

**脏页的回写周期**

```bash
syz@syz:~$ sysctl vm.dirty_writeback_centisecs
vm.dirty_writeback_centisecs = 500
```

参数默认值为 500，即每 5secs 执行一次回写。

你可以通过：`sudo sysctl -w vm.dirty_writeback_centisecs=300` 进行修改。

> 请不要把参数设置为 `0`，否则周期性回写将会被禁用。

Linux 也会当系统内存不足时防止剧烈回写负荷的参数值，`vm.dirty_background_ratio` 可以指定一个百分值，当脏页所占的内存比例超过这个值就会执行回写操作，参数默认值为 `10%`

```bash
syz@syz:~$ sysctl vm.dirty_background_ratio
vm.dirty_background_ratio = 10
```

> `vm.dirty_background_bytes` 以字节为单位来设置该值，其默认值为 `0` 表示不启用

当脏页的内存占比超过 `vm.dirty_ratio` 参数指定的百分比值，将阻塞进程的写入，直到一定量的脏页被写回。

> 同样可以使用 `vm.dirty_bytes` 设置该值。

通过优化这些参数能够让系统不至于在内存不足的情况下出现大量的脏页回写，从而导致性能下降。

**清除页面缓存**

```bash
syz@syz:~$ sudo su
[sudo] password for syz:
root@syz:/home/syz# free
               total        used        free      shared  buff/cache   available
Mem:         7942512      539720     6969052        3368      433740     7166044
Swap:        2097152           0     2097152
root@syz:/home/syz# echo 3 >/proc/sys/vm/drop_caches
root@syz:/home/syz# free
               total        used        free      shared  buff/cache   available
Mem:         7942512      528912     7262540        3368      151060     7205288
Swap:        2097152           0     2097152
root@syz:/home/syz# exit
exit
syz@syz:~$
```

## 超线程

当我们使用 `time` 命令显示计入 `user` 和 `sys` 的 CPU 使用时间中，大部分时间浪费在等待数据从内存或高速缓存传输至 CPU 的过程。

`top` 命令的 `%cpu` 或者 `sar -P ` 命令中的 `%user`  和 `%system` 字段也包含等待数据传输的时间。通过超线程功能可以充分利用这些浪费等待上的 CPU 资源。

> 超线程与进程上的线程毫无关系。

使用超线程功能后，可以为 CPU 核心提供多份硬件资源(其中包括一部分 CPU 核心使用的硬件资源，例如寄存器)，然后将其划分为多个会被系统识别为逻辑 CPU 的超线程。符合这些特殊条件时，可以同时运行多个超线程。

> 超线程可能导致性能下降的例子通常涉及以下情况：
>
> 1. **密集型浮点运算：** 在进行大量浮点运算的情况下，超线程可能会导致性能下降。这是因为超线程技术只复制了部分 CPU 核心的硬件资源，对于某些密集型计算任务，两个逻辑处理单元共享了部分硬件资源，导致资源竞争和效率降低。
> 2. **大量内存访问的任务：** 对于需要大量内存访问的任务，超线程也可能导致性能下降。因为两个逻辑处理单元共享了同一个物理核心的缓存等资源，在大量内存访问的情况下可能会导致缓存争用，进而影响性能。
> 3. **同类任务并行执行：** 当两个逻辑处理单元同时执行相似的任务时，由于它们共享一些硬件资源，可能会导致资源竞争，从而降低整体性能。

搭建系统时，有必要实际施加负载，来比较一下超线程启用时和禁用时的性能区别，再确定是否启用该功能。



