#include "rvcc.h"

// 在解析时，全部的变量实例都被累加到这个列表里。
Obj *Locals;

// program = functionDefinition*
// functionDefinition = declspec declarator "{" compoundStmt*
// declspec = "int"
// declarator = "*"* ident typeSuffix
// typeSuffix = ("(" funcParams? ")")?
// funcParams = param ("," param)*
// param = declspec declarator

// compoundStmt = (declaration | stmt)* "}"
// declaration =
//    declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
// stmt = "return" expr ";"
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "for" "(" exprStmt expr? ";" expr? ")" stmt
//        | "while" "(" expr ")" stmt
//        | "{" compoundStmt
//        | exprStmt
// exprStmt = expr? ";"
// expr = assign
// assign = equality ("=" assign)?
// equality = relational ("==" relational | "!=" relational)*
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
// add = mul ("+" mul | "-" mul)*
// mul = unary ("*" unary | "/" unary)*
// unary = ("+" | "-" | "*" | "&") unary | primary
// primary = "(" expr ")" | ident func-args? | num
// funcall = ident "(" (assign ("," assign)*)? ")"

static Type *declspec(Token **Rest, Token *Tok);
static Type *declarator(Token **Rest, Token *Tok, Type *Ty);
static Node *declaration(Token **Rest, Token *Tok);
static Node *compoundStmt(Token **Rest, Token *Tok);
static Node *stmt(Token **Rest, Token *Tok);
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
static Obj *newLVar(char *Name,Type *Ty){
  Obj *Var = calloc(1, sizeof(Obj));
  Var->Name = Name;
  Var->Ty = Ty;
  Var->Next = Locals;
  Locals = Var;
  return Var;
}

// 获取标识符
static char *getIdent(Token *Tok){
  if(Tok->Kind != TK_IDENT)
    errorTok(Tok, "expected an identifier");
  return strndup(Tok->Loc, Tok->Len);
}

static Type *declspec(Token **Rest, Token *Tok){
  *Rest = skip(Tok, "int");
  return TyInt;
}


// typeSuffix = ("(" funcParams? ")")?
// funcParams = param ("," param)*
// param = declspec declarator
static Type *typeSuffix(Token **Rest, Token *Tok, Type *Ty) {
  // ("(" funcParams? ")")?
  if (equal(Tok, "(")) {
    Tok = Tok->Next;

    Type Head = {};
    Type *Cur = &Head;

    while (!equal(Tok, ")")) {
      if (Cur != &Head)
        Tok = skip(Tok, ",");
      Type *BaseTy = declspec(&Tok, Tok);
      Type *DeclarTy = declarator(&Tok, Tok, BaseTy);
      Cur->Next = copyType(DeclarTy);
      Cur = Cur->Next;
    }

    Ty = funcType(Ty);
    Ty->Params = Head.Next;
    *Rest = Tok->Next;
    return Ty;
  }
  *Rest = Tok;
  return Ty;
}


// declarator = "*"* ident
static Type *declarator(Token **Rest, Token *Tok, Type *Ty){
  while(consume(&Tok, Tok, "*"))
    Ty = pointerTo(Ty);
  
  if(Tok->Kind != TK_IDENT)
    errorTok(Tok, "expected a variable name");

  Ty = typeSuffix(Rest, Tok->Next, Ty);
  Ty->Name = Tok;
  return Ty;
}

static Node *declaration(Token **Rest, Token *Tok){
  Type *Basety = declspec(&Tok, Tok);

  Node Head = {};
  Node *Cur = &Head;

  int I = 0;

  while(!equal(Tok, ";")){
    // 第1个变量不必匹配 ","
    if(I++ > 0)
      Tok = skip(Tok, ",");

    Type *Ty = declarator(&Tok, Tok, Basety);
    Obj *Var = newLVar(getIdent(Ty->Name), Ty);

    if(!equal(Tok, "="))
      continue;

    Node *LHS = newVarNode(Var, Ty->Name);

    Node *RHS = assign(&Tok, Tok->Next);
    Node *Node = newBinary(ND_ASSIGN, LHS, RHS, Tok);

    Cur->Next = newUnary(ND_EXPR_STMT, Node, Tok);
    Cur = Cur->Next;
  }

  Node *Nd = newNode(ND_BLOCK, Tok);
  Nd->Body = Head.Next;
  *Rest = Tok->Next;
  return Nd;
}

