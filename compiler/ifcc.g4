grammar ifcc;

axiom : prog EOF ;

prog : (function | global_decl)* EOF ;

function : ('int'|'void') VAR '(' param_list? ')' block_stmt ;

// Ajout de la règle pour les déclarations globales
global_decl : 'int' VAR ';' | 'int' VAR '=' expr ';' ;

stmt: return_stmt
    | decl_stmt
    | expr_stmt
    | if_stmt
    | block_stmt
    ;

if_stmt: IF '(' expr ')' stmt (ELSE stmt)? ;

block_stmt: '{' stmt* '}' ;

return_stmt: RETURN expr? ';' ;
expr_stmt: expr ';' ;
decl_stmt: 'int' VAR ';' | 'int' VAR '=' expr ';' ;

// Expression avec priorités (du plus bas au plus haut)
expr : expr ASSIGN expr                             # assignExpr
     | (PLUS | MINUS | NOT) expr                    # unaryExpr
     | expr (MULT | DIV | MOD) expr                 # multiplicativeExpr
     | expr (PLUS | MINUS) expr                     # additiveExpr
     | expr (LT | GT | LE | GE) expr                # relationalExpr
     | expr (EQ | NEQ) expr                         # equalityExpr
     | expr (BITAND) expr                           # bitwiseAndExpr
     | expr (BITXOR) expr                           # bitwiseXorExpr
     | expr (BITOR) expr                            # bitwiseOrExpr
     | expr OR expr                                 # logicalOrExpr
     | expr AND expr                                # logicalAndExpr
     | VAR '(' arg_list? ')'                        # callExpr
     | VAR                                          # varExpr
     | CONST                                        # constExpr
     | CHAR_LITERAL                                 # charExpr
     | '(' expr ')'                                 # parensExpr
     ;

// Liste de paramètres
param_list : 'int' VAR (',' 'int' VAR)* ;

// Liste d'arguments
arg_list : expr (',' expr)* ;

IF : 'if' ;
ELSE : 'else' ;
RETURN : 'return' ;
VAR : [a-zA-Z_][a-zA-Z0-9_]* ;
CONST : [0-9]+ ;
CHAR_LITERAL : '\'' . '\'' ;  // Nouveau token pour les caractères

ASSIGN : '=' ;
PLUS : '+' ;
MINUS : '-' ;
MULT : '*' ;
DIV : '/' ;
MOD : '%' ;

EQ : '==' ;
NEQ : '!=' ;
LT : '<' ;
GT : '>' ;
LE : '<=' ;
GE : '>=' ;

NOT : '!' ;
AND : '&&' ;  // Nouveau token pour l'opérateur logique AND
OR : '||' ;   // Nouveau token pour l'opérateur logique OR

BITAND : '&' ;
BITXOR : '^' ;
BITOR : '|' ;

COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS : [ \t\r\n]+ -> skip ;
