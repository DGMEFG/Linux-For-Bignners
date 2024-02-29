#!/bin/bash

# 第一行告诉操作系统使用 bash 来执行该脚本
while true ; do 
	# 将当前日期字符串去掉换行符后，存在变量 DATE 
	DATE=$(date | tr -d '\n')
	# 获得 ps -eo xxx 中含 demand-paging 的一整行的结果, grep -v 作用暂且不知
	INFO=$(ps -eo pid,comm,vsz,rss,maj_flt,min_flt | grep demand-paging | grep -v grep)
	# 条件控制语句，如果 "INFO" 字段为空，则退出
	if [ -z "$INFO" ] ; then 
		echo "$DATE: target process seems to be finished"
		break
	fi 
	echo "${DATE}: ${INFO}"
	# 睡眠 1s 让人看起来舒服
	sleep 1
done
