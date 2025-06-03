grammar ifcc;

axiom : prog EOF ;

prog : 'int' 'main' '(' ')' '{' stmt* '}' ;

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
     | VAR                                        # varExpr
     | CONST                                      # constExpr
     | '(' expr ')'                               # parensExpr
     ;

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
