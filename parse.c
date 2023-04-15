#include "rvcc.h"

// 在解析时，全部的变量实例都被累加到这个列表里。
Obj *Locals;

// program = "{" compoundStmt   
// compoundStmt = stmt* "}"     
// stmt = "return" expr ";" 
//      | "if" "(" expr ")" stmt ("else" stmt)?
//      | "for" "(" expr ")" stmt ("else" stmt)?
//      | "while" "(" expr ")" stmt
//      | "{" compoundStmt 
//      | exprStmt 
// exprStmt = expr ";"
// expr = assign 
// assign = equality ("=" assign)?
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-") unary | primary
// primary = "(" expr ")" | ident | num

static Node *compoundStmt(Token **Rest, Token *Tok);
static Node *exprStmt(Token **Rest, Token *Tok);
static Node *expr(Token **Rest, Token *Tok);
static Node *assign(Token **Rest, Token *Tok);
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

static Node *newNode(NodeKind Kind, Token *Tok) {
  Node *Nd = calloc(1, sizeof(Node));
  Nd->Kind = Kind;
  Nd->Tok = Tok;
  return Nd;
}

static Node *newUnary(NodeKind Kind, Node *Expr, Token *Tok) {
  Node *Nd = newNode(Kind, Tok);
  Nd->LHS = Expr;
  return Nd;
}

static Node *newBinary(NodeKind Kind, Node *LHS, Node *RHS, Token *Tok) {
  Node *Nd = newNode(Kind,Tok);
  Nd->LHS = LHS;
  Nd->RHS = RHS;
  return Nd;
}

static Node *newNum(int Val, Token *Tok) {
  Node *Nd = newNode(ND_NUM, Tok);
  Nd->Val = Val;
  return Nd;
}

static Node *newVarNode(Obj *Var, Token *Tok){
    Node *Nd = newNode(ND_VAR, Tok);
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
static Node *stmt(Token **Rest, Token *Tok){ 
  if(equal(Tok, "return")){
    Node *Nd = newUnary(ND_RETURN, expr(&Tok, Tok->Next), Tok)  ;
    *Rest = skip(Tok, ";");
    return Nd;
  }

  if(equal(Tok, "if")){
    Node *Nd = newNode(ND_IF, Tok);
    Tok = skip(Tok->Next, "(");
    Nd->Cond = expr(&Tok, Tok);
    Tok = skip(Tok, ")");
    Nd->Then = stmt(&Tok, Tok);

    if(equal(Tok, "else"))
      Nd->Els = stmt(&Tok, Tok->Next);
    
    *Rest = Tok;
    return Nd;
  }

  if(equal(Tok, "for")){
    Node *Nd = newNode(ND_FOR, Tok);
    Tok = skip(Tok->Next, "(");

    Nd->Init = exprStmt(&Tok, Tok);

    if(!equal(Tok, ";"))
      Nd->Cond = expr(&Tok, Tok);
    
    Tok = skip(Tok, ";");

    if(!equal(Tok, ")"))
      Nd->Inc = expr(&Tok, Tok);
    Tok = skip(Tok, ")");

    Nd->Then = stmt(Rest, Tok);
    return Nd;
  }

  if(equal(Tok, "while")){
    Node *Nd = newNode(ND_FOR, Tok);

    Tok = skip(Tok->Next, "(");

    Nd->Cond = expr(&Tok, Tok);

    Tok = skip(Tok, ")");
    
    Nd->Then = stmt(Rest, Tok);
    return Nd;
  }


  if(equal(Tok, "{"))
    return compoundStmt(Rest, Tok->Next);

  return exprStmt(Rest, Tok);
}

static Node *compoundStmt(Token **Rest, Token *Tok){
  Node Head = {};
  Node *Cur = &Head;
  while(!equal(Tok, "}")){
    Cur->Next = stmt(&Tok, Tok);
    Cur = Cur->Next;
  }

  Node *Nd = newNode(ND_BLOCK, Tok);
  Nd->Body = Head.Next;
  *Rest = Tok->Next;
  return Nd;
}

static Node *exprStmt(Token **Rest, Token *Tok){
    if(equal(Tok, ";")){
      *Rest = Tok->Next;
      return newNode(ND_BLOCK, Tok);
    }

    Node *Nd = newUnary(ND_EXPR_STMT, expr(&Tok, Tok), Tok);
    *Rest = skip(Tok, ";");
    return Nd;
}
static Node *expr(Token **Rest, Token *Tok) { return assign(Rest, Tok); }

static Node *assign(Token **Rest, Token *Tok){
    Node *Nd = equality(&Tok, Tok);

    // 可能存在递归赋值，如a=b=1
    // ("=" assign)?
    if(equal(Tok, "=")){
        Nd = newBinary(ND_ASSIGN, Nd, assign(&Tok, Tok->Next), Tok);
    }
    *Rest = Tok;
    return Nd;
}
static Node *equality(Token **Rest, Token *Tok) {
  Node *Nd = relational(&Tok, Tok);
  while (true) {
    Token *Start = Tok;
    if (equal(Tok, "==")) {
      Nd = newBinary(ND_EQ, Nd, relational(&Tok, Tok->Next), Start);
      continue;
    }
    if (equal(Tok, "!=")) {
      Nd = newBinary(ND_NE, Nd, relational(&Tok, Tok->Next), Start);
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

static Node *relational(Token **Rest, Token *Tok) {
  Node *Nd = add(&Tok, Tok);

  while (true) {
    Token *Start = Tok;
    if (equal(Tok, "<")) {
      Nd = newBinary(ND_LT, Nd, add(&Tok, Tok->Next), Start);
      continue;
    }
    if (equal(Tok, "<=")) {
      Nd = newBinary(ND_LE, Nd, add(&Tok, Tok->Next), Start);
      continue;
    }
    if (equal(Tok, ">")) {
      Nd = newBinary(ND_LT, add(&Tok, Tok->Next), Nd, Start);
      continue;
    }
    if (equal(Tok, ">=")) {
      Nd = newBinary(ND_LE, add(&Tok, Tok->Next), Nd, Start);
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

static Node *add(Token **Rest, Token *Tok) {
  Node *Nd = mul(&Tok, Tok);
  while (true) {
    Token *Start = Tok;
    if (equal(Tok, "+")) {
      Nd = newBinary(ND_ADD, Nd, mul(&Tok, Tok->Next), Start);
      continue;
    }

    if (equal(Tok, "-")) {
      Nd = newBinary(ND_SUB, Nd, mul(&Tok, Tok->Next), Start);
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
      Nd = newBinary(ND_MUL, Nd, unary(&Tok, Tok->Next), Tok);
      continue;
    }
    if (equal(Tok, "/")) {
      Nd = newBinary(ND_DIV, Nd, unary(&Tok, Tok->Next), Tok);
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
    return newUnary(ND_NEG, unary(Rest, Tok->Next), Tok);

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
      return newVarNode(Var, Tok);
    }

    // num
    if (Tok->Kind == TK_NUM) {
        Node *Nd = newNum(Tok->Val, Tok);
        *Rest = Tok->Next;
        return Nd;
    }
    errorTok(Tok, "expected an expression");
    return NULL;
}

Function *parse(Token *Tok){
    Tok = skip(Tok, "{");

    // 函数体存储语句的AST，Locals存储变量
    Function *Prog = calloc(1,sizeof(Function));
    Prog->Body = compoundStmt(&Tok, Tok);
    Prog->Locals = Locals;
    return Prog;
}