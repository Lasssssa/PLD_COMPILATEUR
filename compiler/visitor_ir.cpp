#include "visitor_ir.h"
#include "DefFonction.h"
#include <sstream>
#include <iostream>

using std::to_string;
using namespace std;

VisitorIR::~VisitorIR() {
    for (auto &pair: cfgs) {
        delete pair.second;
    }
}

string VisitorIR::createTempVar(Type t) {
    return current_cfg->create_new_tempvar(t);
}

void VisitorIR::addInstr(IRInstr::Operation op, Type t, const vector <string> &params) {
    current_bb->add_IRInstr(op, t, params);
}

BasicBlock *VisitorIR::createNewBB() {
    string bbName = "BB_" + to_string(nextBBnumber++);
    BasicBlock *bb = new BasicBlock(current_cfg, bbName);
    current_cfg->add_bb(bb);
    return bb;
}

void VisitorIR::setCurrentBB(BasicBlock *bb) {
    current_bb = bb;
    current_cfg->current_bb = bb;
}

antlrcpp::Any VisitorIR::visitProg(ifccParser::ProgContext *ctx) {
    // Créer la fonction main
    DefFonction *main = new DefFonction("main", Type::INT_TYPE, {});
    current_cfg = new CFG(main);
    cfgs["main"] = current_cfg;
    currentFunctionName = "main";

    current_bb = new BasicBlock(current_cfg, "BB_0");

    // Visiter les instructions du programme
    for (auto stmt: ctx->stmt()) {
        visit(stmt);
    }

    // Générer le code assembleur une seule fois
#ifdef __APPLE__
    std::cout << ".globl _main\n";
    std::cout << "_main:\n";
#else
    std::cout << "\t.section\t.text\n";
    std::cout << ".globl main\n";
    std::cout << "main:\n";
    std::cout << "\t.type\tmain, @function\n";
#endif

    // Prologue
    std::cout << "\tpushq\t%rbp\n";
    std::cout << "\tmovq\t%rsp, %rbp\n";

    // Allouer l'espace pour les variables locales
    int maxNegOffset = 0;
    for (const auto &p: symbolTable) {
        if (p.second < maxNegOffset)
            maxNegOffset = p.second;
    }
    int stackSize = -maxNegOffset; // taille en octets
    if (stackSize > 0) {
        // Arrondir à 16 pour l'alignement
        int aligned = ((stackSize + 15) / 16) * 16;
        std::cout << "\tsubq\t$" << aligned << ", %rsp\n";
    } else {
        // Taille minimale de 16 octets pour l'alignement
        std::cout << "\tsubq\t$16, %rsp\n";
    }

    // Générer le code des blocs de base
    for (auto bb: current_cfg->get_bbs()) {

        // Générer le label du bloc
        std::cout << bb->get_name() << ":\n";

        // Générer les instructions du bloc
        for (auto instr: bb->get_instrs()) {
            instr->gen_asm_x86(std::cout);
        }
    }

    #ifdef __APPLE__
        std::cout << "    movq %rbp, %rsp\n";
        std::cout << "    popq %rbp\n";
        std::cout << "    ret\n";
    #else
        std::cout << "\tmovq\t%rbp, %rsp\n";
        std::cout << "\tpopq\t%rbp\n";
        std::cout << "\tret\n";
        std::cout << "\t.size\tmain, .-main\n";
        std::cout << "\t.section\t.note.GNU-stack,\"\",@progbits\n";
    #endif

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
            
            // Si le résultat est déjà dans une variable temporaire, l'utiliser directement
            if (resultStr[0] == '!') {
                // Le résultat est déjà dans une variable temporaire, l'utiliser directement
                current_bb->add_IRInstr(IRInstr::Operation::ret, Type::INT_TYPE, {resultStr});
            } else {
                // Copier dans la variable de retour
                current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {"!0", resultStr});
                current_bb->add_IRInstr(IRInstr::Operation::ret, Type::INT_TYPE, {"!0"});
            }
        } catch (const std::bad_any_cast &e) {
            std::cerr << "Error: Invalid return value type" << std::endl;
            return 0;
        }
    } else {
        current_bb->add_IRInstr(IRInstr::Operation::ret, Type::INT_TYPE, {"!0"});
    }

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
        } catch (const std::bad_any_cast &e) {
            std::cerr << "Error: Invalid expression type in declaration" << std::endl;
            return 0;
        }
    }

    return 0;
}

