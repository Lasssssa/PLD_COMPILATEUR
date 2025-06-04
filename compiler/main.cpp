#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"

#include "SymbolTableVisitor.h"
#include "Visitors.h"

using namespace antlr4;
using namespace std;

int main(int argn, const char **argv)
{
    stringstream in;
    if (argn == 2)
    {
        ifstream lecture(argv[1]);
        if (!lecture.good())
        {
            cerr << "error: cannot read file: " << argv[1] << endl;
            exit(1);
        }
        in << lecture.rdbuf();
    }
    else
    {
        cerr << "usage: ifcc path/to/file.c" << endl;
        exit(1);
    }

    ANTLRInputStream input(in.str());

    ifccLexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    tokens.fill();

    ifccParser parser(&tokens);
    tree::ParseTree *tree = parser.axiom();

    if (parser.getNumberOfSyntaxErrors() != 0)
    {
        cerr << "error: syntax error during parsing" << endl;
        exit(1);
    }

    // PHASE 1: Analyse sémantique et construction de la table des symboles
    SymbolTableVisitor symbolTableVisitor;
    symbolTableVisitor.visit(tree);

    // Vérifier s'il y a eu des erreurs sémantiques
    if (symbolTableVisitor.hasSemanticErrors())
    {
        cerr << "error: semantic errors found during analysis" << endl;
        exit(1);
    }

    // PHASE 2: Génération de code avec la table des symboles
    Visitor Visitor(symbolTableVisitor.getSymbolTable());
    Visitor.visit(tree);

    return 0;
}
