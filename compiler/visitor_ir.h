// VISITOR_IR_H : Visiteur pour la génération de l'IR à partir de l'AST généré par ANTLR
#ifndef VISITOR_IR_H
#define VISITOR_IR_H

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "IR.h"
#include "type.h"
#include <map>
#include <string>
#include <vector>
#include <set>

// Structure pour représenter un paramètre de fonction
// Permet de stocker le nom et le type de chaque paramètre
struct Param
{
    std::string name;
    Type type;

    Param(const std::string &n, Type t) : name(n), type(t) {}
};

// Visiteur ANTLR pour générer l'IR à partir de l'AST
// Ce visiteur fait le lien entre le front-end (AST) et le middle-end (IR/CFG)
class VisitorIR : public ifccBaseVisitor
{
private:
    // Table des CFGs pour chaque fonction (clé = nom de fonction)
    map<string, CFG *> cfgs;
    // CFG courant (fonction en cours de visite)
    CFG *current_cfg;
    // BasicBlock courant (bloc de code en cours de génération)
    BasicBlock *current_bb;
    // Numéro pour générer des noms uniques de BasicBlock
    int nextBBnumber;
    // Nom de la fonction courante
    string currentFunctionName;

    // Méthodes utilitaires internes
    std::string createTempVar(Type t); // Crée une variable temporaire dans l'IR
    void addInstr(IRInstr::Operation op, Type t, const std::vector<std::string> &params); // Ajoute une instruction IR
    BasicBlock *createNewBB(); // Crée un nouveau BasicBlock
    void setCurrentBB(BasicBlock *bb); // Change le BasicBlock courant

public:
    // Constructeur par défaut
    VisitorIR() : current_cfg(nullptr), current_bb(nullptr), nextBBnumber(0) {}

    // Constructeur avec paramètre (pour initialiser la table des symboles si besoin)
    VisitorIR(const map<string, int> &symbols)
        : current_cfg(nullptr), current_bb(nullptr), nextBBnumber(0) {}

    ~VisitorIR(); // Libère la mémoire des CFGs

    // Récupère le CFG d'une fonction (pour la génération de code)
    CFG *getCFG(const std::string &functionName) const
    {
        auto it = cfgs.find(functionName);
        return (it != cfgs.end()) ? it->second : nullptr;
    }

    // Surcharge des méthodes de visite pour chaque type de nœud de l'AST
    // Ces méthodes traduisent l'AST en instructions IR (middle-end)
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override; // Programme complet
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override; // return
    virtual antlrcpp::Any visitExpr_stmt(ifccParser::Expr_stmtContext *ctx) override; // expression seule
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override; // déclaration de variable
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override; // variable
    virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override; // constante
    virtual antlrcpp::Any visitCharExpr(ifccParser::CharExprContext *ctx) override; // caractère
    virtual antlrcpp::Any visitAssignExpr(ifccParser::AssignExprContext *ctx) override; // assignation
    virtual antlrcpp::Any visitLogicalAndExpr(ifccParser::LogicalAndExprContext *ctx) override; // &&
    virtual antlrcpp::Any visitLogicalOrExpr(ifccParser::LogicalOrExprContext *ctx) override; // ||
    virtual antlrcpp::Any visitAdditiveExpr(ifccParser::AdditiveExprContext *ctx) override; // +, -
    virtual antlrcpp::Any visitMultiplicativeExpr(ifccParser::MultiplicativeExprContext *ctx) override; // *, /, %
    virtual antlrcpp::Any visitUnaryExpr(ifccParser::UnaryExprContext *ctx) override; // -, +, !
    virtual antlrcpp::Any visitParensExpr(ifccParser::ParensExprContext *ctx) override; // parenthèses
    virtual antlrcpp::Any visitFunction(ifccParser::FunctionContext *ctx) override; // fonction
    virtual antlrcpp::Any visitCallExpr(ifccParser::CallExprContext *ctx) override; // appel de fonction
    virtual antlrcpp::Any visitParam_list(ifccParser::Param_listContext *ctx) override; // liste de paramètres
    virtual antlrcpp::Any visitArg_list(ifccParser::Arg_listContext *ctx) override; // liste d'arguments
    virtual antlrcpp::Any visitEqualityExpr(ifccParser::EqualityExprContext *ctx) override; // ==, !=
    virtual antlrcpp::Any visitRelationalExpr(ifccParser::RelationalExprContext *ctx) override; // <, >, <=, >=
    virtual antlrcpp::Any visitBitwiseAndExpr(ifccParser::BitwiseAndExprContext *ctx) override; // &
    virtual antlrcpp::Any visitBitwiseXorExpr(ifccParser::BitwiseXorExprContext *ctx) override; // ^
    virtual antlrcpp::Any visitBitwiseOrExpr(ifccParser::BitwiseOrExprContext *ctx) override; // |
    virtual antlrcpp::Any visitIf_stmt(ifccParser::If_stmtContext *ctx) override; // if/else
    virtual antlrcpp::Any visitBlock_stmt(ifccParser::Block_stmtContext *ctx) override; // bloc d'instructions
};

#endif