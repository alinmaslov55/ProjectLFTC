#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"
#include "ad.h"

int iTk;	// the iterator in tokens
Token *consumed;	// the last consumed token


// defining missing functions

// forward declarations
bool block();
_Noreturn void tkerr(const char *fmt,...);
bool defVar();
bool defFunc();
bool consume(int code);
bool baseType();
bool funcParams();
bool funcParam();
bool instr();
bool expr();
bool exprLogic();
bool exprAssign();
bool exprComp();
bool exprAdd();
bool exprMul();
bool exprPrefix();
bool factor();

// baseType ::= TYPE_INT | TYPE_REAL | TYPE_STR
bool baseType(){
    if(tokens[iTk].code == TYPE_INT){
        consume(TYPE_INT);
        ret.type = TYPE_INT;
        ret.lval = false;
        return true;
    }
    if(tokens[iTk].code == TYPE_REAL){
        consume(TYPE_REAL);
        ret.type = TYPE_REAL;
        ret.lval = false;
        return true;
    }
    if(tokens[iTk].code == TYPE_STR){
        consume(TYPE_STR);
        ret.type = TYPE_STR;
        ret.lval = false;
        return true;
    }
    return false;
}

// defVar ::= VAR ID COLON baseType SEMICOLON
bool defVar(){
    if(tokens[iTk].code != VAR) return false;
    consume(VAR);
    if(!consume(ID)) tkerr("expected identifier after 'var'");
    /* semantic action: add symbol for the variable */
    const char *name = consumed->text;
    Symbol *s = searchInCurrentDomain(name);
    if(s) tkerr("symbol redefinition: %s", name);
    s = addSymbol(name, KIND_VAR);
    s->local = crtFn != NULL;

    if(!consume(COLON)) tkerr("expected ':' after identifier in var declaration");
    if(!baseType()) tkerr("expected type after ':' in var declaration");
    /* semantic action: set the type of the newly added symbol from ret */
    s->type = ret.type;

    if(!consume(SEMICOLON)) tkerr("expected ';' after var declaration");
    return true;
}

bool defFunc(){
    if(tokens[iTk].code != FUNCTION) return false;
    consume(FUNCTION);
    if(!consume(ID)) tkerr("expected function name after 'function'");
    /* semantic action: create function symbol and new domain */
    const char *name = consumed->text;
    Symbol *s = searchInCurrentDomain(name);
    if(s) tkerr("symbol redefinition: %s", name);
    crtFn = addSymbol(name, KIND_FN);
    crtFn->args = NULL;
    addDomain(); /* new domain for function body and parameters */
    if(!consume(LPAR)) tkerr("expected '(' after function name");
    if(tokens[iTk].code != RPAR){
        if(!funcParams()) tkerr("invalid function parameters");
    }
    if(!consume(RPAR)) tkerr("expected ')' after function parameters");
    if(!consume(COLON)) tkerr("expected ':' after function header");
    if(!baseType()) tkerr("expected return type after ':' in function header");
    /* semantic action: set function return type */
    crtFn->type = ret.type;
    while(defVar()){}
    if(!block()) tkerr("expected function body (block)");
    if(!consume(END)) tkerr("expected 'end' after function body");
    /* leave function domain */
    delDomain();
    crtFn = NULL;
    return true;
}

bool block(){
    int cnt = 0;
    for(;;){
        int c = tokens[iTk].code;
        /* tokens that cannot start an instr and thus end a block */
        if(c==END || c==ELSE || c==FINISH) break;
        if(!instr()) break;
        cnt++;
    }
    return cnt>0;
}

// funcParams ::= funcParam ( COMMA funcParam )*
bool funcParams(){
    if(!funcParam()) return false;
    while(consume(COMMA)){
        if(!funcParam()) tkerr("expected parameter after ','");
    }
    return true;
}

// funcParam ::= ID COLON baseType
bool funcParam(){
    if(!consume(ID)) return false;
    const char *argName = consumed->text;
    /* check redefinition in current domain */
    Symbol *s = searchInCurrentDomain(argName);
    if(s) tkerr("symbol redefinition: %s", argName);
    if(!consume(COLON)) tkerr("expected ':' after parameter name");
    if(!baseType()) tkerr("expected type after ':' in parameter");
    /* semantic action: add parameter to current domain and to function's arg list */
    s = addSymbol(argName, KIND_ARG);
    s->local = crtFn != NULL;
    s->type = ret.type;
    /* also register in the function's args list and set its type */
    Symbol *sFnParam = addFnArg(crtFn, argName);
    if(sFnParam) sFnParam->type = ret.type;
    return true;
}

