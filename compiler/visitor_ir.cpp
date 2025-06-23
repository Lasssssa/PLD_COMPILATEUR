#include "visitor_ir.h"
#include "DefFonction.h"
#include <sstream>
#include <iostream>
#include <algorithm> // Pour std::reverse
#include <set>

using std::to_string;
using namespace std;

VisitorIR::~VisitorIR()
{
    for (auto &pair : cfgs)
    {
        delete pair.second;
    }
}

string VisitorIR::createTempVar(Type t)
{
    return current_cfg->create_new_tempvar(t);
}

void VisitorIR::addInstr(IRInstr::Operation op, Type t, const vector<string> &params)
{
    current_bb->add_IRInstr(op, t, params);
}

BasicBlock *VisitorIR::createNewBB()
{
    string bbName = "BB_" + to_string(nextBBnumber++);
    BasicBlock *bb = new BasicBlock(current_cfg, bbName);
    current_cfg->add_bb(bb);
    return bb;
}

void VisitorIR::setCurrentBB(BasicBlock *bb)
{
    current_bb = bb;
    current_cfg->current_bb = bb;
}

void postOrderDFS(BasicBlock *bb, std::set<BasicBlock *> &visited, std::vector<BasicBlock *> &postOrder)
{
    if (!bb || visited.count(bb))
    {
        return;
    }
    visited.insert(bb);
    postOrderDFS(bb->exit_true, visited, postOrder);
    postOrderDFS(bb->exit_false, visited, postOrder);
    postOrder.push_back(bb);
}

antlrcpp::Any VisitorIR::visitProg(ifccParser::ProgContext *ctx)
{
    // Générer le prologue global
    std::cout << "\t.text\n";

    // Visiter toutes les fonctions pour construire les CFG
    for (auto func : ctx->function())
    {
        this->visit(func);
    }

    // Générer le code assembleur pour toutes les fonctions
    for (const auto &pair : cfgs)
    {
        std::string funcName = pair.first;
        CFG *cfg = pair.second;

// Déclarer la fonction comme globale
#ifdef __APPLE__
        // Pour macOS, main doit s'appeler _main
        if (funcName == "main")
        {
            std::cout << "\t.globl\t_main\n";
            std::cout << "_main:\n";
        }
        else
        {
            std::cout << "\t.globl\t" << funcName << "\n";
            std::cout << funcName << ":\n";
        }
#else
        // Pour Linux, main reste main
        std::cout << "\t.globl\t" << funcName << "\n";
        std::cout << funcName << ":\n";
#endif

        // Générer le prologue de la fonction
        cfg->gen_asm_prologue(std::cout);

        // Générer le code de tous les blocs de base avec un tri topologique (reverse post-order)
        std::vector<BasicBlock *> postOrder;
        std::set<BasicBlock *> visited_bbs;
        if (!cfg->get_bbs().empty())
        {
            postOrderDFS(cfg->get_bbs()[0], visited_bbs, postOrder);
        }
        std::reverse(postOrder.begin(), postOrder.end());

        for (auto bb : postOrder)
        {
            bb->gen_asm(std::cout);
        }

        // Générer l'épilogue de la fonction
        cfg->gen_asm_epilogue(std::cout);

        // Ajouter les directives de taille pour la fonction
#ifdef ARM
        std::cout << "\t.size " << funcName << ", .-" << funcName << "\n";
#else
#ifndef __APPLE__
        std::cout << "\t.size\t" << funcName << ", .-" << funcName << "\n";
#endif
#endif
    }

#ifndef __APPLE__
    std::cout << "\t.section\t.note.GNU-stack,\"\",@progbits\n";
#endif

    return 0;
}

