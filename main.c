#include "rvcc.h"

int main(int Argc , char **Argv){
    if( Argc != 2){
        error("%s: invalid number of arguments\n", Argv[0]);
    }
    //[3]:解析Argv[1]
    Token *Tok = tokenize(Argv[1]);
    Node *Nd = parse(Tok);
    codegen(Nd);
    return 0;
}