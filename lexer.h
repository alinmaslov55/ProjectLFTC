#pragma once
#include <string.h>

enum{
	ID
	// keywords
	,VAR  // 'var'
	,FUNCTION // 'function'
	,IF // 'if'
	,ELSE // 'else'
	,WHILE // 'while'
	,END // 'end'
	,RETURN // 'return'
	,TYPE_INT // 'int'
	,TYPE_REAL // 'real'
	,TYPE_STR // 'str'
	// delimiters
	,COMMA // ','
	,SEMICOLON // ';'
	,COLON // ':'
	,LPAR // '('
	,RPAR // ')'
	,FINISH // '\0' | EOF
	// operators
	,ADD // '+'
	,SUB // '-'
	,MUL // '*'
	,DIV // '/'
	,AND // '&&'
	,OR // '||'
	,NOT // '!'
	,ASSIGN // '='
	,EQUAL // '=='
	,NOTEQ // '!='
	,LESS // '<'
	,GREATER // '>'
	,GREATEREQ // '>='
	,LESSEQ // '<='
	,STR // string literal
	,INT // integer literal
	,REAL // real literal
	};

	// SPACE; [ \n\t\r]+
	// COMMENT; '//'[^'\n']*('\n'|'\0')

#define MAX_STR		127

typedef struct{
	int code;		// ID, TYPE_INT, ...
	int line;		// the line from the input file
	union{
		char text[MAX_STR+1];		// the chars for ID, STR
		int i;		// the value for INT
		double r;		// the value for REAL
		};
	}Token;

#define MAX_TOKENS		4096
extern Token tokens[];
extern int nTokens;

void tokenize(const char *pch);
void showTokens();
void tokenPrint(Token tk);
char* deleteComments(const char* src);