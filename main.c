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

static bool startsWith(char *Str, char *SubStr) {
  return strncmp(Str, SubStr, strlen(SubStr)) == 0;
}

static int readPunct(char *Ptr) {
  if (startsWith(Ptr, "==") || startsWith(Ptr, "!=") || startsWith(Ptr, "<=") ||
      startsWith(Ptr, ">="))
    return 2;
  return ispunct(*Ptr) ? 1 : 0;
}

static bool equal(Token *Tok,char *Str){
    return memcmp(Tok->Loc,Str,Tok->Len) == 0 && Str[Tok->Len] == '\0';
}

static Token *skip(Token *Tok,char *Str){
    if(!equal(Tok, Str))
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

        int PunctLen = readPunct(P);
        if(PunctLen){
            Cur->Next = newToken(TK_PUNCT, P, P + PunctLen);
            Cur = Cur->Next;
            P += PunctLen;
            continue;
        }

        // error("invalid token: %c",*P);
        errorAt(P, "invalid token");
    }

    Cur->Next = newToken(TK_EOF, P, P);
    return Head.Next;
}

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

typedef struct Node Node;
struct Node {
  NodeKind Kind; 
  Node *LHS;     
  Node *RHS;     
  int Val;       
};

static Node *newNode(NodeKind Kind) {
  Node *Nd = calloc(1, sizeof(Node));
  Nd->Kind = Kind;
  return Nd;
}

static Node *newUnary(NodeKind Kind, Node *Expr) {
  Node *Nd = newNode(Kind);
  Nd->LHS = Expr;
  return Nd;
}


static Node *newBinary(NodeKind Kind, Node *LHS, Node *RHS) {
  Node *Nd = newNode(Kind);
  Nd->LHS = LHS;
  Nd->RHS = RHS;
  return Nd;
}

static Node *newNum(int Val) {
  Node *Nd = newNode(ND_NUM);
  Nd->Val = Val;
  return Nd;
}

// expr = equality
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-") unary | primary
// primary = "(" expr ")" | num
static Node *expr(Token **Rest, Token *Tok);
static Node *equality(Token **Rest, Token *Tok);
static Node *relational(Token **Rest, Token *Tok);
static Node *add(Token **Rest, Token *Tok);
static Node *mul(Token **Rest, Token *Tok);
static Node *unary(Token **Rest, Token *Tok);
static Node *primary(Token **Rest, Token *Tok);


static Node *expr(Token **Rest, Token *Tok) { return equality(Rest, Tok); }
static Node *equality(Token **Rest, Token *Tok) {
  Node *Nd = relational(&Tok, Tok);
  while (true) {
    if (equal(Tok, "==")) {
      Nd = newBinary(ND_EQ, Nd, relational(&Tok, Tok->Next));
      continue;
    }
    if (equal(Tok, "!=")) {
      Nd = newBinary(ND_NE, Nd, relational(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

static Node *relational(Token **Rest, Token *Tok) {
  Node *Nd = add(&Tok, Tok);

  while (true) {
    if (equal(Tok, "<")) {
      Nd = newBinary(ND_LT, Nd, add(&Tok, Tok->Next));
      continue;
    }
    if (equal(Tok, "<=")) {
      Nd = newBinary(ND_LE, Nd, add(&Tok, Tok->Next));
      continue;
    }
    if (equal(Tok, ">")) {
      Nd = newBinary(ND_LT, add(&Tok, Tok->Next), Nd);
      continue;
    }
    if (equal(Tok, ">=")) {
      Nd = newBinary(ND_LE, add(&Tok, Tok->Next), Nd);
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

static Node *add(Token **Rest, Token *Tok) {
  Node *Nd = mul(&Tok, Tok);
  while (true) {
    if (equal(Tok, "+")) {
      Nd = newBinary(ND_ADD, Nd, mul(&Tok, Tok->Next));
      continue;
    }

    if (equal(Tok, "-")) {
      Nd = newBinary(ND_SUB, Nd, mul(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

static Node *mul(Token **Rest, Token *Tok) {
  Node *Nd = unary(&Tok, Tok);
  while (true) {
    if (equal(Tok, "*")) {
      Nd = newBinary(ND_MUL, Nd, unary(&Tok, Tok->Next));
      continue;
    }
    if (equal(Tok, "/")) {
      Nd = newBinary(ND_DIV, Nd, unary(&Tok, Tok->Next));
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

static Node *unary(Token **Rest, Token *Tok) {
  if (equal(Tok, "+"))
    return unary(Rest, Tok->Next);

  if (equal(Tok, "-"))
    return newUnary(ND_NEG, unary(Rest, Tok->Next));

  return primary(Rest, Tok);
}

static Node *primary(Token **Rest, Token *Tok) {
  if (equal(Tok, "(")) {
    Node *Nd = expr(&Tok, Tok->Next);
    *Rest = skip(Tok, ")");
    return Nd;
  }

  // num
  if (Tok->Kind == TK_NUM) {
    Node *Nd = newNum(Tok->Val);
    *Rest = Tok->Next;
    return Nd;
  }

  errorTok(Tok, "expected an expression");
  return NULL;
}

//
// 语义分析与代码生成
//

static int Depth;
static void push(void) {
  printf("  addi sp, sp, -8\n");
  printf("  sd a0, 0(sp)\n");
  Depth++;
}

static void pop(char *Reg) {
  printf("  ld %s, 0(sp)\n", Reg);
  printf("  addi sp, sp, 8\n");
  Depth--;
}

static void genExpr(Node *Nd) {
  switch (Nd->Kind) {
  case ND_NUM:
    printf("  li a0, %d\n", Nd->Val);
    return;
  case ND_NEG:
    genExpr(Nd->LHS);
    printf("  neg a0, a0\n");
    return;
  default:
    break;
  }

  genExpr(Nd->RHS);
  push();
  genExpr(Nd->LHS);
  pop("a1");

  switch (Nd->Kind) {
  case ND_ADD: 
    printf("  add a0, a0, a1\n");
    return;
  case ND_SUB: 
    printf("  sub a0, a0, a1\n");
    return;
  case ND_MUL: 
    printf("  mul a0, a0, a1\n");
    return;
  case ND_DIV: 
    printf("  div a0, a0, a1\n");
    return;
  case ND_EQ:
  case ND_NE:
    printf("  xor a0, a0, a1\n");

    if (Nd->Kind == ND_EQ)
      printf("  seqz a0, a0\n");
    else
      printf("  snez a0, a0\n");
    return;
  case ND_LT:
    printf("  slt a0, a0, a1\n");
    return;
  case ND_LE:
    printf("  slt a0, a1, a0\n");
    printf("  xori a0, a0, 1\n");
    return;
  default:
    break;
  }

  error("invalid expression");
}

int main(int Argc , char **Argv){
    if( Argc != 2){
        error("%s: invalid number of arguments\n", Argv[0]);
    }
    //[3]:解析Argv[1]
    CurrentInput = Argv[1];
    Token *Tok = tokenize(Argv[1]);
    Node *Node = expr(&Tok, Tok);
    
    if (Tok->Kind != TK_EOF) errorTok(Tok, "extra token");


    printf("    .globl main\n");
    printf("main:\n");

    genExpr(Node);
    printf("    ret\n");
    assert(Depth == 0);
    return 0;
}