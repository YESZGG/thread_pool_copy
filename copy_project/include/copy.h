/*
 * @Description: 
 * @version: 1.80.1
 * @Author: ZGG
 * @Date: 2023-08-05 10:45:08
 * @LastEditors: ZGG
 * @LastEditTime: 2023-08-05 15:15:57
 */
#ifndef _COPY_H__
#define _COPY_H__

#include <stdio.h>
#include <string.h>
#include <sys/types.h> // opendir
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h> // chdir
#include "thread_pool.h"

void *copy(void *arg); // 显示拷贝进程
void print_progress(double percentage);// 显示拷贝进度条
void get_dir_path(char *src_path, char *dest_path, thread_pool *pool);// 获取源文件路径 和 目标路径
void copy_stod(char *src_path, char *dest_path);// 拷贝函数

#endif