antlrcpp::Any VisitorIR::visitFunction(ifccParser::FunctionContext *ctx)
{
    std::string funcName = ctx->VAR()->getText();

    // Créer la fonction
    std::vector<Param> params;
    if (ctx->param_list())
    {
        // Traiter les paramètres
        antlrcpp::Any paramResult = visit(ctx->param_list());
        try
        {
            params = any_cast<std::vector<Param>>(paramResult);
        }
        catch (const std::bad_any_cast &e)
        {
            std::cerr << "Error: Invalid parameter list type" << std::endl;
        }
    }

    DefFonction *func = new DefFonction(funcName, Type::INT_TYPE, params);
    current_cfg = new CFG(func);
    cfgs[funcName] = current_cfg;
    currentFunctionName = funcName;

    // Créer le bloc de base (il sera automatiquement ajouté au CFG)
    current_bb = new BasicBlock(current_cfg, funcName + "_BB_" + to_string(nextBBnumber++));

    // Ajouter les paramètres à la table des symboles et les récupérer depuis les registres
    for (size_t i = 0; i < params.size(); i++)
    {
        current_cfg->add_to_symbol_table(params[i].name, params[i].type);
        string paramVar = "!" + to_string(current_cfg->get_var_index(params[i].name));

        // Récupérer les paramètres depuis les registres selon la convention x86_64
        if (i == 0)
        {
            current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {paramVar, "%edi"});
        }
        else if (i == 1)
        {
            current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {paramVar, "%esi"});
        }
        else if (i == 2)
        {
            current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {paramVar, "%edx"});
        }
        else if (i == 3)
        {
            current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {paramVar, "%ecx"});
        }
        else if (i == 4)
        {
            current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {paramVar, "%r8d"});
        }
        else if (i == 5)
        {
            current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {paramVar, "%r9d"});
        }
        // Pour plus de 6 paramètres, ils seraient sur la pile
    }

    // Visiter les instructions de la fonction
    visit(ctx->block_stmt());

    return 0;
}

antlrcpp::Any VisitorIR::visitBlock_stmt(ifccParser::Block_stmtContext *ctx)
{
    for (auto stmt : ctx->stmt())
    {
        visit(stmt);
    }
    return 0;
}

antlrcpp::Any VisitorIR::visitIf_stmt(ifccParser::If_stmtContext *ctx)
{
    // 1. Évaluer l'expression de la condition
    visit(ctx->expr());

    // 2. Créer les blocs de base pour les branches then, else, et pour la suite
    BasicBlock *then_bb = createNewBB();
    BasicBlock *after_if_bb = createNewBB();
    BasicBlock *else_bb = after_if_bb;

    if (ctx->ELSE())
    {
        else_bb = createNewBB();
    }

    // 3. Le bloc courant se termine par un saut conditionnel
    current_bb->exit_true = then_bb;
    current_bb->exit_false = else_bb;

    // 4. Générer le code pour le bloc 'then'
    setCurrentBB(then_bb);
    visit(ctx->stmt(0));
    if (current_bb->exit_true == nullptr && current_bb->exit_false == nullptr)
    {
        current_bb->exit_true = after_if_bb; // Saut inconditionnel vers la suite
    }

    // 5. Générer le code pour le bloc 'else' s'il existe
    if (ctx->ELSE())
    {
        setCurrentBB(else_bb);
        visit(ctx->stmt(1));
        if (current_bb->exit_true == nullptr && current_bb->exit_false == nullptr)
        {
            current_bb->exit_true = after_if_bb; // Saut inconditionnel vers la suite
        }
    }

    // 6. Continuer la génération de code dans le bloc qui suit le if
    setCurrentBB(after_if_bb);

    return 0;
}

antlrcpp::Any VisitorIR::visitReturn_stmt(ifccParser::Return_stmtContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in return statement" << std::endl;
        return 0;
    }

    if (ctx->expr())
    {
        antlrcpp::Any result = visit(ctx->expr());
        try
        {
            string resultStr = any_cast<string>(result);

            // Si le résultat est déjà dans une variable temporaire, l'utiliser directement
            if (resultStr[0] == '!')
            {
                // Le résultat est déjà dans une variable temporaire, l'utiliser directement
                current_bb->add_IRInstr(IRInstr::Operation::ret, Type::INT_TYPE, {resultStr});
            }
            else
            {
                // Copier dans la variable de retour
                current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {"!0", resultStr});
                current_bb->add_IRInstr(IRInstr::Operation::ret, Type::INT_TYPE, {"!0"});
            }
        }
        catch (const std::bad_any_cast &e)
        {
            std::cerr << "Error: Invalid return value type" << std::endl;
            return 0;
        }
    }
    else
    {
        current_bb->add_IRInstr(IRInstr::Operation::ret, Type::INT_TYPE, {"!0"});
    }

    return 0;
}

antlrcpp::Any VisitorIR::visitExpr_stmt(ifccParser::Expr_stmtContext *ctx)
{
    return visit(ctx->expr());
}

