#include<stdio.h>  // printf / fprintf / perror / snprintf
#include<dirent.h> // opendir / readdir / closedir / struct dirent
#include<sys/stat.h>  // stat / struct stat / S_ISDIR / S_ISREG
#include<string.h>  

// 判断文件名是否以 ".md" 结尾。返回 1=是，0=否
// static：只在本文件内可见——单文件程序里的好习惯
static int has_md_suffix(const char *name)
{
    size_t len = strlen(name);
    return len >= 3 && strcmp(name + len - 3,".md") == 0;
    // name + len - 3：指针算术——从首地址往后跳 len-3 格，正好落在最后 3 个字符上
    // strcmp 从那里比到结尾，比完恰好 3 个字符；返回 0 表示内容相同
    // len < 3 时 && 短路，直接得 0（假），避免越界读
}

// 递归遍历 dirpath 目录，把其中所有 .md 文件的路径打印到 stdout
// 返回值：0 = 成功；1 = 本层目录打不开（顶层调用者拿它当退出码）
static int scan_dir(const char *dirpath)
{
    DIR *dir = opendir(dirpath);
    if(dir == NULL)
    {
        perror(dirpath);
        return 1;
    }

    struct dirent *entry = NULL;
    while((entry = readdir(dir)) != NULL)
    {   
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char child[1024];
        int n = snprintf(child, sizeof child, "%s/%s",dirpath, entry->d_name); //n<0则说明发生异常，n >= sizeof child则说明发生了截断，其他情况正常
        if(n < 0 || (size_t)n >= sizeof child) //size_t 的这一步转化的含义是？
        {
            fprintf(stderr, "路径过长，跳过：%s/%s", dirpath, entry->d_name);
            continue;   
        }

        struct stat st;
        if (stat(child, &st) != 0)// 关于stat()，失败返回-1并设置errno，成功返回0并填充struct stat
        {
            perror(child);
            continue;
        }

        if(S_ISDIR(st.st_mode))//其中S_ISDIR是一个宏，被展开成一个计算式，计算结果：1 = 是普通文件；0 = 不是
        {
            scan_dir(child);
        }
        else if(S_ISREG(st.st_mode) && has_md_suffix(entry->d_name))
        {
            printf("%s\n",child);
        }
    
    }
    
    closedir(dir);
    return 0;

}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "用法：note-stats <目录>\n");
        return 1;
    }

   return scan_dir(argv[1]);
}