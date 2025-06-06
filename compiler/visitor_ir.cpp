#include "visitor_ir.h"
#include "DefFonction.h"
#include <sstream>
#include <iostream>

using std::to_string;
using namespace std;

VisitorIR::~VisitorIR() {
    for (auto& pair : cfgs) {
        delete pair.second;
    }
}

string VisitorIR::createTempVar(Type t) {
    return current_cfg->create_new_tempvar(t);
}

void VisitorIR::addInstr(IRInstr::Operation op, Type t, const vector<string>& params) {
    current_bb->add_IRInstr(op, t, params);
}

BasicBlock* VisitorIR::createNewBB() {
    string bbName = "BB_" + to_string(nextBBnumber++);
    BasicBlock* bb = new BasicBlock(current_cfg, bbName);
    current_cfg->add_bb(bb);
    return bb;
}

void VisitorIR::setCurrentBB(BasicBlock* bb) {
    current_bb = bb;
    current_cfg->current_bb = bb;
}

antlrcpp::Any VisitorIR::visitProg(ifccParser::ProgContext *ctx) {
    // Créer la fonction main
    DefFonction* main = new DefFonction("main", Type::INT_TYPE, {});
    current_cfg = new CFG(main);
    cfgs["main"] = current_cfg;
    currentFunctionName = "main";
    
    // Créer le premier bloc de base
    current_bb = new BasicBlock(current_cfg, "BB_0");
    current_cfg->add_bb(current_bb);
    
    // Visiter les instructions du programme
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }
    
    // Générer le code assembleur une seule fois
    std::cout << "\t.section\t.text\n";
    std::cout << "\t.globl\tmain\n";
    std::cout << "\t.type\tmain, @function\n";
    std::cout << "main:\n";
    
    // Prologue
    std::cout << "\tpushq\t%rbp\n";
    std::cout << "\tmovq\t%rsp, %rbp\n";
    
    // Allouer l'espace pour les variables locales
    const int STACK_SIZE = 16;  // Taille minimale de 16 octets pour l'alignement
    std::cout << "\tsubq\t$" << STACK_SIZE << ", %rsp\n";
    
    // Générer le code des blocs de base
    for (auto bb : current_cfg->get_bbs()) {
        // Générer le label du bloc
        std::cout << bb->get_name() << ":\n";
        
        // Générer les instructions du bloc
        for (auto instr : bb->get_instrs()) {
            instr->gen_asm_x86(std::cout);
        }
    }
    
    // Épilogue
    std::cout << "\tmovq\t%rbp, %rsp\n";
    std::cout << "\tpopq\t%rbp\n";
    std::cout << "\tret\n";
    std::cout << "\t.size\tmain, .-main\n";
    std::cout << "\t.section\t.note.GNU-stack,\"\",@progbits\n";
    
    return 0;
}

antlrcpp::Any VisitorIR::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in return statement" << std::endl;
        return 0;
    }
    
    if (ctx->expr()) {
        antlrcpp::Any result = visit(ctx->expr());
        try {
            string resultStr = any_cast<string>(result);
            current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {"!0", resultStr});
        } catch (const std::bad_any_cast& e) {
            std::cerr << "Error: Invalid return value type" << std::endl;
            return 0;
        }
    }
    
    current_bb->add_IRInstr(IRInstr::Operation::ret, Type::INT_TYPE, {"!0"});
    return 0;
}

antlrcpp::Any VisitorIR::visitExpr_stmt(ifccParser::Expr_stmtContext *ctx) {
    return visit(ctx->expr());
}

antlrcpp::Any VisitorIR::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in declaration statement" << std::endl;
        return 0;
    }
    
    string varName = ctx->VAR()->getText();
    current_cfg->add_to_symbol_table(varName, Type::INT_TYPE);
    int varIndex = current_cfg->get_var_index(varName);
    
    if (ctx->expr()) {
        antlrcpp::Any result = visit(ctx->expr());
        try {
            string resultStr = any_cast<string>(result);
            current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {"!" + to_string(varIndex), resultStr});
        } catch (const std::bad_any_cast& e) {
            std::cerr << "Error: Invalid expression type in declaration" << std::endl;
            return 0;
        }
    }
    
    return 0;
}

