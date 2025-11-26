#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "lexer.h"
#include "AnalysisDomain.h"
#include "AnalysisTypes.h"
#include "gen.h"

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
    /* code generation: emit C variable declaration */
    Text_write(crtVar, "%s %s;\n", cType(ret.type), name);
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
    
    // Code Generation
    crtCode=&tFunctions;
    crtVar=&tFunctions;
    Text_clear(&tFnHeader);
    Text_write(&tFnHeader,"%s(",name);

    if(!consume(LPAR)) tkerr("expected '(' after function name");
    if(tokens[iTk].code != RPAR){
        if(!funcParams()) tkerr("invalid function parameters");
    }
    if(!consume(RPAR)) tkerr("expected ')' after function parameters");
    if(!consume(COLON)) tkerr("expected ':' after function header");
    if(!baseType()) tkerr("expected return type after ':' in function header");
    /* semantic action: set function return type */

    // Code Generation
    Text_write(&tFunctions,"\n%s %s){\n",cType(ret.type),tFnHeader.buf);

    crtFn->type = ret.type;
    while(defVar()){}
    if(!block()) tkerr("expected function body (block)");
    if(!consume(END)) tkerr("expected 'end' after function body");
    // Code Generation
    Text_write(&tFunctions,"}\n");
    crtCode=&tMain;
    crtVar=&tBegin;
    
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
        Text_write(&tFnHeader,",");
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

    // Code Generation
    Text_write(&tFnHeader,"%s %s",cType(ret.type), argName);

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
        // Code Generation
        Text_write(crtCode, "if(");

        if(!expr()) tkerr("expected expression in if condition");
        /* semantic action: check that condition type is not STR */
        if(ret.type==TYPE_STR) tkerr("the if condition must have type int or real");
        if(!consume(RPAR)) tkerr("expected ')' after if condition");
        // Code Generation
        Text_write(crtCode, "){\n");

        if(!block()) tkerr("expected block after if condition");
        // Code Generation
        Text_write(crtCode, "}\n");
        if(consume(ELSE)){
            // Code Generation
            Text_write(crtCode, "else{\n");
            if(!block()) tkerr("expected block after else");
            // Code Generation
            Text_write(crtCode, "}\n");
        }
        if(!consume(END)) tkerr("expected 'end' after if");
        return true;
    }
    if(c==RETURN){
        Text_write(crtCode, "return ");
        consume(RETURN);
        if(!expr()) tkerr("expected expression after 'return'");
        /* semantic action: check return statement is in a function and types match */
        if(!crtFn) tkerr("return can be used only in a function");
        if(ret.type!=crtFn->type) tkerr("the return type must be the same as the function return type");
        if(!consume(SEMICOLON)) tkerr("expected ';' after return expression");

        Text_write(crtCode, ";\n");
        return true;
    }
    if(c==WHILE){
        consume(WHILE);
        Text_write(crtCode, "while(");
        if(!consume(LPAR)) tkerr("expected '(' after 'while'");
        if(!expr()) tkerr("expected expression in while condition");
        /* semantic action: check that condition type is not STR */
        if(ret.type==TYPE_STR) tkerr("the while condition must have type int or real");
        if(!consume(RPAR)) tkerr("expected ')' after while condition");

        Text_write(crtCode, "){\n");

        if(!block()) tkerr("expected block after while condition");
        if(!consume(END)) tkerr("expected 'end' after while");

        Text_write(crtCode, "}\n");
        return true;
    }
    /* otherwise expr? SEMICOLON */
    if(tokens[iTk].code==SEMICOLON){
        consume(SEMICOLON);
        return true;
    }
    if(expr()){
        if(!consume(SEMICOLON)) tkerr("expected ';' after expression");
        Text_write(crtCode, ";\n");
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
        /* semantic action: check that operands have type int or real */
        Ret leftType = ret;
        if(leftType.type==TYPE_STR) tkerr("the left operand of '&&' or '||' cannot be of type string");

        if(op == AND) Text_write(crtCode, "&&");
        if(op == OR) Text_write(crtCode, "||");

        if(!exprAssign()){
            if(op==AND) tkerr("expected expression after '%s'", "&&");
            else tkerr("expected expression after '%s'", "||");
        }

        if(ret.type==TYPE_STR) tkerr("the right operand of '&&' or '||' cannot be of type string");
        setRet(TYPE_INT,false); /* logical expressions have int type */
    }
    return true;
}

