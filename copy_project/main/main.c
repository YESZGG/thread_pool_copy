/*
 * @Description:
 * @version: 1.80.1
 * @Author: ZGG
 * @Date: 2023-08-05 14:33:04
 * @LastEditors: ZGG
 * @LastEditTime: 2023-08-09 19:53:31
 */
#include "thread_pool.h"
#include "copy.h"

int main(int argc, char *argv[])
{
    char src[256], dst[256];

    printf("请输入源路径：");
    scanf("%s", src);

    printf("请输入目标路径：");
    scanf("%s", dst);
    thread_pool *pool = malloc(sizeof(thread_pool));
    // 初始化线程池
    init_pool(pool, 2);
    get_dir_path(src, dst, pool);

    destroy_pool(pool);

    return 0;
}