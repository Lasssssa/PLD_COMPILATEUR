// SymbolTableVisitor.h : Visiteur pour la construction et la vérification de la table des symboles
#ifndef SYMBOL_TABLE_VISITOR_H
#define SYMBOL_TABLE_VISITOR_H

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include <map>
#include <set>
#include <string>
#include <iostream>

// Visiteur ANTLR pour la gestion de la table des symboles et des analyses statiques
// Ce composant fait le lien entre le front-end (AST) et le middle-end (analyses sémantiques)
class SymbolTableVisitor : public ifccBaseVisitor
{
private:
    // Table des symboles : nom de variable -> offset depuis %rbp
    std::map<std::string, int> symbolTable;
    // Variables locales déclarées dans la fonction courante
    std::set<std::string> declaredVars;
    // Variables globales déclarées
    std::set<std::string> globalVars;
    // Variables utilisées (pour détecter les variables non utilisées)
    std::set<std::string> usedVars;
    // Fonctions déclarées
    std::set<std::string> declaredFunctions;
    // Fonctions qui possèdent un return
    std::set<std::string> functionsWithReturn;
    // Nom de la fonction courante
    std::string currentFunction;
    // Offset courant pour l'allocation des variables locales
    int currentOffset;
    // Indique s'il y a des erreurs sémantiques
    bool hasErrors;
    // Nombre de paramètres pour chaque fonction
    std::map<std::string, int> functionParamCount;

public:
    SymbolTableVisitor();

    // Accès à la table des symboles
    const std::map<std::string, int> &getSymbolTable() const { return symbolTable; }
    bool hasSemanticErrors() const { return hasErrors; }

    // Vérification des variables non utilisées et de la présence de main
    void checkUnusedVariables();
    void checkMainFunction(); // Vérifie la présence de main

    // Visiteurs ANTLR pour chaque type de nœud de l'AST
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
    
    // Déclarations manquantes pour la couverture complète de la grammaire
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

    // Méthode utilitaire pour vérifier la présence de return dans les fonctions
    void checkReturnStatements();
};

#endif