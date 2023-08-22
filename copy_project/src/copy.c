#include "thread_pool.h"
#include "copy.h"

void *copy(void *arg)
{
    struct task *p = (struct task *)arg;
    printf("COPY：%s TO  %s\n", p->src_path, p->dest_path);
    printf("[%u] ==> start doing!\n", (unsigned)pthread_self()); // 打印提示信息，某某线程开始工作
    copy_stod(p->src_path, p->dest_path);                        // 调用复制函数进行复制

    printf("[%u] ==> job done!\n", (unsigned)pthread_self()); // 打印提示信息，某某线程已经完成工作，并汇报它的tid

    return NULL;
}

void print_progress(double percentage)
{
    int bar_width = 50;
    int filled_width = bar_width * percentage;
    printf("[");
    for (int i = 0; i < bar_width; i++)
    {
        if (i < filled_width)
            printf("=");
        else
            printf(" ");
    }
    printf("] %.2f%%\r", percentage * 100);
    fflush(stdout);

    printf("拷贝成功！");
}

// 功能是遍历指定目录下的所有文件和子目录，并将常规文件添加到一个线程池中，用于复制到目标路径。
void get_dir_path(char *src_path, char *dest_path, thread_pool *pool)
{
    struct stat st;
    if (stat(src_path, &st) != 0)
    {
        perror("stat error");
        return;
    }

    if (S_ISREG(st.st_mode))
    { // Input is a regular file
        add_task(pool, copy, (void *)pool, src_path, dest_path);
    }
    else if (S_ISDIR(st.st_mode))
    {
        // 打开源文件路径
        DIR *src_dp = opendir(src_path);
        if (src_dp == NULL)
        {
            perror("open src_path error");
            return;
        }
        // 修改系统权限 umask 的值
        umask(0000);
        /*
        头文件：unistd.h
        功 能: 确定文件或文件夹的访问权限。即，检查某个文件的存取方式，比如说是只读方式、只写方式等。
               如果指定的存取方式有效，则函数返回0，否则函数返回-1。
            int access(const char *filenpath, int mode);
        */
        // 检查目标文件是否存在
        if (access(dest_path, F_OK) != 0)
        {
            mkdir(dest_path, 0777); // 如果不存在，则创建目标文件目录
        }

        // 定义目录结构体指针
        struct dirent *rd = NULL;
        char src_name[4096] = {0};  // 用于存储源文件/目录路径的缓冲区
        char dest_name[4096] = {0}; // 用于存储目标文件/目录路径的缓冲区

        while ((rd = readdir(src_dp)) != NULL)
        {
            if (strncmp(rd->d_name, ".", 1) != 0) // 跳过以“ . ”开头的文件 即跳过隐藏文件
            {
                sprintf(src_name, "%s/%s", src_path, rd->d_name);   // 构造源路径
                sprintf(dest_name, "%s/%s", dest_path, rd->d_name); // 构建目标路径

                if (rd->d_type == DT_REG) // 如果是常规文件，请添加用于复制的任务
                {
                    add_task(pool, copy, (void *)pool, src_name, dest_name); // 添加到线程池中
                }
                else if (rd->d_type == DT_DIR) // 如果是目录，则递归调用get_dir_path 自己本身
                {
                    get_dir_path(src_name, dest_name, pool); // 处理目录中文件的递归调用
                }
            }
        }
        closedir(src_dp);
    }
}

// 功能是将指定源文件拷贝到目标文件中。 它会打开源文件和目标文件，并计算源文件大小。 然后，通过循环读取源文件内容，并将内容写入目标文件中，直到源文件的所有内容都被复制完成。
void copy_stod(char *src_path, char *dest_path)
{
    // 打开源文件
    FILE *src_file = fopen(src_path, "r");
    if (src_file == NULL)
    {
        perror("Failed to open source file.");
        return;
    }
    FILE *dest_file = fopen(dest_path, "w+");
    if (dest_file == NULL)
    {
        perror("Failed to open destination file.");
        return;
    }

    // 计算文件大小
    /*     fseek函数
    成功：返回0     失败：返回-1
           ftell函数
    成功：返回当前的偏移量  失败：返回-1
    */
    fseek(src_file, 0, SEEK_END);
    long fileSize = ftell(src_file);
    if (fileSize == -1)
    {
        perror("获取源文件大小失败");
        fclose(src_file);
        fclose(dest_file);
        return;
    }
    fseek(src_file, 0, SEEK_SET);
    // 拷贝源文件数据到目标文件
    const size_t buffer_size = 4096;
    
    char buffer[buffer_size];
    memset(buffer, 0, sizeof(buffer_size));

    size_t bytes_read = 0, bytes_write = 0, totalBytes = 0;

    while (bytes_read = fread(buffer, 1, sizeof(buffer), src_file))
    {
        bytes_write = fwrite(buffer, 1, bytes_read, dest_file);
        if (bytes_read != bytes_write)
        {
            perror("拷贝失败!");
            fclose(src_file);
            fclose(dest_file);
            return;
        }
        totalBytes += bytes_write;
        double progress = (double)totalBytes / fileSize;
        print_progress(progress);
    }
    if (fclose(src_file) != 0)
    {
        perror("fclose source_file fail.");
        return;
    }

    if (fclose(dest_file) != 0)
    {
        perror("fclose target_file fail.");
        return;
    }
}

/* struct dirent {
    ino_t          d_ino;       // Inode编号
    off_t          d_off;       // 不是偏移；见下文
    unsigned short d_reclen;    // 记录的长度
    unsigned char  d_type;      // 文件类型；并非所有文件系统类型都支持
    char           d_name[256]; // 文件名（不能超过256个字节)
}; */

/*
DT_BLK      一个块设备              6
DT_CHR      一个字符设备            2
DT_DIR      一个目录                4
DT_FIFO     一个命名管道（FIFO）     1
DT_LNK      一个符号链接            10
DT_REG      一个常规文件            8
DT_SOCK     一个UNIX域套接字        12
DT_UNKNOWN  无法确定文件类型
*/