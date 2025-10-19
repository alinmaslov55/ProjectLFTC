#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "lexer.h"
#include "utils.h"

Token tokens[MAX_TOKENS];
int nTokens;

int line = 1; // the current line in the input file

// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code)
{
	if (nTokens == MAX_TOKENS)
		err("too many tokens");
	Token *tk = &tokens[nTokens];
	tk->code = code;
	tk->line = line;
	nTokens++;
	printf("\n%d ", tk->line);
	tokenPrint(*tk);
	return tk;
}
Token *addTkInt(int value){
	if (nTokens == MAX_TOKENS)
		err("too many tokens");
	Token *tk = &tokens[nTokens];
	tk->code = INT;
	tk->i = value;
	nTokens++;
	tk->line = line;
	printf("\n%d ", tk->line);
	tokenPrint(*tk);
	return tk;
}
Token *addTkReal(double value){
	if (nTokens == MAX_TOKENS)
		err("too many tokens");
	Token *tk = &tokens[nTokens];
	tk->code = REAL;
	tk->r = value;
	tk->line = line;
	nTokens++;
	printf("\n%d ", tk->line);
	tokenPrint(*tk);
	return tk;
}
Token *addTkString(char* value){
	if (nTokens == MAX_TOKENS)
		err("too many tokens");
	Token *tk = &tokens[nTokens];
	tk->code = STR;
	tk->line = line;
	strcpy(tk->text, value);
	nTokens++;
	printf("\n%d ", tk->line);
	tokenPrint(*tk);
	return tk;
}

// copy in the dst buffer the string between [begin,end)
char *copyn(char *dst, const char *begin, const char *end)
{
	char *p = dst;
	if (end - begin > MAX_STR)
		err("string too long");
	while (begin != end)
		*p++ = *begin++;
	*p = '\0';
	return dst;
}

char *deleteComments(const char *src){
	const char *p = src;
	char *result = (char *)safeAlloc(strlen(src) + 1); // Allocate memory for the result
	char *q = result;

	while (*p != '\0') {
		if (p[0] == '/' && p[1] == '/') { // Start of a comment
			while (*p != '\n' && *p != '\0') {
				p++; // Skip until the end of the line or end of string
			}
		} else {
			*q++ = *p++; // Copy non-comment characters
		}
	}
	*q = '\0'; // Null-terminate the result string

	return result;
}

