#ifndef VISITOR_IR_H
#define VISITOR_IR_H

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "IR.h"
#include "type.h"
#include <map>
#include <string>
#include <vector>
#include <stack>

// Structure pour représenter un paramètre de fonction
struct Param
{
    std::string name;
    Type type;

    Param(const std::string &n, Type t) : name(n), type(t) {}
};

class VisitorIR : public ifccBaseVisitor
{
private:
    map<string, CFG *> cfgs;
    CFG *current_cfg;
    BasicBlock *current_bb;
    std::stack<BasicBlock*> bb_stack;
    int nextFreeSymbolIndex;
    map<string, int> symbolTable;
    int nextBBnumber;
    string currentFunctionName;

    // Méthodes utilitaires
    std::string createTempVar(Type t);
    void addInstr(IRInstr::Operation op, Type t, const std::vector<std::string> &params);
    BasicBlock *createNewBB();
    void setCurrentBB(BasicBlock *bb);

public:
    // Constructeur par défaut
    VisitorIR() : current_cfg(nullptr), current_bb(nullptr), nextFreeSymbolIndex(0), nextBBnumber(0) {}

    // Constructeur avec paramètre
    VisitorIR(const map<string, int> &symbols)
        : current_cfg(nullptr), current_bb(nullptr), nextFreeSymbolIndex(0), nextBBnumber(0), symbolTable(symbols) {}

    ~VisitorIR();

    // Méthode pour récupérer le CFG d'une fonction
    CFG *getCFG(const std::string &functionName) const
    {
        auto it = cfgs.find(functionName);
        return (it != cfgs.end()) ? it->second : nullptr;
    }

    // Visiteurs pour les différents nœuds de l'AST
    virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
    virtual antlrcpp::Any visitReturn_stmt(ifccParser::Return_stmtContext *ctx) override;
    virtual antlrcpp::Any visitExpr_stmt(ifccParser::Expr_stmtContext *ctx) override;
    virtual antlrcpp::Any visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) override;
    virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
    virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
    virtual antlrcpp::Any visitAssignExpr(ifccParser::AssignExprContext *ctx) override;
    virtual antlrcpp::Any visitAdditiveExpr(ifccParser::AdditiveExprContext *ctx) override;
    virtual antlrcpp::Any visitMultiplicativeExpr(ifccParser::MultiplicativeExprContext *ctx) override;
    virtual antlrcpp::Any visitUnaryExpr(ifccParser::UnaryExprContext *ctx) override;
    virtual antlrcpp::Any visitParensExpr(ifccParser::ParensExprContext *ctx) override;
    virtual antlrcpp::Any visitFunction(ifccParser::FunctionContext *ctx) override;
    virtual antlrcpp::Any visitCallExpr(ifccParser::CallExprContext *ctx) override;
    virtual antlrcpp::Any visitParam_list(ifccParser::Param_listContext *ctx) override;
    virtual antlrcpp::Any visitArg_list(ifccParser::Arg_listContext *ctx) override;
    virtual antlrcpp::Any visitComparisonExpr(ifccParser::ComparisonExprContext *ctx) override;
};

#endif