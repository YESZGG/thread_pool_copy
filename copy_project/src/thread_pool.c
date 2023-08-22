/*
 * @Description:
 * @version: 1.80.1
 * @Author: ZGG
 * @Date: 2023-08-05 08:45:06
 * @LastEditors: ZGG
 * @LastEditTime: 2023-08-05 16:38:32
 */
#include "thread_pool.h"

void handler(void *arg)
{
    pthread_mutex_unlock((pthread_mutex_t *)arg); // 解锁
}
// 线程池的初始化
bool init_pool(thread_pool *pool, unsigned int threads_number)
{
    // 1. 初始化线程池互斥锁
    pthread_mutex_init(&pool->lock, NULL);

    // 2. 初始化条件变量
    pthread_cond_init(&pool->cond, NULL);

    // 3. 初始化标志位为false，代表当前线程池正在运行。
    pool->shutdown = false;

    // 4. 初始化任务队列的头节点
    pool->task_list = malloc(sizeof(struct task));

    // 5. 为储存线程ID号申请空间。
    pool->tids = malloc(sizeof(pthread_t) * MAX_ACTIVE_THREADS);

    // 第4步与第5步错误判断
    if (pool->task_list == NULL || pool->tids == NULL)
    {
        perror("allocate memory error");
        return false; // 初始化线程池失败
    }

    // 6. 为线程池任务队列的头节点的指针域赋值NULL
    pool->task_list->next = NULL; // 单向链表

    // 7. 设置线程池最大任务个数为1000
    pool->max_waiting_tasks = MAX_WAITING_TASKS;

    // 8. 当前需要处理的任务为0
    pool->waiting_tasks = 0;

    // 9. 初始化线程池中线程的个数
    pool->active_threads = threads_number;

    // 10. 创建线程
    int i;
    for (i = 0; i < pool->active_threads; i++)
    {
        // (void *)pool 表示将线程池的地址作为参数传递给线程函数。
        if (pthread_create(&((pool->tids)[i]), NULL, routine, (void *)pool) != 0)
        {
            perror("create threads error");
            return false;
        }
    }

    // 11. 线程池初始化成功
    return true;
}

bool add_task(thread_pool *pool, void *(*do_task)(void *arg), void *arg, char *src_path, char *dest_path)
{
    // 1. 为新任务的节点申请空间
    struct task *new_task = malloc(sizeof(struct task));
    if (new_task == NULL)
    {
        perror("allocate memory error");
        return false;
    }

    // 2. 为新节点的数据域与指针域赋值
    new_task->do_task = do_task; // 将传入的任务执行函数 do_task赋值给新任务节点 do_task成员
    new_task->arg = arg;         // 将传入的任务参数 arg 赋值给新任务节点 arg成员
    // 将传入的源文件路径和目标文件路径赋值给新任务节点
    strcpy(new_task->src_path, src_path);
    strcpy(new_task->dest_path, dest_path);

    new_task->next = NULL;

    // 3.  在添加任务之前，必须先上锁，因为添加任务属于访问临界资源 -> 任务队列
    pthread_mutex_lock(&pool->lock);

    // 4. 如果当前需要处理的任务个数>=1000
    if (pool->waiting_tasks >= MAX_WAITING_TASKS)
    {
        // 解锁
        pthread_mutex_unlock(&pool->lock);

        // 打印一句话提示一下，太多任务了
        fprintf(stderr, "too many tasks.\n");

        // 释放掉刚刚准备好的新节点
        free(new_task);

        return false;
    }

    // 5. 寻找任务队列中的最后一个节点(尾插)
    struct task *tmp = pool->task_list;
    while (tmp->next != NULL)
        tmp = tmp->next;
    // 从循环中出来时，tmp肯定是指向最后一个节点

    // 6. 让最后一个节点的指针域指向新节点
    tmp->next = new_task;

    // 7. 当前需要处理的任务+1
    pool->waiting_tasks++;

    // 8. 添加完毕，就解锁。
    pthread_mutex_unlock(&pool->lock);

    // 9. 唤醒条件变量中的一个线程起-来做任务(单播)
    pthread_cond_signal(&pool->cond);

    return true;
}   