// instr ::= expr? SEMICOLON
// | IF LPAR expr RPAR block ( ELSE block )? END
// | RETURN expr SEMICOLON
// | WHILE LPAR expr RPAR block END
bool instr(){
    int c = tokens[iTk].code;
    if(c==IF){
        consume(IF);
        if(!consume(LPAR)) tkerr("expected '(' after 'if'");
        if(!expr()) tkerr("expected expression in if condition");
        if(!consume(RPAR)) tkerr("expected ')' after if condition");
        if(!block()) tkerr("expected block after if condition");
        if(consume(ELSE)){
            if(!block()) tkerr("expected block after else");
        }
        if(!consume(END)) tkerr("expected 'end' after if");
        return true;
    }
    if(c==RETURN){
        consume(RETURN);
        if(!expr()) tkerr("expected expression after 'return'");
        if(!consume(SEMICOLON)) tkerr("expected ';' after return expression");
        return true;
    }
    if(c==WHILE){
        consume(WHILE);
        if(!consume(LPAR)) tkerr("expected '(' after 'while'");
        if(!expr()) tkerr("expected expression in while condition");
        if(!consume(RPAR)) tkerr("expected ')' after while condition");
        if(!block()) tkerr("expected block after while condition");
        if(!consume(END)) tkerr("expected 'end' after while");
        return true;
    }
    /* otherwise expr? SEMICOLON */
    if(tokens[iTk].code==SEMICOLON){
        consume(SEMICOLON);
        return true;
    }
    if(expr()){
        if(!consume(SEMICOLON)) tkerr("expected ';' after expression");
        return true;
    }
    return false;
}

// expr ::= exprLogic
bool expr(){
    return exprLogic();
}

// exprLogic ::= exprAssign ( ( AND | OR ) exprAssign )*
bool exprLogic(){
    if(!exprAssign()) return false;
    while(tokens[iTk].code==AND || tokens[iTk].code==OR){
        int op = tokens[iTk].code;
        consume(op);
        if(!exprAssign()){
            if(op==AND) tkerr("expected expression after '%s'", "&&");
            else tkerr("expected expression after '%s'", "||");
        }
    }
    return true;
}

// exprAssign ::= ( ID ASSIGN )? exprComp
bool exprAssign(){
    if(tokens[iTk].code==ID && tokens[iTk+1].code==ASSIGN){
        consume(ID);
        consume(ASSIGN);
        if(!exprComp()) tkerr("expected expression after '='");
        return true;
    }
    return exprComp();
}

// exprComp ::= exprAdd ( ( LESS | EQUAL ) exprAdd )?
bool exprComp(){
    if(!exprAdd()) return false;
    if(tokens[iTk].code==LESS || tokens[iTk].code==EQUAL){
        int op = tokens[iTk].code;
        consume(op);
        if(!exprAdd()){
            if(op==LESS) tkerr("expected expression after '%s'", "<");
            else tkerr("expected expression after '%s'", "==");
        }
    }
    return true;
}

// exprAdd ::= exprMul ( ( ADD | SUB ) exprMul )*
bool exprAdd(){
    if(!exprMul()) return false;
    while(tokens[iTk].code==ADD || tokens[iTk].code==SUB){
        int op = tokens[iTk].code;
        consume(op);
        if(!exprMul()){
            if(op==ADD) tkerr("expected term after '%s'", "+");
            else tkerr("expected term after '%s'", "-");
        }
    }
    return true;
}

// exprMul ::= exprPrefix ( ( MUL | DIV ) exprPrefix )*
bool exprMul(){
    if(!exprPrefix()) return false;
    while(tokens[iTk].code==MUL || tokens[iTk].code==DIV){
        int op = tokens[iTk].code;
        consume(op);
        if(!exprPrefix()){
            if(op==MUL) tkerr("expected factor after '%s'", "*");
            else tkerr("expected factor after '%s'", "/");
        }

    }
    return true;
}

// exprPrefix ::= ( SUB | NOT )? factor
bool exprPrefix(){
    if(tokens[iTk].code==SUB || tokens[iTk].code==NOT){
        consume(tokens[iTk].code);
    }
    return factor();
}

// factor ::= INT
// | REAL
// | STR
// | LPAR expr RPAR
// | ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
bool factor(){
    int c = tokens[iTk].code;
    if(c==INT){ consume(INT); return true; }
    if(c==REAL){ consume(REAL); return true; }
    if(c==STR){ consume(STR); return true; }
    if(c==LPAR){
        consume(LPAR);
        if(!expr()) tkerr("expected expression after '('");
        if(!consume(RPAR)) tkerr("expected ')' after expression");
        return true;
    }
    if(c==ID){
        consume(ID);
        if(tokens[iTk].code==LPAR){
            consume(LPAR);
            if(tokens[iTk].code!=RPAR){
                if(!expr()) tkerr("expected expression in function call");
                while(consume(COMMA)){
                    if(!expr()) tkerr("expected expression after ',' in call");
                }
            }
            if(!consume(RPAR)) tkerr("expected ')' after function call arguments");
        }
        return true;
    }
    return false;
}


// same as err, but also prints the line of the current token
_Noreturn void tkerr(const char *fmt,...){
	fprintf(stderr,"[ERROR] in line %d: ",tokens[iTk].line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

bool consume(int code){
	if(tokens[iTk].code==code){
		consumed=&tokens[iTk++];
		return true;
		}
	return false;
	}

// program ::= ( defVar | defFunc | block )* FINISH
bool program(){
	for(;;){
		if(defVar()){}
		else if(defFunc()){}
		else if(block()){}
		else break;
		}
	if(consume(FINISH)){
		return true;
		}else tkerr("syntax error");
	return false;
	}

void parse(){
	iTk=0;
    addDomain(); // create the global domain
	program();
    delDomain(); // delete the global domain
	}
