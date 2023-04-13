#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


//
// 终结符分析，词法分析
//

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


void error(char *Fmt,...);
//void verrorAt(char *Loc, char *Fmt, va_list VA);
void errorAt(char *Loc, char *Fmt, ...);
void errorTok(Token *Tok, char *Fmt, ...);


// 判断Token与Str的关系
bool equal(Token *Tok, char *Str);
Token *skip(Token *Tok, char *Str);

// 词法分析
Token *tokenize(char *Input);


//
// 生成AST（抽象语法树），语法解析
//
// AST的节点种类
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NEG, // 负号-
  ND_EQ,  // ==
  ND_NE,  // !=
  ND_LT,  // <
  ND_LE,  // <=
  ND_NUM, 
} NodeKind;

// AST中二叉树节点
typedef struct Node Node;
struct Node {
  NodeKind Kind; 
  Node *LHS;     
  Node *RHS;     
  int Val;       
};
Node *parse(Token *Tok);

//
// 语义分析与代码生成
//

// 代码生成入口函数
void codegen(Node *Nd);




