#include "SymbolTableVisitor.h"

SymbolTableVisitor::SymbolTableVisitor() : currentOffset(-8), hasErrors(false)
{
}

antlrcpp::Any SymbolTableVisitor::visitProg(ifccParser::ProgContext *ctx)
{
    std::cerr << "=== ANALYSE DE LA TABLE DES SYMBOLES ===" << std::endl;

    // Visiter tous les statements
    for (auto stmt : ctx->stmt())
    {
        this->visit(stmt);
    }

    // Vérifier les variables non utilisées
    checkUnusedVariables();

    std::cerr << "=== TABLE DES SYMBOLES FINALE ===" << std::endl;
    for (const auto &pair : symbolTable)
    {
        std::cerr << "Variable '" << pair.first << "' -> offset " << pair.second << " (%rbp" << pair.second << ")" << std::endl;
    }
    std::cerr << "========================================" << std::endl;

    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx)
{
    std::string varName = ctx->VAR()->getText();

    // Vérifier si la variable est déjà déclarée
    if (declaredVars.find(varName) != declaredVars.end())
    {
        std::cerr << "ERREUR: Variable '" << varName << "' déclarée plusieurs fois!" << std::endl;
        hasErrors = true;
        return 0;
    }

    // Ajouter la variable à la table des symboles
    symbolTable[varName] = currentOffset;
    declaredVars.insert(varName);

    std::cerr << "Déclaration: Variable '" << varName << "' assignée à l'offset " << currentOffset << std::endl;

    // Décrémenter l'offset pour la prochaine variable (multiple de 4)
    currentOffset -= 4;

    // Si il y a une initialisation, visiter l'expression (côté droit seulement)
    if (ctx->expr())
    {
        std::cerr << "Initialisation de '" << varName << "' avec expression..." << std::endl;
        this->visit(ctx->expr());
    }

    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitVarExpr(ifccParser::VarExprContext *ctx)
{
    std::string varName = ctx->VAR()->getText();

    // Vérifier si la variable a été déclarée
    if (declaredVars.find(varName) == declaredVars.end())
    {
        std::cerr << "ERREUR: Variable '" << varName << "' utilisée sans être déclarée!" << std::endl;
        hasErrors = true;
        return 0;
    }

    // Marquer la variable comme utilisée
    usedVars.insert(varName);

    std::cerr << "Utilisation: Variable '" << varName << "' (offset " << symbolTable[varName] << ")" << std::endl;

    return 0;
}

antlrcpp::Any SymbolTableVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx)
{
    std::cerr << "Traitement d'une affectation..." << std::endl;

    // D'abord visiter le côté droit (l'expression à affecter)
    // Cela va marquer les variables utilisées dans l'expression de droite
    std::cerr << "Évaluation du côté droit de l'affectation..." << std::endl;
    this->visit(ctx->expr(1));

    // Gérer le côté gauche
    if (auto varExpr = dynamic_cast<ifccParser::VarExprContext *>(ctx->expr(0)))
    {
        // Cas simple : variable = expression
        std::string varName = varExpr->VAR()->getText();

        // Vérifier que la variable est déclarée
        if (declaredVars.find(varName) == declaredVars.end())
        {
            std::cerr << "ERREUR: Variable '" << varName << "' utilisée sans être déclarée!" << std::endl;
            hasErrors = true;
        }
        else
        {
            std::cerr << "Assignation à la variable '" << varName << "' (offset " << symbolTable[varName] << ")" << std::endl;
            // NOTE: On ne marque PAS la variable comme utilisée ici, c'est une assignation
        }
    }
    else if (auto assignExpr = dynamic_cast<ifccParser::AssignExprContext *>(ctx->expr(0)))
    {
        // Cas d'assignation chaînée : (expr = expr) = expr
        std::cerr << "Traitement d'une assignation chaînée..." << std::endl;
        this->visit(assignExpr);
    }
    else
    {
        std::cerr << "ERREUR: Le côté gauche d'une affectation doit être une variable ou une autre assignation!" << std::endl;
        hasErrors = true;
    }

    return 0;
}

void SymbolTableVisitor::checkUnusedVariables()
{
    std::cerr << "=== VÉRIFICATION DES VARIABLES NON UTILISÉES ===" << std::endl;

    bool foundUnused = false;
    for (const std::string &varName : declaredVars)
    {
        if (usedVars.find(varName) == usedVars.end())
        {
            std::cerr << "AVERTISSEMENT: Variable '" << varName << "' déclarée mais jamais utilisée!" << std::endl;
            foundUnused = true;
        }
    }

    if (!foundUnused)
    {
        std::cerr << "Toutes les variables déclarées sont utilisées." << std::endl;
    }
}