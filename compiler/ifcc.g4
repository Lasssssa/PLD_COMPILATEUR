grammar ifcc;

axiom : prog EOF ;

prog : function* EOF ;

function : 'int' VAR '(' param_list? ')' '{' stmt* '}' ;

stmt: return_stmt
    | decl_stmt
    | expr_stmt
    ;

return_stmt: RETURN expr ';' ;
expr_stmt: expr ';' ;
decl_stmt: 'int' VAR ';' | 'int' VAR '=' expr ';' ;

// Expression avec priorités (du plus bas au plus haut)
expr : expr ASSIGN <assoc=right> expr               # assignExpr
     | (PLUS | MINUS) expr                        # unaryExpr
     | expr (MULT | DIV) expr                     # multiplicativeExpr
     | expr (PLUS | MINUS) expr                   # additiveExpr
     | VAR '(' arg_list? ')'                      # callExpr
     | VAR                                        # varExpr
     | CONST                                      # constExpr
     | '(' expr ')'                               # parensExpr
     ;

// Liste de paramètres
param_list : 'int' VAR (',' 'int' VAR)* ;

// Liste d'arguments
arg_list : expr (',' expr)* ;

RETURN : 'return' ;
VAR : [a-zA-Z_][a-zA-Z0-9_]* ;
CONST : [0-9]+ ;

ASSIGN : '=' ;
PLUS : '+' ;
MINUS : '-' ;
MULT : '*' ;
DIV : '/' ;

COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS : [ \t\r\n]+ -> skip ;
