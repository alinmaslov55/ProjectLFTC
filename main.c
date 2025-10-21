#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "utils.h"
#include "parser.h"

#define FILENAME "1.q"

int main(){
    char* source = loadFile(FILENAME);

    char* deleted_comments = deleteComments(source);
    tokenize(deleted_comments);

    // printf("\nt---------------\n");
    // showTokens();

    /* run the parser on the produced tokens */
    parse();

    free(deleted_comments);
    free(source);
    return 0;
}