#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "utils.h"

#define FILENAME "1.q"

int main(){
    char* source = loadFile(FILENAME);

    tokenize(deleteComments(source));

    printf("---------------\n");
    showTokens();

    free(source);
    return 0;
}