#ifndef __MYLOG_H__
#define __MYLOG_H__

#define    PRINT_MACRO_HELPER(x)  #x
#define    PRINT_MACRO(x)         #x"="PRINT_MACRO_HELPER(x)

//在编译阶段打印宏的内容：在需要的地方放上这句就可以。
//#pragma message(PRINT_MACRO(YOUR_MACRO))


extern char *basename(char *s);
#include <stdio.h>

#define myprintf(...) do {\
                        printf("[%s][%s][%d]: ", basename(__FILE__), __func__, __LINE__);\
                        printf(__VA_ARGS__);\
                        printf("\n");\
                       }while(0)
#define mylogd(...) do {\
                            printf("[DEBUG][%s][%s][%d]: ", basename(__FILE__), __func__, __LINE__);\
                            printf(__VA_ARGS__);\
                            printf("\n");\
                           }while(0)
#define myloge(...) do {\
                            printf("[ERROR][%s][%s][%d]: ", basename(__FILE__), __func__, __LINE__);\
                            printf(__VA_ARGS__);\
                            printf("\n");\
                           }while(0)
#define mylogw(...) do {\
                            printf("[WARN][%s][%s][%d]: ", basename(__FILE__), __func__, __LINE__);\
                            printf(__VA_ARGS__);\
                            printf("\n");\
                           }while(0)
#endif
