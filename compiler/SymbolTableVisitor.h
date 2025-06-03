#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include <map>
#include <set>
#include <string>
#include <iostream>

class SymbolTableVisitor : public ifccBaseVisitor
{
private:
    std::map<std::string, int> symbolTable; // nom de variable -> offset depuis %rbp
    std::set<std::string> declaredVars;     // variables déclarées
    std::set<std::string> usedVars;         // variables utilisées
    int currentOffset;                      // offset actuel (commence à -4, décrémente par 4)
    bool hasErrors;                         // flag pour indiquer des erreurs

public:
    SymbolTableVisitor();

    // Getter pour la table des symboles
    const std::map<std::string, int> &getSymbolTable() const { return symbolTable; }
    bool hasSemanticErrors() const { return hasErrors; }

    // Méthodes de vérification finale
    void checkUnusedVariables();

    // Visiteurs ANTLR
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
    virtual antlrcpp::Any visitAssignExpr(ifccParser::AssignExprContext *ctx) override;
};