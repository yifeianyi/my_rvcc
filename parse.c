#include "rvcc.h"

// 在解析时，全部的变量实例都被累加到这个列表里。
Obj *Locals;

// [9]
// program = stmt*
// stmt = exprStmt
// exprStmt = expr ";"

// [1]-[8]
// expr = assign 
// assign = equality ("=" assign)?
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-") unary | primary
// primary = "(" expr ")" | num


// [10]
static Node *assign(Token **Rest, Token *Tok);
// [9]
static Node *exprStmt(Token **Rest, Token *Tok);
// [1]-[8]
static Node *expr(Token **Rest, Token *Tok);
static Node *equality(Token **Rest, Token *Tok);
static Node *relational(Token **Rest, Token *Tok);
static Node *add(Token **Rest, Token *Tok);
static Node *mul(Token **Rest, Token *Tok);
static Node *unary(Token **Rest, Token *Tok);
static Node *primary(Token **Rest, Token *Tok);



static Obj *findVar(Token *Tok){
  for(Obj *Var = Locals; Var; Var = Var->Next){
    if(strlen(Var->Name) == Tok->Len &&
      !strncmp(Tok->Loc, Var->Name, Tok->Len)) return Var;
  }
  return NULL;
}

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

static Node *newVarNode(Obj *Var){
    Node *Nd = newNode(ND_VAR);
    Nd->Var = Var;
    return Nd;
}

//Create a new Var in list
static Obj *newLVar(char *Name){
  Obj *Var = calloc(1, sizeof(Obj));
  Var->Name = Name;
  Var->Next = Locals;
  Locals = Var;
  return Var;
}


// 解析语句
static Node *stmt(Token **Rest, Token *Tok){ return exprStmt(Rest, Tok);}
static Node *exprStmt(Token **Rest, Token *Tok){
    Node *Nd = newUnary(ND_EXPR_STMT, expr(&Tok, Tok));
    *Rest = skip(Tok, ";");
    return Nd;
}
static Node *expr(Token **Rest, Token *Tok) { return assign(Rest, Tok); }

static Node *assign(Token **Rest, Token *Tok){
    Node *Nd = equality(&Tok, Tok);

    // 可能存在递归赋值，如a=b=1
    // ("=" assign)?
    if(equal(Tok, "=")){
        Nd = newBinary(ND_ASSIGN, Nd, assign(&Tok, Tok->Next));
    }
    *Rest = Tok;
    return Nd;
}
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

    // ident
    if(Tok->Kind == TK_IDENT){
      Obj *Var = findVar(Tok);

      if(!Var)
        Var = newLVar(strndup(Tok->Loc, Tok->Len));
      
      *Rest = Tok->Next;
      return newVarNode(Var);
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

Function *parse(Token *Tok){
    //Node *Nd = expr(&Tok, Tok);

    Node Head = {};
    Node *Cur = &Head;
    while(Tok->Kind != TK_EOF){
        Cur->Next = stmt(&Tok, Tok);
        Cur = Cur->Next;
    }

    // 函数体存储语句的AST，Locals存储变量
    Function *Prog = calloc(1,sizeof(Function));
    Prog->Body = Head.Next;
    Prog->Locals = Locals;
    return Prog;
}