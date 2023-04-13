#include "rvcc.h"

// Input string
static char *CurrentInput;

//================ error System ===================
void error(char *Fmt,...){
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
void errorAt(char *Loc, char *Fmt, ...){
    va_list VA;
    va_start(VA, Fmt);
    verrorAt(Loc, Fmt, VA);
    exit(1);
}
void errorTok(Token *Tok, char *Fmt, ...){
    va_list VA;
    va_start(VA, Fmt);
    verrorAt(Tok->Loc, Fmt, VA);
    exit(1);
}


//================= Others ===================
bool equal(Token *Tok,char *Str){
    return memcmp(Tok->Loc,Str,Tok->Len) == 0 && Str[Tok->Len] == '\0';
}

Token *skip(Token *Tok,char *Str){
    if(!equal(Tok, Str))
        errorTok(Tok,"expect a number");
    return Tok->Next;
}

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

static bool startsWith(char *Str, char *SubStr) {
  return strncmp(Str, SubStr, strlen(SubStr)) == 0;
}

static int readPunct(char *Ptr) {
  if (startsWith(Ptr, "==") || startsWith(Ptr, "!=") || startsWith(Ptr, "<=") ||
      startsWith(Ptr, ">="))
    return 2;
  return ispunct(*Ptr) ? 1 : 0;
}


Token *tokenize(char *P){
    CurrentInput = P;
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

        // sign 
        if('a' <= *P && *P <= 'z'){
            Cur->Next = newToken(TK_IDENT, P, P + 1);
            Cur = Cur->Next;
            ++P;
            continue;
        }

        // op
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
