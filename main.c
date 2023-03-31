#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//=============== about token ======================
typedef enum{
    TK_PUNCT,   //opcode : + -
    TK_NUM,     // number
    TK_EOF,     // file ending
}TokenKind;


typedef struct Token Token;
struct Token{
    TokenKind Kind;
    Token *Next;    //next token
    int Val;
    char *Loc;      //在解析的字符串内的位置
    int Len;
};

// Input string
static char *CurrentInput;

//================ error information ===================
static void error(char *Fmt,...){
    va_list VA;
    va_start(VA,Fmt);
    vfprintf(stderr,Fmt,VA);
    fprintf(stderr,"\n");
    va_end(VA);
    exit(1);
}
static void verrorAt(char *Loc, char *Fmt, va_list VA){
    fprintf(stderr, "%s\n", CurrentInput);
    int Pos = Loc - CurrentInput;
    fprintf(stderr, "%*s", Pos, "");
    fprintf(stderr ,"^ ");
    vfprintf(stderr, Fmt, VA);
    fprintf(stderr, "\n");
    va_end(VA);
}
static void errorAt(char *Loc, char *Fmt, ...){
    va_list VA;
    va_start(VA, Fmt);
    verrorAt(Loc, Fmt, VA);
    exit(1);
}
static void errorTok(Token *Tok, char *Fmt, ...){
    va_list VA;
    va_start(VA, Fmt);
    verrorAt(Tok->Loc, Fmt, VA);
    exit(1);
}
//===================== other func ======================
static bool equal(Token *Tok,char *Str){
    return memcmp(Tok->Loc,Str,Tok->Len) == 0 && Str[Tok->Len] == '\0';
}

// 跳过指定的Str
static Token *skip(Token *Tok,char *Str){
    if(!equal(Tok, Str))// error("expect '%s'",Str);
        errorTok(Tok,"expect a number");
    return Tok->Next;
}

//
static int getNumber(Token *Tok){
    if(Tok->Kind != TK_NUM)//error("expect a number");
        errorTok(Tok, "expect a number");
    return Tok->Val;
}

static Token* newToken(TokenKind Kind,char *Start, char *End){
    // 分配1个Token的内存空间
    Token *Tok = calloc(1,sizeof(Token));
    Tok->Kind = Kind;
    Tok->Loc = Start;
    Tok->Len = End -Start;
    return Tok;
}

static Token *tokenize(){
    char *P = CurrentInput;
    Token Head = {};
    Token *Cur = &Head;

    while(*P){
        if(isspace(*P)){
            ++P;
            continue;
        }

        if(isdigit(*P)){
            Cur->Next = newToken(TK_NUM,P,P);
            Cur = Cur->Next;
            const char *OldPtr = P;
            Cur->Val = strtoul(P, &P, 10);
            Cur->Len = P - OldPtr;
            continue;
        }

        if(*P == '+' || *P=='-'){
            Cur->Next = newToken(TK_PUNCT, P, P + 1);
            Cur = Cur->Next;
            ++P;
            continue;
        }

        // error("invalid token: %c",*P);
        errorAt(P, "invalid token");
    }

    Cur->Next = newToken(TK_EOF, P, P);
    return Head.Next;
}

int main(int Argc , char **Argv){
    if( Argc != 2){
        error("%s: invalid number of arguments\n", Argv[0]);
        //  fprintf(stderr, "%s: invalid number of arguments\n", Argv[0]);
        //  return 1;
    }

    printf("    .globl main\n");
    printf("main:\n");

    //[3]:解析Argv[1]
    CurrentInput = Argv[1];
    Token *Tok = tokenize(Argv[1]);

    //[3]:这里我们将算式分解为 num (op num) (op num)...的形式
    //[3]:所以先将第一个num传入a0
    printf("    li a0, %d\n",getNumber(Tok));
    Tok = Tok->Next;

    // 解析 (op num)
    while(Tok->Kind != TK_EOF){
        //[3]
        if(equal(Tok, "+")){
            Tok = Tok->Next;
            printf("    addi a0, a0, %d\n", getNumber(Tok));
            Tok = Tok->Next;
            continue;
        }
        Tok = skip(Tok, "-");
        printf("    addi a0, a0, -%d\n", getNumber(Tok));
        Tok = Tok->Next;
    }
    printf("    ret\n");

    return 0;
}