#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  //env
#include <string.h>
#include"global.h"


/**
 * @brief 初始化可执行程序的环境变量内存，为程序设置新的内存空间并拷贝环境变量到新内存中。
 *
 * 该函数会为程序分配内存，并将原环境变量拷贝到新分配的内存中，
 * 以便为设置可执行程序标题腾出内存空间。
 *
 * @note 该函数不对内存分配失败做任何处理，内存分配失败会导致程序崩溃。
 */
extern void init_setproctitle()
{
    //这里无需判断penvmen == NULL,有些编译器new会返回NULL，有些会报异常，但不管怎样，如果在重要的地方new失败了，你无法收场，让程序失控崩溃，助你发现问题为好； 
    gp_envmem = new char[g_envneedmem];
    memset(gp_envmem, 0, g_envneedmem);  //内存要清空防止出现问题

    char* ptmp = gp_envmem;
    //把原来的内存内容搬到新地方来
    for (int i = 0; environ[i]; i++)
    {
        size_t size = strlen(environ[i]) + 1; //不要拉下+1，否则内存全乱套了，因为strlen是不包括字符串末尾的\0的
        strcpy(ptmp, environ[i]);      //把原环境变量内容拷贝到新地方【新内存】
        environ[i] = ptmp;            //然后还要让新环境变量指向这段新内存
        ptmp += size;
    }
    return;
}

/**
 * @brief 设置可执行程序的标题。
 *
 * 该函数将会覆盖原有的命令行参数，并设置新的可执行程序标题。程序的命令行参数将会被清空，除非它们被标题覆盖。
 *
 * @param title 新的程序标题字符串。
 *
 * @note 如果标题长度过大，导致无法同时存储标题和命令行参数、环境变量，函数将不会执行任何操作。
 *       标题长度不应超过原始 `argv` 和 `environ` 占用内存的总和。
 */
extern void setproctitle(const char* title)
{
    //我们假设，所有的命令 行参数我们都不需要用到了，可以被随意覆盖了；
    //注意：我们的标题长度，不会长到原始标题和原始环境变量都装不下，否则怕出问题，不处理

    //(1)计算新标题长度
    size_t ititlelen = strlen(title);

    //(2)计算总的原始的argv那块内存的总长度【包括各种参数】    
    size_t esy = g_argvneedmem + g_envneedmem; //argv和environ内存总和
    if (esy <= ititlelen)
    {
        //你标题多长啊，我argv和environ总和都存不下？注意字符串末尾多了个 \0，所以这块判断是 <=【也就是=都算存不下】
        return;
    }

    //空间够保存标题的，够长，存得下，继续走下来    

    //(3)设置后续的命令行参数为空，表示只有argv[]中只有一个元素了，这是好习惯；防止后续argv被滥用，因为很多判断是用argv[] == NULL来做结束标记判断的;
    g_os_argv[1] = NULL;

    //(4)把标题弄进来，注意原来的命令行参数都会被覆盖掉，不要再使用这些命令行参数,而且g_os_argv[1]已经被设置为NULL了
    char* ptmp = g_os_argv[0]; //让ptmp指向g_os_argv所指向的内存
    strcpy(ptmp, title);
    ptmp += ititlelen; //跳过标题

    //(5)把剩余的原argv以及environ所占的内存全部清0，否则会出现在ps的cmd列可能还会残余一些没有被覆盖的内容；
    size_t cha = esy - ititlelen;  //内存总和减去标题字符串长度(不含字符串末尾的\0)，剩余的大小，就是要memset的；
    memset(ptmp, 0, cha);
    return;
}