void *routine(void *arg)
{
    // 1. 先接收线程池的地址
    thread_pool *pool = (thread_pool *)arg;
    // 定义任务链表指针
    struct task *p;

    while (1)
    {
        // 2. 线程取消例程函数
        // 将来要是有人要取消我，由于已经上了锁，所以定义的线程取消例程函数，请先把锁解开，然后再响应取消。
        pthread_cleanup_push(handler, (void *)&pool->lock);

        // 3. 上锁
        pthread_mutex_lock(&pool->lock);

        // 如果当前线程池没有关闭，并且当前线程池没有任务做（有线程没有任务，池子是打开的）
        while (pool->waiting_tasks == 0 && !pool->shutdown)
        {
            // 那么就进去条件变量中睡觉。
            pthread_cond_wait(&pool->cond, &pool->lock); // 自动解锁
        }

        // 要是线程池关闭了，或者有任务做，这些线程就会运行到这行代码
        // 判断当前线程池是不是关闭了，并且没有等待的任务
        if (pool->waiting_tasks == 0 && pool->shutdown == true)
        {
            // 如果线程池关闭，又没有任务做
            // 线程那么就会解锁
            pthread_mutex_unlock(&pool->lock);

            // 线程走人
            pthread_exit(NULL);
        }

        // 能运行到这里，说明有任务做  （删除节点）
        // p指向头节点的下一个
        p = pool->task_list->next;

        // 让头节点的指针域指向p的下一个节点
        pool->task_list->next = p->next;

        // 当前任务个数-1
        pool->waiting_tasks--;

        // 解锁
        pthread_mutex_unlock(&pool->lock);

        // 删除线程取消例程函数
        pthread_cleanup_pop(0);

        // 设置线程不能响应取消
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        // 执行这个p节点指向的节点的函数（执行任务）
        (p->do_task)(p);

        // 设置线程能响应取消
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        // 释放p对应的空间
        free(p);
    }

    pthread_exit(NULL);
}

// 添加线程
int add_thread(thread_pool *pool, unsigned additional_threads)
{
    // 如果说你想添加0条，则直接返回0。
    if (additional_threads == 0)
        return 0;

    unsigned total_threads = pool->active_threads + additional_threads;
    // total_threads = 原本 2条 + 现在再加2条

    int i, actual_increment = 0;

    // i = 2 ; i<4 && i<20; i++
    /*
        i 小于总共需要创建的线程数 total_threads 且同时小于线程池支持的最大活跃线程数 MAX_ACTIVE_THREADS。
        这个条件确保了循环不会超过线程池的承载能力
    */
    for (i = pool->active_threads; i < total_threads && i < MAX_ACTIVE_THREADS; i++)
    {
        if (pthread_create(&((pool->tids)[i]), NULL, routine, (void *)pool) != 0)
        {
            perror("add threads error");
            if (actual_increment == 0)
                return -1;

            break;
        }
        actual_increment++; // 真正创建线程的条数
    }

    pool->active_threads += actual_increment; // 当前线程池线程个数 = 原来的个数 + 实际创建的个数

    return actual_increment; // 返回真正创建的个数
}

int remove_thread(thread_pool *pool, unsigned int removing_threads)
{
    // 如果删除的线程数为0
    if (removing_threads == 0)
        return pool->active_threads;

    int remaining_threads = pool->active_threads - removing_threads;
    // 将剩余的线程数量 `remaining_threads` 设置为至少为1，确保至少保留一个线程在线程池中。
    remaining_threads = remaining_threads > 0 ? remaining_threads : 1;
    // 删除从最后面一个线程号开始删除
    int i = 0;
    for (i = pool->active_threads - 1; i > remaining_threads - 1; i--)
    {
        if (pthread_cancel(pool->tids[i]) != 0)
        {
            perror("pthread_cancel tids:%d error.");
            break;
        }
    }

    if (i == pool->active_threads - 1) // 删除失败
    {
        return -1;
    }
    else // 删除成功
    {
        pool->active_threads = i + 1; // 当前实际线程个数
        return i + 1;                 // 返回还有多少个线程数
    }
}

bool destroy_pool(thread_pool *pool)
{
    // 1. 设置线程池关闭标志为真
    pool->shutdown = true; // 表示要关闭线程池

    // 广播
    pthread_cond_broadcast(&pool->cond); // 目的是唤醒所有等待在条件变量上的线程，让它们退出

    // 2. 接合所有的线程
    int i;
    for (i = 0; i < pool->active_threads; i++)
    {
        errno = pthread_join(pool->tids[i], NULL); // 阻塞等待所有的线程退出

        if (errno != 0)
        {
            printf("join tids[%d] error: %s\n",
                   i, strerror(errno));
        }

        else
            printf("[%u] is joined\n", (unsigned)pool->tids[i]);
    }

    // 3. 释放空间
    free(pool->task_list); // 释放链表
    free(pool->tids);      // 释放线程数
    free(pool);            // 释放线程池

    return true;
}