// 解析语句
static Node *stmt(Token **Rest, Token *Tok){ 
  if(equal(Tok, "return")){
    Node *Nd = newNode(ND_RETURN, Tok);
    Nd->LHS = expr(&Tok, Tok->Next);
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

    if(equal(Tok, "int")){
      Cur->Next = declaration(&Tok, Tok);
    }
    else {
      Cur->Next = stmt(&Tok, Tok);
    }
    Cur = Cur->Next;

    addType(Cur);
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

  Node *Nd = newNode(ND_EXPR_STMT, Tok);
  Nd->LHS = expr(&Tok, Tok);
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

// 解析各种加法
static Node *newAdd(Node *LHS, Node *RHS, Token *Tok){
  addType(LHS);
  addType(RHS);

  // num + num
  if(isInteger(LHS->Ty) && isInteger(RHS->Ty)){
    return newBinary(ND_ADD, LHS, RHS, Tok);
  }

  // 不能解析 ptr + ptr
  if(LHS->Ty->Base && RHS->Ty->Base)
    errorTok(Tok, "invalid operands");

  // 将 num + ptr 转换为 ptr + num
  if(LHS->Ty->Base && RHS->Ty->Base){
    Node *Tmp = LHS;
    LHS = RHS;
    RHS = Tmp;
  }

  // ptr + num
  // 指针加法，ptr+1，这里的1不是1个字节，而是1个元素的空间，所以需要 ×8 操作
  RHS = newBinary(ND_MUL, RHS, newNum(8,Tok), Tok);
  return newBinary(ND_ADD, LHS, RHS, Tok);
}


static Node *newSub(Node *LHS, Node *RHS, Token *Tok){
  addType(LHS);
  addType(RHS);

  if(isInteger(LHS->Ty) && isInteger(RHS->Ty)){
    return newBinary(ND_SUB, LHS, RHS, Tok);
  }

  if(LHS->Ty->Base && isInteger(RHS->Ty)){
    RHS = newBinary(ND_MUL, RHS, newNum(8, Tok), Tok);
    addType(RHS);
    Node *Nd = newBinary(ND_SUB, LHS, RHS, Tok);
    Nd->Ty = LHS->Ty;
    return Nd;
  }

  // ptr - ptr，返回两指针间有多少元素
  if(LHS->Ty->Base && RHS->Ty->Base){
    Node *Nd = newBinary(ND_SUB, LHS, RHS, Tok);
    Nd->Ty = TyInt;
    return newBinary(ND_DIV, Nd, newNum(8, Tok), Tok);
  }

  errorTok(Tok, "invalid operands");
  return NULL;
}

static Node *add(Token **Rest, Token *Tok) {
  Node *Nd = mul(&Tok, Tok);
  while (true) {
    Token *Start = Tok;
    if (equal(Tok, "+")) {
      Nd = newAdd(Nd, mul(&Tok, Tok->Next), Start);
      continue;
    }

    if (equal(Tok, "-")) {
      Nd = newSub(Nd, mul(&Tok, Tok->Next), Start);
      continue;
    }

    *Rest = Tok;
    return Nd;
  }
}

static Node *mul(Token **Rest, Token *Tok) {
  Node *Nd = unary(&Tok, Tok);
  while (true) {
    Token *Start = Tok;
    if (equal(Tok, "*")) {
      Nd = newBinary(ND_MUL, Nd, unary(&Tok, Tok->Next), Start);
      continue;
    }
    if (equal(Tok, "/")) {
      Nd = newBinary(ND_DIV, Nd, unary(&Tok, Tok->Next), Start);
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

  if(equal(Tok, "&"))
    return newUnary(ND_ADDR, unary(Rest, Tok->Next), Tok);

  if(equal(Tok, "*"))
    return newUnary(ND_DEREF, unary(Rest, Tok->Next), Tok);

  return primary(Rest, Tok);
}

static Node *funCall(Token **Rest, Token *Tok) {
  Token *Start = Tok;
  Tok = Tok->Next->Next;

  Node Head = {};
  Node *Cur = &Head;

  while (!equal(Tok, ")")) {
    if (Cur != &Head)
      Tok = skip(Tok, ",");
    // assign
    Cur->Next = assign(&Tok, Tok);
    Cur = Cur->Next;
  }

  *Rest = skip(Tok, ")");

  Node *Nd = newNode(ND_FUNCALL, Start);
  // ident
  Nd->FuncName = strndup(Start->Loc, Start->Len);
  Nd->Args = Head.Next;
  return Nd;
}



static Node *primary(Token **Rest, Token *Tok) {
    if (equal(Tok, "(")) {
        Node *Nd = expr(&Tok, Tok->Next);
        *Rest = skip(Tok, ")");
        return Nd;
    }

    // ident
    if(Tok->Kind == TK_IDENT){
      // 函数调用
      if (equal(Tok->Next, "("))
        return funCall(Rest, Tok);
      
      Obj *Var = findVar(Tok);
      // 如果变量不存在，就在链表中新增一个变量
      if(!Var)
        errorTok(Tok, "undefined variable");
      
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


static void createParamLVars(Type *Param) {
  if (Param) {
    createParamLVars(Param->Next);
    newLVar(getIdent(Param->Name), Param);
  }
}

// functionDefinition = declspec declarator "{" compoundStmt*
static Function *function(Token **Rest, Token *Tok) {
  Type *Ty = declspec(&Tok, Tok);
  Ty = declarator(&Tok, Tok, Ty);

  Locals = NULL;

  // 从解析完成的Ty中读取ident
  Function *Fn = calloc(1, sizeof(Function));
  Fn->Name = getIdent(Ty->Name);
  createParamLVars(Ty->Params);
  Fn->Params = Locals;

  Tok = skip(Tok, "{");
  Fn->Body = compoundStmt(Rest, Tok);
  Fn->Locals = Locals;
  return Fn;
}


Function *parse(Token *Tok){
  Function Head = {};
  Function *Cur = &Head;

  while (Tok->Kind != TK_EOF)
    Cur = Cur->Next = function(&Tok, Tok);
  return Head.Next;
}