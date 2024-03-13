# Linux-For-Bignners

参考：[Linux 是怎样工作的](https://book.douban.com/subject/35768243/?from=tag)

操作系统准备：
* [WSL2](https://learn.microsoft.com/zh-cn/windows/wsl/install-manual#step-4---download-the-linux-kernel-update-package)
* Ubuntu 22.04.3 LTS

同时需要安装以下软件包：

* `binutils`
* `build-essential`:编译C/C++
* `sysstat`:收集系统相关信息

``
sudo apt install binutils build-essential sysstat
``

---

绘图软件包：

* `sudo apt install gnuplot` : 绘图包
* ` sudo apt install feh` : 图片显示器

---

`wsl vim` 与 `windows 剪贴板` 互通的解决方案：

在 vim 的命令模式下，首先：

* 输入 `v` ，进入 visual 模式
* 选中你需要复制的内容
* 输入命令：`:'<,'>w !clip.exe`

[参考](https://blog.csdn.net/AngelLover2017/article/details/122072001)

---

cgroupfs 辅助工具包

`sudo apt install cgroup-tools` 

**cgcreate**

```bash
cgcreate -g <subsystem_path>:<cgroup_name>
```

**cgdelete**

```bash
cgdelete -g <subsystem_path>:<cgroup_name>
```