antlrcpp::Any VisitorIR::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in declaration statement" << std::endl;
        return 0;
    }

    string varName = ctx->VAR()->getText();
    current_cfg->add_to_symbol_table(varName, Type::INT_TYPE);
    int varIndex = current_cfg->get_var_index(varName);

    if (ctx->expr())
    {
        antlrcpp::Any result = visit(ctx->expr());
        try
        {
            string resultStr = any_cast<string>(result);
            current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {"!" + to_string(varIndex), resultStr});
        }
        catch (const std::bad_any_cast &e)
        {
            std::cerr << "Error: Invalid expression type in declaration" << std::endl;
            return 0;
        }
    }

    return 0;
}

antlrcpp::Any VisitorIR::visitVarExpr(ifccParser::VarExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
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

antlrcpp::Any VisitorIR::visitConstExpr(ifccParser::ConstExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in constant expression" << std::endl;
        return string("0");
    }

    string value = ctx->CONST()->getText();
    string result = createTempVar(Type::INT_TYPE);

    current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT_TYPE, {result, value});
    return result;
}

antlrcpp::Any VisitorIR::visitCharExpr(ifccParser::CharExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in character expression" << std::endl;
        return string("0");
    }

    string charLiteral = ctx->CHAR_LITERAL()->getText();
    // Extraire le caractère entre les guillemets simples
    // charLiteral est de la forme 'a', on veut extraire 'a'
    char character = charLiteral[1]; // Le caractère est à l'index 1
    string value = to_string((int)character); // Convertir en valeur ASCII
    
    string result = createTempVar(Type::INT_TYPE);
    current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT_TYPE, {result, value});
    return result;
}

antlrcpp::Any VisitorIR::visitAssignExpr(ifccParser::AssignExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in assignment expression" << std::endl;
        return string("0");
    }

    // Évaluer d'abord l'expression de droite
    antlrcpp::Any rightResult = visit(ctx->expr(1));
    string rightStr = any_cast<string>(rightResult);

    // Gérer le côté gauche
    if (auto varExpr = dynamic_cast<ifccParser::VarExprContext *>(ctx->expr(0)))
    {
        // Cas simple : variable = expression
        string varName = varExpr->VAR()->getText();
        int varIndex = current_cfg->get_var_index(varName);
        current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT_TYPE, {"!" + to_string(varIndex), rightStr});
        return rightStr;
    }
    else
    {
        // Cas d'assignation chaînée : (expr = expr) = expr
        // On doit d'abord évaluer l'assignation de gauche
        antlrcpp::Any leftResult = visit(ctx->expr(0));
        string leftStr = any_cast<string>(leftResult);

        // Pour l'assignation chaînée, on retourne simplement la valeur de droite
        // car l'assignation de gauche a déjà été traitée et a retourné cette valeur
        return rightStr;
    }
}

antlrcpp::Any VisitorIR::visitAdditiveExpr(ifccParser::AdditiveExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in additive expression" << std::endl;
        return string("0");
    }

    string result = createTempVar(Type::INT_TYPE);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    try
    {
        string leftStr = any_cast<string>(leftResult);
        string rightStr = any_cast<string>(rightResult);

        string op = ctx->children[1]->getText();
        IRInstr::Operation operation = (op == "+") ? IRInstr::Operation::add : IRInstr::Operation::sub;

        current_bb->add_IRInstr(operation, Type::INT_TYPE, {result, leftStr, rightStr});
        return result;
    }
    catch (const std::bad_any_cast &e)
    {
        std::cerr << "Error: Invalid type in additive expression" << std::endl;
        return string("0");
    }
}

antlrcpp::Any VisitorIR::visitMultiplicativeExpr(ifccParser::MultiplicativeExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in multiplicative expression" << std::endl;
        return string("0");
    }

    string result = createTempVar(Type::INT_TYPE);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    try
    {
        string leftStr = any_cast<string>(leftResult);
        string rightStr = any_cast<string>(rightResult);

        string op = ctx->children[1]->getText();
        IRInstr::Operation operation;
        if (op == "*")
            operation = IRInstr::Operation::mul;
        else if (op == "/")
            operation = IRInstr::Operation::div;
        else if (op == "%")
            operation = IRInstr::Operation::mod;
        else
            throw std::runtime_error("Opération multiplicative inconnue");

        current_bb->add_IRInstr(operation, Type::INT_TYPE, {result, leftStr, rightStr});
        return result;
    }
    catch (const std::bad_any_cast &e)
    {
        std::cerr << "Error: Invalid type in multiplicative expression" << std::endl;
        return string("0");
    }
}

