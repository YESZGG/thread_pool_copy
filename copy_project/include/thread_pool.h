/*
 * @Description: 
 * @version: 1.80.1
 * @Author: ZGG
 * @Date: 2023-08-05 08:44:49
 * @LastEditors: ZGG
 * @LastEditTime: 2023-08-05 16:28:42
 */
#ifndef _THREAD_POOL_H__
#define _THREAD_POOL_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_WAITING_TASKS 1000 // 最大任务节点数
#define MAX_ACTIVE_THREADS 20  // 最大线程活跃数

struct task // 任务链表结构体
{
    void *(*do_task)(void *arg);
    void *arg;

    char src_path[4096];  // 存储源文件路径
    char dest_path[4096]; // 存储目标文件路径
    // 指针域
    struct task *next;
};

typedef struct thread_pool // 线程池结构体
{
    pthread_mutex_t lock;       // 互斥锁
    pthread_cond_t cond;        // 条件变量
    struct task *task_list;     // 任务类型结构体指针
    bool shutdown;              // 标识线程池是否开启
    pthread_t *tids;            // 线程id号
    unsigned max_waiting_tasks; // 最大等待处理的任务数
    unsigned waiting_tasks;     // 任务链表队列中等待的任务个数
    unsigned active_threads;    // 当前线程池中有多少个活动的线程（线程数量）
} thread_pool;

bool init_pool(thread_pool *pool, unsigned int threads_number);
bool add_task(thread_pool *pool, void *(*do_task)(void *arg), void *arg, char *src_path, char *dest_path);
void *routine(void *arg);
int add_thread(thread_pool *pool, unsigned additional_threads);
int remove_thread(thread_pool *pool, unsigned int removing_threads);
bool destroy_pool(thread_pool *pool);

#endif