antlrcpp::Any VisitorIR::visitVarExpr(ifccParser::VarExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in variable expression" << std::endl;
        return string("0");  // Retourner une valeur par défaut
    }
    
    string varName = ctx->VAR()->getText();
    int varIndex = current_cfg->get_var_index(varName);
    string result = "!" + to_string(nextFreeSymbolIndex++);
    
    current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT_TYPE, {result, "!" + to_string(varIndex)});
    return result;
}

antlrcpp::Any VisitorIR::visitConstExpr(ifccParser::ConstExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in constant expression" << std::endl;
        return string("0");  // Retourner une valeur par défaut
    }
    
    string value = ctx->CONST()->getText();
    string result = "!" + to_string(nextFreeSymbolIndex++);
    
    current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT_TYPE, {result, value});
    return result;
}

antlrcpp::Any VisitorIR::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in assignment expression" << std::endl;
        return string("0");  // Retourner une valeur par défaut
    }
    
    string varName = ctx->expr(0)->getText();
    int varIndex = current_cfg->get_var_index(varName);
    
    antlrcpp::Any result = visit(ctx->expr(1));
    try {
        string resultStr = any_cast<string>(result);
        current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {"!" + to_string(varIndex), resultStr});
        return resultStr;
    } catch (const std::bad_any_cast& e) {
        std::cerr << "Error: Invalid expression type in assignment" << std::endl;
        return string("0");
    }
}

antlrcpp::Any VisitorIR::visitAdditiveExpr(ifccParser::AdditiveExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in additive expression" << std::endl;
        return string("0");  // Retourner une valeur par défaut
    }
    
    string result = "!" + to_string(nextFreeSymbolIndex++);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));
    
    string leftStr = any_cast<string>(leftResult);
    string rightStr = any_cast<string>(rightResult);
    
    string op = ctx->children[1]->getText();
    IRInstr::Operation operation = (op == "+") ? IRInstr::Operation::add : IRInstr::Operation::sub;
    
    current_bb->add_IRInstr(operation, Type::INT_TYPE, {result, leftStr, rightStr});
    return result;
}

antlrcpp::Any VisitorIR::visitMultiplicativeExpr(ifccParser::MultiplicativeExprContext *ctx) {
    // Visiter les opérandes
    visit(ctx->expr(0));
    std::string leftVar = createTempVar(Type::INT_TYPE);
    
    visit(ctx->expr(1));
    std::string rightVar = createTempVar(Type::INT_TYPE);
    
    // Créer une variable temporaire pour le résultat
    std::string resultVar = createTempVar(Type::INT_TYPE);
    
    // Ajouter l'instruction de multiplication
    addInstr(IRInstr::mul, Type::INT_TYPE, {resultVar, leftVar, rightVar});
    
    return 0;
}

antlrcpp::Any VisitorIR::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
    // Visiter l'opérande
    visit(ctx->expr());
    std::string operandVar = createTempVar(Type::INT_TYPE);
    
    // Créer une variable temporaire pour le résultat
    std::string resultVar = createTempVar(Type::INT_TYPE);
    
    // Pour un moins unaire, on multiplie par -1
    std::string constVar = createTempVar(Type::INT_TYPE);
    addInstr(IRInstr::ldconst, Type::INT_TYPE, {constVar, "-1"});
    addInstr(IRInstr::mul, Type::INT_TYPE, {resultVar, operandVar, constVar});
    
    return 0;
}

antlrcpp::Any VisitorIR::visitParensExpr(ifccParser::ParensExprContext *ctx) {
    return visit(ctx->expr());
}

// ... Implémentation des autres méthodes de visite 