antlrcpp::Any VisitorIR::visitUnaryExpr(ifccParser::UnaryExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in unary expression" << std::endl;
        return string("0");
    }

    antlrcpp::Any operandResult = visit(ctx->expr());
    string operandVar = any_cast<string>(operandResult);
    string resultVar = createTempVar(Type::INT_TYPE);

    string op = ctx->children[0]->getText();
    if (op == "-")
    {
        // Pour un moins unaire, on multiplie par -1
        string constVar = createTempVar(Type::INT_TYPE);
        current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT_TYPE, {constVar, "-1"});
        current_bb->add_IRInstr(IRInstr::Operation::mul, Type::INT_TYPE, {resultVar, operandVar, constVar});
    }
    else if (op == "+")
    {
        // Pour un plus unaire, on copie simplement la valeur
        current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT_TYPE, {resultVar, operandVar});
    }
    else if (op == "!")
    {
        // Pour la négation logique, on utilise l'opération NOT
        current_bb->add_IRInstr(IRInstr::Operation::not_op, Type::INT_TYPE, {resultVar, operandVar});
    }

    return resultVar;
}

antlrcpp::Any VisitorIR::visitParensExpr(ifccParser::ParensExprContext *ctx)
{
    return visit(ctx->expr());
}

antlrcpp::Any VisitorIR::visitCallExpr(ifccParser::CallExprContext *ctx)
{
    std::cerr << "DEBUG: visitCallExpr called for function " << ctx->VAR()->getText() << std::endl;

    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in call expression" << std::endl;
        return string("0");
    }

    string funcName = ctx->VAR()->getText();
    string result = createTempVar(Type::INT_TYPE);

    // Préparer les paramètres pour l'instruction call
    vector<string> callParams = {funcName, result};

    // Ajouter les arguments
    if (ctx->arg_list())
    {
        antlrcpp::Any argListResult = visit(ctx->arg_list());
        try
        {
            vector<string> args = any_cast<vector<string>>(argListResult);
            callParams.insert(callParams.end(), args.begin(), args.end());
        }
        catch (const std::bad_any_cast &e)
        {
            std::cerr << "Error: Invalid argument list type" << std::endl;
        }
    }

    std::cerr << "DEBUG: Adding call instruction with " << callParams.size() << " parameters" << std::endl;

    // Ajouter l'instruction d'appel avec tous les paramètres
    current_bb->add_IRInstr(IRInstr::Operation::call, Type::INT_TYPE, callParams);

    return result;
}

antlrcpp::Any VisitorIR::visitParam_list(ifccParser::Param_listContext *ctx)
{
    std::vector<Param> params;

    // Traiter tous les paramètres
    for (size_t i = 0; i < ctx->VAR().size(); i++)
    {
        std::string paramName = ctx->VAR(i)->getText();
        // Pour l'instant, tous les paramètres sont de type int
        params.emplace_back(paramName, Type::INT_TYPE);
    }

    return params;
}

antlrcpp::Any VisitorIR::visitArg_list(ifccParser::Arg_listContext *ctx)
{
    std::vector<std::string> args;

    // Traiter tous les arguments
    for (auto expr : ctx->expr())
    {
        antlrcpp::Any argResult = visit(expr);
        string argStr = any_cast<string>(argResult);
        args.push_back(argStr);
    }

    return args;
}

antlrcpp::Any VisitorIR::visitEqualityExpr(ifccParser::EqualityExprContext *ctx)
{
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    string leftStr = any_cast<string>(leftResult);
    string rightStr = any_cast<string>(rightResult);

    string op = ctx->children[1]->getText();

    IRInstr::Operation operation;
    if (op == "==")
        operation = IRInstr::Operation::cmp_eq;
    else if (op == "!=")
        operation = IRInstr::Operation::cmp_ne;
    else
        throw std::runtime_error("Opérateur d'égalité inconnu");

    string result = createTempVar(Type::INT_TYPE);
    current_bb->add_IRInstr(operation, Type::INT_TYPE, {result, leftStr, rightStr});
    return result;
}

