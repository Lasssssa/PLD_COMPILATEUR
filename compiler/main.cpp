#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "visitor_ir.h"
#include "SymbolTableVisitor.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "antlr4-runtime.h"

using namespace antlr4;

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }

    // Lecture du fichier d'entrée
    std::stringstream in;
    std::ifstream file(argv[1]);
    if (!file) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }
    in << file.rdbuf();

    // Création du lexer et du parser
    ANTLRInputStream input(in.str());
    ifccLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    tokens.fill();  // Important : remplir le buffer de tokens

    ifccParser parser(&tokens);
    tree::ParseTree* tree = parser.axiom();  // Utiliser axiom() au lieu de prog()

    // Vérifier les erreurs de syntaxe
    if (parser.getNumberOfSyntaxErrors() != 0) {
        std::cerr << "Error: syntax error during parsing" << std::endl;
        return 1;
    }

    // PHASE 1: Analyse sémantique et construction de la table des symboles
    SymbolTableVisitor symbolTableVisitor;
    symbolTableVisitor.visit(tree);

    // Vérifier s'il y a eu des erreurs sémantiques
    if (symbolTableVisitor.hasSemanticErrors()) {
        std::cerr << "Error: semantic errors found during analysis" << std::endl;
        return 1;
    }

    // PHASE 2: Génération de code avec la table des symboles
    VisitorIR visitor(symbolTableVisitor.getSymbolTable());
    visitor.visit(tree);

    return 0;
}