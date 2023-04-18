#include "rvcc.h"

static int Depth;

static char *ArgReg[] = {"a0", "a1", "a2", "a3", "a4", "a5"};
static Function *CurrentFn;

static void genExpr(Node *Nd);
// Count of code block
static int count(void){
  static int I = 1;
  return I++;
}
static void push(void) {
  printf("  # 压栈，将a0的值存入栈顶\n");
  printf("  addi sp, sp, -8\n");
  printf("  sd a0, 0(sp)\n");
  Depth++;
}

static void pop(char *Reg) {
  printf("  # 弹栈，将栈顶的值存入%s\n", Reg);
  printf("  ld %s, 0(sp)\n", Reg);
  printf("  addi sp, sp, 8\n");
  Depth--;
}

static void genAddr(Node *Nd){
  switch (Nd->Kind)
  {
  case ND_VAR:
    printf("  # 获取变量%s的栈内地址为%d(fp)\n", Nd->Var->Name,
          Nd->Var->Offset);
    printf("    addi a0, fp, %d\n", Nd->Var->Offset);
    return ;
  case ND_DEREF:
    genExpr(Nd->LHS);
    return ;
  default:
    break;
  }

  errorTok(Nd->Tok,"not an lvalue");
}

// 对齐到Align的整数倍
static int alignTo(int N, int Align){
  return (N + Align - 1) / Align *Align;
}

static void genExpr(Node *Nd) {
  switch (Nd->Kind) {
    case ND_NUM:
      printf("  # 将%d加载到a0中\n", Nd->Val);
      printf("  li a0, %d\n", Nd->Val);
      return;
    case ND_NEG:
      genExpr(Nd->LHS);
      printf("  # 对a0值进行取反\n");
      printf("  neg a0, a0\n");
      return;
    case ND_VAR:
      genAddr(Nd);
      printf("  # 读取a0中存放的地址，得到的值存入a0\n");
      printf("    ld a0, 0(a0)\n");
      return ;
    case ND_DEREF:
      genExpr(Nd->LHS);
      printf("  # 读取a0中存放的地址，得到的值存入a0\n");
      printf("  ld a0, 0(a0)\n");
      return ;
    case ND_ADDR:
      genAddr(Nd->LHS);
      return ;
    case ND_ASSIGN:
      genAddr(Nd->LHS);
      push();
      genExpr(Nd->RHS);
      pop("a1");
      printf("  # 将a0的值，写入到a1中存放的地址\n");
      printf("    sd a0, 0(a1)\n");
      return;
    case ND_FUNCALL: {
      int NArgs = 0;
      for (Node *Arg = Nd->Args; Arg; Arg = Arg->Next) {
        genExpr(Arg);
        push();
        NArgs++;
      }
      for (int i = NArgs - 1; i >= 0; i--)
        pop(ArgReg[i]);
      printf("  # 调用%s函数\n", Nd->FuncName);
      printf("  call %s\n", Nd->FuncName);
      return;
    }
    default:
      break;
  }

  genExpr(Nd->RHS);
  push();
  genExpr(Nd->LHS);
  pop("a1");

  switch (Nd->Kind) {
  case ND_ADD: 
    printf("  # a0 = a0 + a1\n");
    printf("  add a0, a0, a1\n");
    return;
  case ND_SUB: 
    printf("  # a0 = a0 - a1\n");
    printf("  sub a0, a0, a1\n");
    return;
  case ND_MUL: 
  printf("  # a0 = a0 * a1\n");
    printf("  mul a0, a0, a1\n");
    return;
  case ND_DIV: 
  printf("  # a0 = a0 / a1\n");
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
    printf("  # 判断a0<a1\n");
    printf("  slt a0, a0, a1\n");
    return;
  case ND_LE:
    printf("  # 判断是否a0≤a1\n");
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
  case ND_IF:{
    int C = count();
    printf("\n# =====分支语句%d==============\n", C);
    printf("\n# Cond表达式%d\n", C);

    genExpr(Nd->Cond);

    printf("  # 若a0为0，则跳转到分支%d的.L.else.%d段\n", C, C);
    printf("  beqz a0, .L.else.%d\n", C);

    printf("\n# Then语句%d\n", C);
    genStmt(Nd->Then);

    printf("  # 跳转到分支%d的.L.end.%d段\n", C, C);
    printf("  j .L.end.%d\n",C);

    printf("\n# Else语句%d\n", C);
    printf("# 分支%d的.L.else.%d段标签\n", C, C);
    printf(".L.else.%d:\n",C);
    if(Nd->Els)
      genStmt(Nd->Els);

    printf("\n# 分支%d的.L.end.%d段标签\n", C, C);
    printf(".L.end.%d:\n",C);
    return ;
  }
  case ND_FOR:{
    int C = count();
    printf("\n# =====循环语句%d===============\n", C);

    if(Nd->Init){
      printf("\n# Init语句%d\n", C);
      genStmt(Nd->Init);
    }
      
    printf("\n# 循环%d的.L.begin.%d段标签\n", C, C);
    printf(".L.begin.%d:\n",C);

    printf("# Cond表达式%d\n", C);
    if(Nd->Cond){
      genExpr(Nd->Cond);
      printf("  # 若a0为0，则跳转到循环%d的.L.end.%d段\n", C, C);
      printf("  beqz a0, .L.end.%d\n", C);
    }

    genStmt(Nd->Then);
    if(Nd->Inc){
      printf("\n# Inc语句%d\n", C);
      genExpr(Nd->Inc);
    }
    
    printf("  # 跳转到循环%d的.L.begin.%d段\n", C, C);
    printf("  j .L.begin.%d\n", C);

    printf("\n# 循环%d的.L.end.%d段标签\n", C, C);
    printf(".L.end.%d:\n", C);
    return ;
  }
  case ND_BLOCK:
    for(Node *N = Nd->Body; N; N = N->Next)
      genStmt(N);
    return ;
  case ND_RETURN:
    printf("# 返回语句\n");
    genExpr(Nd->LHS);

    printf("  # 跳转到.L.return.%s段\n", CurrentFn->Name);
    printf("  j .L.return.%s\n", CurrentFn->Name);
    return ;
    
  // 生成表达式语句
  case ND_EXPR_STMT:
    genExpr(Nd->LHS);
    return;
  
  default:
    break;
  }
  errorTok(Nd->Tok,"invalid statement");
}

