#include "rvcc.h"

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

static void genAddr(Node *Nd){
    if(Nd->Kind == ND_VAR){
        printf("    addi a0, fp, %d\n", Nd->Var->Offset);
        return ;
    }
    error("not an lvalue");
}

// 对齐到Align的整数倍
static int alignTo(int N, int Align){
  return (N + Align - 1) / Align *Align;
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
    case ND_VAR:
        genAddr(Nd);
        printf("    ld a0, 0(a0)\n");
        return ;
    case ND_ASSIGN:
        genAddr(Nd->LHS);
        push();
        genExpr(Nd->RHS);
        pop("a1");
        printf("    sd a0, 0(a1)\n");
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

static void genStmt(Node *Nd){
    switch (Nd->Kind)
    {
    case ND_BLOCK:
      for(Node *N = Nd->Body; N; N = N->Next)
        genStmt(N);
      return ;
    case ND_RETURN:
      genExpr(Nd->LHS);
      printf("  j .L.return\n");
      return ;
    case ND_EXPR_STMT:
      genExpr(Nd->LHS);
      return;
    
    default:
      break;
    }
    error("invalid statement");
}

static void assignLVarOffsets(Function *Prog){
  int Offset = 0;
  for(Obj *Var = Prog->Locals; Var; Var = Var->Next){
    Offset += 8;
    Var->Offset = -Offset;
  }
  Prog->StackSize = alignTo(Offset, 16);
}

void codegen(Function *Prog){
  assignLVarOffsets(Prog);
    printf("    .globl main\n");
    printf("main:\n");

  // 栈布局
  //-------------------------------// sp
  //              fp
  //-------------------------------// fp = sp-8
  //             变量
  //-------------------------------// sp = sp-8-StackSize
  //           表达式计算
  //-------------------------------//
    

    // Prologue, 前言
    // 将fp压入栈中，保存fp的值
    printf("    addi sp, sp, -8\n");
    printf("    sd fp, 0(sp)\n");
    printf("    mv fp, sp\n");  // 将sp写入fp
    printf("    addi sp, sp, -%d\n", Prog->StackSize);// 26个字母*8字节=208字节，栈腾出208字节的空间

    for(Node *N = Prog->Body; N; N=N->Next){
        genStmt(N);
        assert(Depth == 0);
    }

    // Epilogue，后语
    // 将fp的值改写回sp
    printf(".L.return: \n");
    printf("  mv sp, fp\n");
    // 将最早fp保存的值弹栈，恢复fp。
    printf("  ld fp, 0(sp)\n");
    printf("  addi sp, sp, 8\n");
    // 返回
    printf("    ret\n");
}