void tokenize(const char *pch) // mai trebuie constante
{
	const char *start;
	Token *tk;
	char buf[MAX_STR + 1];
	for (;;)
	{
		switch (*pch)
		{
		case ' ':
		case '\t':
			pch++;
			break;
		case '\r': // handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
			if (pch[1] == '\n')
				pch++;
			// fallthrough to \n
			break;
		case '\n':
			line++;
			pch++;
			break;
		case '\0':
			addTk(FINISH);
			return;
		case ',':
			addTk(COMMA);
			pch++;
			break;
		case ';':
			addTk(SEMICOLON);
			pch++;
			break;
		case ':':
			addTk(COLON);
			pch++;
			break;
		case '(':
			addTk(LPAR);
			pch++;
			break;
		case ')':
			addTk(RPAR);
			pch++;
			break;
		case '+':
			addTk(ADD);
			pch++;
			break;
		case '-':
			addTk(SUB);
			pch++;
			break;
		case '*':
			addTk(MUL);
			pch++;
			break;
		case '/':
			addTk(DIV);
			pch++;
			break;
		case '&':
			if (pch[1] == '&')
			{
				addTk(AND);
				pch += 2;
			}
			else
			{
				err("invalid char: %c (%d)", *pch, *pch);
			}
			break;
		case '|':
			if (pch[1] == '|')
			{
				addTk(OR);
				pch += 2;
			}
			else
			{
				err("invalid char: %c (%d)", *pch, *pch);
			}
			break;
		case '>':
			if (pch[1] == '=')
			{
				addTk(GREATEREQ);
				pch += 2;
			}
			else
			{
				addTk(GREATER);
				pch++;
			}
			break;
		case '<':
			if (pch[1] == '=')
			{
				addTk(LESSEQ);
				pch += 2;
			}
			else
			{
				addTk(LESS);
				pch++;
			}
			break;
		case '=':
			if (pch[1] == '=')
			{
				addTk(EQUAL);
				pch += 2;
			}
			else
			{
				addTk(ASSIGN);
				pch++;
			}
			break;
		case '!':
			if (pch[1] == '=')
			{
				addTk(NOTEQ);
				pch += 2;
			}
			else
			{
				addTk(NOT);
				pch++;
			}
			break;
			case '"':
        {
            pch++; // skip opening "
            start = pch;
            while (*pch != '"' && *pch != '\0' && *pch != '\n')
                pch++;
            if (*pch != '"')
                err("unterminated string literal started on line %d", line);
            char *text = copyn(buf, start, pch);
            tk = addTkString(text);
            strcpy(tk->text, text);
            pch++; // skip closing "
            break;
        }
		default: // id, mai trebui cuvinte cheie
			if (isalpha(*pch) || *pch == '_')
			{
				for (start = pch++; isalnum(*pch) || *pch == '_'; pch++)
				{
				}
				char *text = copyn(buf, start, pch);
				if (strcmp(text, "int") == 0)
				{
					addTk(TYPE_INT);
				} // alte cuvinte cheie
				else if (strcmp(text, "real") == 0)
				{
					addTk(TYPE_REAL);
				}
				else if (strcmp(text, "str") == 0)
				{
					addTk(TYPE_STR);
				}
				else if (strcmp(text, "var") == 0)
				{
					addTk(VAR);
				}
				else if (strcmp(text, "function") == 0)
				{
					addTk(FUNCTION);
				}
				else if (strcmp(text, "if") == 0)
				{
					addTk(IF);
				}
				else if (strcmp(text, "else") == 0)
				{
					addTk(ELSE);
				}
				else if (strcmp(text, "while") == 0)
				{
					addTk(WHILE);
				}
				else if (strcmp(text, "end") == 0)
				{
					addTk(END);
				}
				else if (strcmp(text, "return") == 0)
				{
					addTk(RETURN);
				}
				else
				{
					tk = addTk(ID);
					strcpy(tk->text, text);
				}
			}
			else if(isdigit(*pch))
			{
				// Numeric literal
                for (start = pch; isdigit(*pch); pch++) {}
				if (*pch == '.') // Real constant
				{
					pch++;
					if(!isdigit(*pch))
						err("invalid real constant on line %d - a digit is expected after '.'", line);
					while (isdigit(*pch)) pch++;
					char *text = copyn(buf, start, pch);
					tk = addTkReal(atof(text));
				}
				else // Integer constant
				{
					char *text = copyn(buf, start, pch);
					tk = addTkInt(atoi(text));
				}
			}
			else
				err("invalid char: %c (ASCII: %d) on line %d", *pch, *pch, line);
		}
	}
}

void showTokens()
{
	for (int i = 0; i < nTokens; i++)
	{
		Token *tk = &tokens[i];
		printf("\n%d ", tk->line);
		tokenPrint(*tk);
	}
}

void tokenPrint(Token tk)
{
	switch (tk.code)
	{
	case ID:
		printf("ID:%s", tk.text);
		break;
	case VAR:
		printf("VAR");
		break;
	case FUNCTION:
		printf("FUNCTION");
		break;
	case IF:
		printf("IF");
		break;
	case ELSE:
		printf("ELSE");
		break;
	case WHILE:
		printf("WHILE");
		break;
	case END:
		printf("END");
		break;
	case RETURN:
		printf("RETURN");
		break;
	case TYPE_INT:
		printf("TYPE_INT");
		break;
	case TYPE_REAL:
		printf("TYPE_REAL:");
		break;
	case TYPE_STR:
		printf("TYPE_STR");
		break;
	case COMMA:
		printf("COMMA");
		break;
	case SEMICOLON:
		printf("SEMICOLON");
		break;
	case COLON:
		printf("COLON");
		break;
	case LPAR:
		printf("LPAR");
		break;
	case RPAR:
		printf("RPAR");
		break;
	case FINISH:
		printf("FINISH");
		break;
	case ADD:
		printf("ADD");
		break;
	case SUB:
		printf("SUB");
		break;
	case MUL:
		printf("MUL");
		break;
	case DIV:
		printf("DIV");
		break;
	case AND:
		printf("AND");
		break;
	case OR:
		printf("OR");
		break;
	case NOT:
		printf("NOT");
		break;
	case ASSIGN:
		printf("ASSIGN");
		break;
	case EQUAL:
		printf("EQUAL");
		break;
	case NOTEQ:
		printf("NOTEQ");
		break;
	case LESS:
		printf("LESS");
		break;
	case GREATER:
		printf("GREATER");
		break;
	case GREATEREQ:
		printf("GREATEREQ");
		break;
	case LESSEQ:
		printf("LESSEQ");
		break;
	case STR:
		printf("STR:\"%s\"", tk.text);
		break;
	case INT:
		printf("INT:%d", tk.i);
		break;
	case REAL:
		printf("REAL:%g", tk.r);
		break;
	default:
		printf("UNKNOWN TOKEN");
	}
}