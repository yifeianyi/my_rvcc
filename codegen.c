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

void codegen(Node* Nd){
    printf("    .globl main\n");
    printf("main:\n");
    genExpr(Nd);
    printf("    ret\n");
    assert(Depth == 0);
}