static void assignLVarOffsets(Function *Prog){
  for (Function *Fn = Prog; Fn; Fn = Fn->Next) {
    int Offset = 0;
    for (Obj *Var = Fn->Locals; Var; Var = Var->Next) {
      Offset += 8;
      Var->Offset = -Offset;
    }
    Fn->StackSize = alignTo(Offset, 16);
  }
}

void codegen(Function *Prog){
  assignLVarOffsets(Prog);
  for (Function *Fn = Prog; Fn; Fn = Fn->Next) {
    printf("\n  # 定义全局%s段\n", Fn->Name);
    printf("  .globl %s\n", Fn->Name);
    printf("# =====%s段开始===============\n", Fn->Name);
    printf("# %s段标签\n", Fn->Name);
    printf("%s:\n", Fn->Name);
    CurrentFn = Fn;

    // 栈布局
    //-------------------------------// sp
    //              ra
    //-------------------------------// ra = sp-8
    //              fp
    //-------------------------------// fp = sp-16
    //             变量
    //-------------------------------// sp = sp-16-StackSize
    //           表达式计算
    //-------------------------------//

    // Prologue, 前言
    // 将ra寄存器压栈,保存ra的值
    printf("  # 将ra寄存器压栈,保存ra的值\n");
    printf("  addi sp, sp, -16\n");
    printf("  sd ra, 8(sp)\n");
    printf("  sd fp, 0(sp)\n");
    printf("  mv fp, sp\n");
    printf("  addi sp, sp, -%d\n", Fn->StackSize);

    int I = 0;
    for (Obj *Var = Fn->Params; Var; Var = Var->Next) {
      printf("  # 将%s寄存器的值存入%s的栈地址\n", ArgReg[I], Var->Name);
      printf("  sd %s, %d(fp)\n", ArgReg[I++], Var->Offset);
    }

    // 生成语句链表的代码
    printf("# =====%s段主体===============\n", Fn->Name);
    genStmt(Fn->Body);
    assert(Depth == 0);

    // Epilogue，后语
    // 输出return段标签
    printf("# =====%s段结束===============\n", Fn->Name);
    printf("# return段标签\n");
    printf(".L.return.%s:\n", Fn->Name);
    printf("  # 将fp的值写回sp\n");
    printf("  mv sp, fp\n");
    printf("  ld fp, 0(sp)\n");
    printf("  ld ra, 8(sp)\n");
    printf("  addi sp, sp, 16\n");
    // 返回
    printf("  # 返回a0值给系统调用\n");
    printf("  ret\n");
  }
}