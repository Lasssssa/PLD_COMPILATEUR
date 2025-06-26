// ifcc.g4 : Grammaire ANTLR pour le front-end du compilateur
// Cette grammaire définit la syntaxe du langage supporté par le compilateur
// Les règles sont organisées par priorité et par type de construction (fonctions, expressions, etc.)

grammar ifcc;

// Point d'entrée du parseur
axiom : prog EOF ;

// Un programme est une suite de fonctions ou de déclarations globales
prog : (function | global_decl)* EOF ;

// Définition d'une fonction (int ou void)
function : ('int'|'void') VAR '(' param_list? ')' block_stmt ;

// Déclaration globale (support limité, pour extensions futures)
// Permet de déclarer des variables globales
// Ex : int x; ou int x = 42;
global_decl : 'int' VAR ';' | 'int' VAR '=' expr ';' ;

// Une instruction (statement) peut être un return, une déclaration, une expression, un if, ou un bloc
stmt: return_stmt
    | decl_stmt
    | expr_stmt
    | if_stmt
    | block_stmt
    ;

// Structure d'un if/else
if_stmt: IF '(' expr ')' stmt (ELSE stmt)? ;

// Bloc d'instructions (accolades)
block_stmt: '{' stmt* '}' ;

// Instruction return (avec ou sans expression)
return_stmt: RETURN expr? ';' ;
// Instruction expression (ex : appel de fonction, calcul)
expr_stmt: expr ';' ;
// Déclaration de variable locale (avec ou sans initialisation)
decl_stmt: 'int' VAR ';' | 'int' VAR '=' expr ';' ;

// Expressions avec priorités (du plus bas au plus haut)
// Chaque règle correspond à un type d'expression (affectation, opérateurs, etc.)
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

// Liste de paramètres de fonction (ex : int a, int b)
param_list : 'int' VAR (',' 'int' VAR)* ;

// Liste d'arguments d'appel de fonction (ex : f(a, b+1))
arg_list : expr (',' expr)* ;

// Mots-clés et tokens
IF : 'if' ;
ELSE : 'else' ;
RETURN : 'return' ;
VAR : [a-zA-Z_][a-zA-Z0-9_]* ;
CONST : [0-9]+ ;
CHAR_LITERAL : '\'' . '\'' ;  // Token pour les caractères

// Opérateurs
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
AND : '&&' ;  // Opérateur logique AND
OR : '||' ;   // Opérateur logique OR

BITAND : '&' ;
BITXOR : '^' ;
BITOR : '|' ;

// Commentaires et espaces ignorés
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS : [ \t\r\n]+ -> skip ;