// exprAssign ::= ( ID ASSIGN )? exprComp
bool exprAssign(){
    if(tokens[iTk].code==ID && tokens[iTk+1].code==ASSIGN){
        consume(ID);
        /* semantic action: check that ID is a variable and is lval */
        const char *name = consumed->text;

        consume(ASSIGN);

        Text_write(crtCode, "%s=", name);
        if(!exprComp()) tkerr("expected expression after '='");

        Symbol *s = searchSymbol(name);
        if(!s) tkerr("undefined symbol '%s'", name);
        if(s->kind == KIND_FN) tkerr("a function (%s) cannot be used as a destination for assignment ", name);
        if(s->type!=ret.type) tkerr("the source and destination for assignment must have same type");
        ret.lval = false;

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
        /* semantic action: */
        Ret leftType = ret;

        if(op == LESS) Text_write(crtCode, "<");
        if(op == EQUAL) Text_write(crtCode, "==");

        if(!exprAdd()){
            if(op==LESS) tkerr("expected expression after '%s'", "<");
            else tkerr("expected expression after '%s'", "==");
        }

        if(leftType.type!=ret.type) tkerr("different types for the operands of '<' or '=='");
        setRet(TYPE_INT, false);
    }
    return true;
}

// exprAdd ::= exprMul ( ( ADD | SUB ) exprMul )*
bool exprAdd(){
    if(!exprMul()) return false;
    while(tokens[iTk].code==ADD || tokens[iTk].code==SUB){
        int op = tokens[iTk].code;
        consume(op);

        if(op == ADD) Text_write(crtCode, "+");
        if(op == SUB) Text_write(crtCode, "-");

        /* semantic action: */
        Ret leftType = ret;
        if(leftType.type == TYPE_STR) tkerr("the operands of '+' or '-' cannot be of type str");

        if(!exprMul()){
            if(op==ADD) tkerr("expected term after '%s'", "+");
            else tkerr("expected term after '%s'", "-");
        }

        /* semantic action: */
        if(leftType.type != ret.type) tkerr("different types for the operands of '+' or '-'");

    }
    return true;
}

// exprMul ::= exprPrefix ( ( MUL | DIV ) exprPrefix )*
bool exprMul(){
    if(!exprPrefix()) return false;
    while(tokens[iTk].code==MUL || tokens[iTk].code==DIV){
        int op = tokens[iTk].code;
        consume(op);

        if(op == MUL) Text_write(crtCode, "*");
        if(op == DIV) Text_write(crtCode, "/");
        /* semantic action: */
        Ret leftType=ret;
        if(leftType.type==TYPE_STR)tkerr("the operands of * or / cannot be of type str");

        if(!exprPrefix()){
            if(op==MUL) tkerr("expected factor after '%s'", "*");
            else tkerr("expected factor after '%s'", "/");
        }

        if(leftType.type!=ret.type)tkerr("different types for the operands of * or /");
        ret.lval=false;
    }
    return true;
}

// exprPrefix ::= ( SUB | NOT )? factor
bool exprPrefix(){

    int code = tokens[iTk].code;
    int retFromFactor;

    switch (code){
        case SUB:
            consume(tokens[iTk].code);
            Text_write(crtCode, "-");

            retFromFactor = factor();
            /* semantic analysis: */
            if(ret.type==TYPE_STR)tkerr("the expression of unary - must be of type int or real");
            ret.lval=false;
            break;
        case NOT:
            consume(tokens[iTk].code);
            Text_write(crtCode, "!");
            retFromFactor = factor();

            /* semantic analysis: */
            if(ret.type==TYPE_STR)tkerr("the expression of ! must be of type int or real");
            setRet(TYPE_INT,false);
            break;
        default:
            retFromFactor = factor();
    }
    return retFromFactor;
}

