// 使用POSIX.1标准
// 使用了strndup函数
#define _POSIX_C_SOURCE 200809L
// #define DEBUG

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
    TK_IDENT,   // sign: var_name , func_name
    TK_PUNCT,   //opcode : + -
    TK_KEYWORD,
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
void errorAt(char *Loc, char *Fmt, ...);
void errorTok(Token *Tok, char *Fmt, ...);

// 判断Token与Str的关系
bool equal(Token *Tok, char *Str);
Token *skip(Token *Tok, char *Str);
bool consume(Token **Rest, Token *Tok, char *Str);
// 词法分析
Token *tokenize(char *Input);


//
// 生成AST（抽象语法树），语法解析
//

typedef struct Node Node;
typedef struct Type Type;
// Local var
typedef struct Obj Obj;
struct Obj{
  Obj *Next;
  char *Name;
  Type *Ty;
  int Offset;
};

typedef struct Function Function;
struct Function{
  Function *Next;
  char *Name;
  Obj *Params;

  Node *Body;
  Obj *Locals;
  int StackSize;
};



// AST的节点种类
typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_NEG,       // 负号-
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // 赋值
  ND_ADDR,
  ND_DEREF,
  ND_RETURN,
  ND_IF,        // "if"，条件判断
  ND_FOR,       // "for" or "while"
  ND_BLOCK,     // { ... }, code block
  ND_FUNCALL,
  ND_EXPR_STMT, // Expr
  ND_VAR,       // var
  ND_NUM, 
} NodeKind;

// AST中二叉树节点

struct Node {
  NodeKind Kind; 
  Node *Next;
  Token *Tok;    // 节点对应的终结符
  Type *Ty;

  Node *LHS;     
  Node *RHS;

  // "if"  or "for"
  Node *Cond; // if expr
  Node *Then; // 符合条件后的语句
  Node *Els;  // 不符合条件后的语句
  Node *Init; 
  Node *Inc;  

  // func_Call
  char *FuncName;
  Node *Args;


  // code block
  Node *Body;  

  Obj *Var; 
  int Val;       
};
Function *parse(Token *Tok);

//
// 类型系统
//
typedef enum{
  TY_INT,  // int 
  TY_PTR,  // pointer
  TY_FUNC,
}TypeKind;

struct Type{
  TypeKind Kind;
  Type *Base;     // 指向的类型
  Token *Name;

  // Func type
  Type *ReturnTy;
  Type *Params;
  Type *Next;  
};

extern Type *TyInt;

bool isInteger(Type *TY);
Type *copyType(Type *Ty);
Type *pointerTo(Type *Base);
void addType(Node *Nd);
Type *funcType(Type *ReturnTy);

//
// 语义分析与代码生成
//

// 代码生成入口函数
void codegen(Function *Prog);




