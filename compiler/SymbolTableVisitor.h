#ifndef SYMBOL_TABLE_VISITOR_H
#define SYMBOL_TABLE_VISITOR_H

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
    std::set<std::string> declaredVars;     // variables locales déclarées
    std::set<std::string> globalVars;       // variables globales déclarées
    std::set<std::string> usedVars;         // variables utilisées
    std::set<std::string> declaredFunctions; // fonctions déclarées
    std::set<std::string> functionsWithReturn; // fonctions qui ont un return
    std::string currentFunction;            // nom de la fonction courante
    int currentOffset;                      // offset actuel (commence à -4, décrémente par 4)
    bool hasErrors;                         // flag pour indiquer des erreurs

public:
    SymbolTableVisitor();

    // Getter pour la table des symboles
    const std::map<std::string, int> &getSymbolTable() const { return symbolTable; }
    bool hasSemanticErrors() const { return hasErrors; }

    // Méthodes de vérification finale
    void checkUnusedVariables();
    void checkMainFunction(); // Nouvelle méthode pour vérifier la présence de main

    // Visiteurs ANTLR
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitGlobal_decl(ifccParser::Global_declContext *ctx) override;
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
    virtual antlrcpp::Any visitAssignExpr(ifccParser::AssignExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicalAndExpr(ifccParser::LogicalAndExprContext *ctx) override;
    virtual antlrcpp::Any visitLogicalOrExpr(ifccParser::LogicalOrExprContext *ctx) override;
    virtual antlrcpp::Any visitFunction(ifccParser::FunctionContext *ctx) override;
    virtual antlrcpp::Any visitParam_list(ifccParser::Param_listContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
    
    // Ajout des déclarations manquantes
    virtual antlrcpp::Any visitCallExpr(ifccParser::CallExprContext *ctx) override;
    virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
    virtual antlrcpp::Any visitCharExpr(ifccParser::CharExprContext *ctx) override;
    virtual antlrcpp::Any visitAdditiveExpr(ifccParser::AdditiveExprContext *ctx) override;
    virtual antlrcpp::Any visitMultiplicativeExpr(ifccParser::MultiplicativeExprContext *ctx) override;
    virtual antlrcpp::Any visitUnaryExpr(ifccParser::UnaryExprContext *ctx) override;
    virtual antlrcpp::Any visitParensExpr(ifccParser::ParensExprContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseAndExpr(ifccParser::BitwiseAndExprContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseXorExpr(ifccParser::BitwiseXorExprContext *ctx) override;
    virtual antlrcpp::Any visitBitwiseOrExpr(ifccParser::BitwiseOrExprContext *ctx) override;
    virtual antlrcpp::Any visitExpr_stmt(ifccParser::Expr_stmtContext *ctx) override;
    virtual antlrcpp::Any visitArg_list(ifccParser::Arg_listContext *ctx) override;
    virtual antlrcpp::Any visitEqualityExpr(ifccParser::EqualityExprContext *ctx) override;
    virtual antlrcpp::Any visitRelationalExpr(ifccParser::RelationalExprContext *ctx) override;
    virtual antlrcpp::Any visitIf_stmt(ifccParser::If_stmtContext *ctx) override;
    virtual antlrcpp::Any visitBlock_stmt(ifccParser::Block_stmtContext *ctx) override;

    // Méthodes utilitaires
    void checkReturnStatements();
};

#endif