// factor ::= INT
// | REAL
// | STR
// | LPAR expr RPAR
// | ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
bool factor(){
    int c = tokens[iTk].code;
    if(c==INT){
        consume(INT); 
        Text_write(crtCode, "%d", consumed->i);
        setRet(TYPE_INT, false);
        return true; 
    }
    if(c==REAL){
        consume(REAL);
        Text_write(crtCode, "%g", consumed->r);
        setRet(TYPE_REAL, false);
        return true;
    }
    if(c==STR){
        consume(STR);
        Text_write(crtCode, "\"%s\"", consumed->text);
        setRet(TYPE_STR, false);
        return true;
    }
    if(c==LPAR){
        consume(LPAR);
        Text_write(crtCode, "(");
        if(!expr()) tkerr("expected expression after '('");
        if(!consume(RPAR)) tkerr("expected ')' after expression");
        Text_write(crtCode, ")");
        return true;
    }
    if(c==ID){
        consume(ID);

        Symbol * s= searchSymbol(consumed->text);
        if(!s) tkerr("undefined symbol: %s", consumed->text);
        Text_write(crtCode, "%s", s->name);

        if(tokens[iTk].code==LPAR){
            consume(LPAR);

            Text_write(crtCode, "(");

            if(s->kind!= KIND_FN) tkerr("%s cannot be called, because it is not a function", s->name);
            Symbol *argDef = s->args;

            if(tokens[iTk].code!=RPAR){
                if(!expr()) tkerr("expected expression in function call");

                if(!argDef) tkerr("the function %s is called with too many arguments", s->name);
                if(argDef->type != ret.type) tkerr("the argument type at function %s call is different from the one given at its definition", s->name);
                argDef=argDef->next;

                while(consume(COMMA)){
                    Text_write(crtCode, ",");
                    if(!expr()) tkerr("expected expression after ',' in call");
                    if(!argDef)tkerr("the function %s is called with too many arguments",s->name);
                    if(argDef->type!=ret.type)tkerr("the argument type at function %s call is different from the one given at its definition",s->name);
                    argDef=argDef->next;
                }
            }
            if(!consume(RPAR)) tkerr("expected ')' after function call arguments");
            Text_write(crtCode, ")");
            if(argDef)tkerr("the function %s is called with too few arguments",s->name);
            setRet(s->type,false);
            return true;
        } else {
            /* not a call: must be a variable (lvalue), not a function */
            if(s->kind==KIND_FN) tkerr("the function %s can only be called", s->name);
            setRet(s->type,true);
            return true;
        }
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
    addDomain(); // create the global domain
    addPredefinedFns();

    /* code generation initialization */
    crtCode = &tMain;
    crtVar = &tBegin;
    Text_write(&tBegin, "#include \"quick.h\"\n\n");
    Text_write(&tMain, "\nint main(){\n");

    for(;;){
        if(defVar()){}
        else if(defFunc()){}
        else if(block()){}
        else break;
    }

    if(consume(FINISH)){
        /* finalize generated code and write to file */
        Text_write(&tMain, "return 0;\n}\n");
        FILE *fis = fopen("1.c","w");
        if(!fis){
            printf("cannot write to file 1.c\n");
            exit(EXIT_FAILURE);
        }
        fwrite(tBegin.buf, sizeof(char), tBegin.n, fis);
        fwrite(tFunctions.buf, sizeof(char), tFunctions.n, fis);
        fwrite(tMain.buf, sizeof(char), tMain.n, fis);
        fclose(fis);

        /* cleanup domains */
        delDomain();
        return true;
    } else tkerr("syntax error");
    return false;
	}

void parse(){
	iTk=0;
	program();
	}
