#include <stdio.h>
#include <stdlib.h>

int main(int Argc , char **Argv){
    if( Argc != 2){
         fprintf(stderr, "%s: invalid number of arguments\n", Argv[0]);
         return 1;
    }
    //[2]:P读取表达式数组
    char *P = Argv[1];

    printf("    .globl main\n");
    printf("main:\n");

    //[2]:没有输入检错机制
    printf("    li a0,%ld\n",strtol(P,&P,10));
    while(*P){
        switch (*P)
        {
            case '+':
                ++P;
                printf("    addi a0, a0, %ld\n",strtol(P, &P, 10));
                continue;
            case '-':
                ++P;
                printf("    addi a0, a0, -%ld\n",strtol(P,&P,10));
                continue;
            default:
                fprintf(stderr,"unexpected character: '%c'\n",*P);
                return 1;
        }
    }
    printf("    ret\n");

    return 0;
}