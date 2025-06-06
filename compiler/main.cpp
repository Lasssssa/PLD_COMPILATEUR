#include <iostream>
#include <fstream>
#include <string>
#include "visitor_ir.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "antlr4-runtime.h"

using namespace antlr4;
using namespace std;

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    // Lecture du fichier d'entrée
    ifstream file(argv[1]);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << argv[1] << endl;
        return 1;
    }

    // Création du lexer et du parser
    ANTLRInputStream input(file);
    ifccLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    ifccParser parser(&tokens);

    // Création d'une table des symboles vide
    map<string, int> emptySymbolTable;
    
    // Création du visiteur avec la table des symboles vide
    VisitorIR visitor(emptySymbolTable);

    // Visite de l'AST - cela va générer le code assembleur
    visitor.visitProg(parser.prog());

    return 0;
}