antlrcpp::Any VisitorIR::visitVarExpr(ifccParser::VarExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in variable expression" << std::endl;
        return string("0");
    }

    string varName = ctx->VAR()->getText();
    int varIndex = current_cfg->get_var_index(varName);
    
    // Si c'est dans le contexte d'une assignation, on peut retourner directement l'adresse
    // Sinon, on lit la valeur
    string result = createTempVar(Type::INT_TYPE);
    current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT_TYPE, {result, "!" + to_string(varIndex)});
    return result;
}

antlrcpp::Any VisitorIR::visitConstExpr(ifccParser::ConstExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in constant expression" << std::endl;
        return string("0");
    }

    string value = ctx->CONST()->getText();
    string result = createTempVar(Type::INT_TYPE);

    current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT_TYPE, {result, value});
    return result;
}

antlrcpp::Any VisitorIR::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in assignment expression" << std::endl;
        return string("0");
    }

    // Évaluer d'abord l'expression de droite
    antlrcpp::Any rightResult = visit(ctx->expr(1));
    string rightStr = any_cast<string>(rightResult);

    // Gérer le côté gauche
    if (auto varExpr = dynamic_cast<ifccParser::VarExprContext *>(ctx->expr(0))) {
        // Cas simple : variable = expression
        string varName = varExpr->VAR()->getText();
        int varIndex = current_cfg->get_var_index(varName);
        current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {"!" + to_string(varIndex), rightStr});
        return rightStr;
    } else {
        // Cas d'assignation chaînée : (expr = expr) = expr
        // On doit d'abord évaluer l'assignation de gauche
        antlrcpp::Any leftResult = visit(ctx->expr(0));
        string leftStr = any_cast<string>(leftResult);
        
        // Pour l'assignation chaînée, on retourne simplement la valeur de droite
        // car l'assignation de gauche a déjà été traitée et a retourné cette valeur
        return rightStr;
    }
}

antlrcpp::Any VisitorIR::visitAdditiveExpr(ifccParser::AdditiveExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in additive expression" << std::endl;
        return string("0");
    }

    string result = createTempVar(Type::INT_TYPE);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    try {
        string leftStr = any_cast<string>(leftResult);
        string rightStr = any_cast<string>(rightResult);

        string op = ctx->children[1]->getText();
        IRInstr::Operation operation = (op == "+") ? IRInstr::Operation::add : IRInstr::Operation::sub;

        current_bb->add_IRInstr(operation, Type::INT_TYPE, {result, leftStr, rightStr});
        return result;
    } catch (const std::bad_any_cast &e) {
        std::cerr << "Error: Invalid type in additive expression" << std::endl;
        return string("0");
    }
}

antlrcpp::Any VisitorIR::visitMultiplicativeExpr(ifccParser::MultiplicativeExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in multiplicative expression" << std::endl;
        return string("0");
    }

    string result = createTempVar(Type::INT_TYPE);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    try {
        string leftStr = any_cast<string>(leftResult);
        string rightStr = any_cast<string>(rightResult);

        string op = ctx->children[1]->getText();
        IRInstr::Operation operation = (op == "*") ? IRInstr::Operation::mul : IRInstr::Operation::div;

        current_bb->add_IRInstr(operation, Type::INT_TYPE, {result, leftStr, rightStr});
        return result;
    } catch (const std::bad_any_cast &e) {
        std::cerr << "Error: Invalid type in multiplicative expression" << std::endl;
        return string("0");
    }
}

antlrcpp::Any VisitorIR::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
    if (current_cfg == nullptr) {
        std::cerr << "Error: No current CFG in unary expression" << std::endl;
        return string("0");
    }

    antlrcpp::Any operandResult = visit(ctx->expr());
    string operandVar = any_cast<string>(operandResult);
    string resultVar = createTempVar(Type::INT_TYPE);

    string op = ctx->children[0]->getText();
    if (op == "-") {
        // Pour un moins unaire, on multiplie par -1
        string constVar = createTempVar(Type::INT_TYPE);
        current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT_TYPE, {constVar, "-1"});
        current_bb->add_IRInstr(IRInstr::Operation::mul, Type::INT_TYPE, {resultVar, operandVar, constVar});
    } else {
        // Pour un plus unaire, on copie simplement la valeur
        current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT_TYPE, {resultVar, operandVar});
    }

    return resultVar;
}

antlrcpp::Any VisitorIR::visitParensExpr(ifccParser::ParensExprContext *ctx) {
    return visit(ctx->expr());
}