antlrcpp::Any VisitorIR::visitRelationalExpr(ifccParser::RelationalExprContext *ctx)
{
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    string leftStr = any_cast<string>(leftResult);
    string rightStr = any_cast<string>(rightResult);

    string op = ctx->children[1]->getText();

    IRInstr::Operation operation;
    if (op == "<")
        operation = IRInstr::Operation::cmp_lt;
    else if (op == ">")
        operation = IRInstr::Operation::cmp_gt;
    else if (op == "<=")
        operation = IRInstr::Operation::cmp_le;
    else if (op == ">=")
        operation = IRInstr::Operation::cmp_ge;
    else
        throw std::runtime_error("Opérateur de comparaison inconnu");

    string result = createTempVar(Type::INT_TYPE);
    current_bb->add_IRInstr(operation, Type::INT_TYPE, {result, leftStr, rightStr});
    return result;
}

antlrcpp::Any VisitorIR::visitBitwiseAndExpr(ifccParser::BitwiseAndExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in bitwise AND expression" << std::endl;
        return string("0");
    }

    string result = createTempVar(Type::INT_TYPE);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    try
    {
        string leftStr = any_cast<string>(leftResult);
        string rightStr = any_cast<string>(rightResult);

        current_bb->add_IRInstr(IRInstr::Operation::bit_and, Type::INT_TYPE, {result, leftStr, rightStr});
        return result;
    }
    catch (const std::bad_any_cast &e)
    {
        std::cerr << "Error: Invalid type in bitwise AND expression" << std::endl;
        return string("0");
    }
}

antlrcpp::Any VisitorIR::visitBitwiseXorExpr(ifccParser::BitwiseXorExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in bitwise XOR expression" << std::endl;
        return string("0");
    }

    string result = createTempVar(Type::INT_TYPE);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    try
    {
        string leftStr = any_cast<string>(leftResult);
        string rightStr = any_cast<string>(rightResult);

        current_bb->add_IRInstr(IRInstr::Operation::bit_xor, Type::INT_TYPE, {result, leftStr, rightStr});
        return result;
    }
    catch (const std::bad_any_cast &e)
    {
        std::cerr << "Error: Invalid type in bitwise XOR expression" << std::endl;
        return string("0");
    }
}

antlrcpp::Any VisitorIR::visitBitwiseOrExpr(ifccParser::BitwiseOrExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in bitwise OR expression" << std::endl;
        return string("0");
    }

    string result = createTempVar(Type::INT_TYPE);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    try
    {
        string leftStr = any_cast<string>(leftResult);
        string rightStr = any_cast<string>(rightResult);

        current_bb->add_IRInstr(IRInstr::Operation::bit_or, Type::INT_TYPE, {result, leftStr, rightStr});
        return result;
    }
    catch (const std::bad_any_cast &e)
    {
        std::cerr << "Error: Invalid type in bitwise OR expression" << std::endl;
        return string("0");
    }
}

antlrcpp::Any VisitorIR::visitLogicalAndExpr(ifccParser::LogicalAndExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in logical AND expression" << std::endl;
        return string("0");
    }

    string result = createTempVar(Type::INT_TYPE);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    try
    {
        string leftStr = any_cast<string>(leftResult);
        string rightStr = any_cast<string>(rightResult);

        // Pour l'opérateur && paresseux, on utilise l'opération logical_and
        current_bb->add_IRInstr(IRInstr::Operation::logical_and, Type::INT_TYPE, {result, leftStr, rightStr});
        return result;
    }
    catch (const std::bad_any_cast &e)
    {
        std::cerr << "Error: Invalid type in logical AND expression" << std::endl;
        return string("0");
    }
}

antlrcpp::Any VisitorIR::visitLogicalOrExpr(ifccParser::LogicalOrExprContext *ctx)
{
    if (current_cfg == nullptr)
    {
        std::cerr << "Error: No current CFG in logical OR expression" << std::endl;
        return string("0");
    }

    string result = createTempVar(Type::INT_TYPE);
    antlrcpp::Any leftResult = visit(ctx->expr(0));
    antlrcpp::Any rightResult = visit(ctx->expr(1));

    try
    {
        string leftStr = any_cast<string>(leftResult);
        string rightStr = any_cast<string>(rightResult);

        // Pour l'opérateur || paresseux, on utilise l'opération logical_or
        current_bb->add_IRInstr(IRInstr::Operation::logical_or, Type::INT_TYPE, {result, leftStr, rightStr});
        return result;
    }
    catch (const std::bad_any_cast &e)
    {
        std::cerr << "Error: Invalid type in logical OR expression" << std::endl;
        return string("0");
    }
}