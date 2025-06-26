// SymbolTableVisitor.cpp : Implémentation du visiteur pour la table des symboles et les analyses statiques
#include "SymbolTableVisitor.h"

// Constructeur : initialise les structures et ajoute les fonctions externes
SymbolTableVisitor::SymbolTableVisitor() : currentOffset(-8), hasErrors(false) {
    declaredFunctions.insert("putchar");
    declaredFunctions.insert("getchar");
    functionParamCount["putchar"] = 1;
    functionParamCount["getchar"] = 0;
}

// Visite du programme : analyse toutes les déclarations globales et fonctions
antlrcpp::Any SymbolTableVisitor::visitProg(ifccParser::ProgContext *ctx)
{
    std::cerr << "=== ANALYSE DE LA TABLE DES SYMBOLES ===" << std::endl;

    // Visiter toutes les déclarations globales d'abord
    for (auto globalDecl : ctx->global_decl())
    {
        this->visit(globalDecl);
    }

    // Visiter toutes les fonctions
    for (auto func : ctx->function())
    {
        this->visit(func);
    }

    // Vérifier les variables non utilisées
    checkUnusedVariables();

    // Vérifier la présence de la fonction main
    checkMainFunction();

    // Affichage de la table des symboles finale
    std::cerr << "=== TABLE DES SYMBOLES FINALE ===" << std::endl;
    for (const auto &pair : symbolTable)
    {
        std::cerr << "Variable '" << pair.first << "' -> offset " << pair.second << " (%rbp" << pair.second << ")" << std::endl;
    }
    std::cerr << "========================================" << std::endl;

    return 0;
}

// Visite d'une fonction : ajoute la fonction à la liste, gère les paramètres et le corps
antlrcpp::Any SymbolTableVisitor::visitFunction(ifccParser::FunctionContext *ctx)
{
    std::string funcName = ctx->VAR()->getText();
    std::cerr << "=== ANALYSE DE LA FONCTION '" << funcName << "' ===" << std::endl;
    
    // Ajouter la fonction à la liste des fonctions déclarées
    declaredFunctions.insert(funcName);
    
    // Définir la fonction courante
    currentFunction = funcName;
    
    // Réinitialiser les variables locales pour cette fonction (mais pas les globales)
    declaredVars.clear(); // Seulement les variables locales
    usedVars.clear();
    // On commence à -8 pour les variables locales : offsets négatifs par rapport à %rbp (convention x86_64)
    // Cela permet de placer chaque variable locale à une adresse unique sur la pile, sans écraser les paramètres ni le retour
    currentOffset = -8; // Commencer à -8 pour les variables locales
    
    // Traiter les paramètres
    if (ctx->param_list())
    {
        this->visit(ctx->param_list());
    }
    
    // Visiter le corps de la fonction
    this->visit(ctx->block_stmt());
    
    int paramCount = 0;
    if (ctx->param_list()) {
        paramCount = ctx->param_list()->VAR().size();
    }
    functionParamCount[funcName] = paramCount;
    
    return 0;
}

// Visite de la liste des paramètres : affecte un offset positif à chaque paramètre
antlrcpp::Any SymbolTableVisitor::visitParam_list(ifccParser::Param_listContext *ctx)
{
    // Les paramètres sont stockés à des offsets positifs (rarement utilisés ici, car on lit surtout dans les registres)
    // Convention x86_64 : les 6 premiers paramètres sont passés dans les registres (%rdi, %rsi, %rdx, %rcx, %r8, %r9)
    // On assigne tout de même un offset pour compatibilité et pour les architectures qui passent tout sur la pile
    int paramOffset = 16; // Commencer après %rbp et l'adresse de retour
    
    for (auto param : ctx->VAR())
    {
        std::string paramName = param->getText();
        symbolTable[paramName] = paramOffset;
        declaredVars.insert(paramName);
        std::cerr << "Paramètre: '" << paramName << "' assigné à l'offset " << paramOffset << std::endl;
        paramOffset += 8; // Chaque paramètre prend 8 octets (alignement stack, même si int = 4)
    }
    
    return 0;
}

// Visite d'une déclaration de variable locale (avec ou sans initialisation)
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
    // On utilise un offset négatif pour placer la variable sur la pile (convention x86_64)
    symbolTable[varName] = currentOffset;
    declaredVars.insert(varName);

    std::cerr << "Déclaration: Variable '" << varName << "' assignée à l'offset " << currentOffset << std::endl;

    // Décrémenter l'offset pour la prochaine variable (multiple de 4)
    // Chaque variable locale de type int occupe 4 octets sur la pile (taille d'un int en C).
    // On décrémente donc l'offset de 4 à chaque déclaration pour réserver l'espace nécessaire.
    // Cela garantit un accès aligné et compatible avec l'ABI x86_64 pour les types int.
    currentOffset -= 4;

    // Si il y a une initialisation, visiter l'expression (côté droit seulement)
    if (ctx->expr())
    {
        std::cerr << "Initialisation de '" << varName << "' avec expression..." << std::endl;
        this->visit(ctx->expr());
    }

    return 0;
}

// Visite d'une variable (utilisation dans une expression)
antlrcpp::Any SymbolTableVisitor::visitVarExpr(ifccParser::VarExprContext *ctx)
{
    std::string varName = ctx->VAR()->getText();

    // Vérifier si la variable a été déclarée (locale ou globale)
    if (declaredVars.find(varName) == declaredVars.end() && globalVars.find(varName) == globalVars.end())
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

// Visite d'une assignation (variable = expression ou assignation chaînée)
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

        // Vérifier que la variable est déclarée (locale ou globale)
        if (declaredVars.find(varName) == declaredVars.end() && globalVars.find(varName) == globalVars.end())
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

// Vérifie les variables non utilisées (affiche un avertissement)
void SymbolTableVisitor::checkUnusedVariables()
{
    std::cerr << "=== VÉRIFICATION DES VARIABLES NON UTILISÉES ===" << std::endl;

    bool foundUnused = false;
    
    // Vérifier les variables locales
    for (const std::string &varName : declaredVars)
    {
        if (usedVars.find(varName) == usedVars.end())
        {
            std::cerr << "AVERTISSEMENT: Variable locale '" << varName << "' déclarée mais jamais utilisée!" << std::endl;
            foundUnused = true;
        }
    }
    
    // Vérifier les variables globales
    for (const std::string &varName : globalVars)
    {
        if (usedVars.find(varName) == usedVars.end())
        {
            std::cerr << "AVERTISSEMENT: Variable globale '" << varName << "' déclarée mais jamais utilisée!" << std::endl;
            foundUnused = true;
        }
    }

    if (!foundUnused)
    {
        std::cerr << "Toutes les variables déclarées sont utilisées." << std::endl;
    }
}

// Vérifie la présence de la fonction main
void SymbolTableVisitor::checkMainFunction()
{
    std::cerr << "=== VÉRIFICATION DE LA FONCTION MAIN ===" << std::endl;
    
    if (declaredFunctions.find("main") == declaredFunctions.end())
    {
        std::cerr << "ERREUR: Fonction 'main' manquante dans le programme!" << std::endl;
        hasErrors = true;
    }
    else
    {
        std::cerr << "Fonction 'main' trouvée." << std::endl;
    }
}

// Visite d'un appel de fonction : vérifie la déclaration et le nombre d'arguments
antlrcpp::Any SymbolTableVisitor::visitCallExpr(ifccParser::CallExprContext *ctx) {
    std::string calledFunc = ctx->VAR()->getText();
    // Vérifier si la fonction appelée est déclarée
    if (declaredFunctions.find(calledFunc) == declaredFunctions.end()) {
        std::cerr << "ERREUR: Appel à la fonction '" << calledFunc << "' qui n'est pas déclarée !" << std::endl;
        hasErrors = true;
    } else {
        int expected = functionParamCount[calledFunc];
        int given = 0;
        if (ctx->arg_list()) {
            given = ctx->arg_list()->expr().size();
        }
        if (expected != given) {
            std::cerr << "ERREUR: Appel à la fonction '" << calledFunc << "' avec " << given
                      << " argument(s), mais " << expected << " attendu(s) !" << std::endl;
            hasErrors = true;
        }
    }
    // Visiter tous les arguments de l'appel de fonction
    if (ctx->arg_list()) {
        for (auto expr : ctx->arg_list()->expr()) {
            this->visit(expr);
        }
    }
    return 0;
}

// Visite d'une constante (rien à faire)
antlrcpp::Any SymbolTableVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
    return 0;
}

// Visite d'une addition ou soustraction (visite les deux opérandes)
antlrcpp::Any SymbolTableVisitor::visitAdditiveExpr(ifccParser::AdditiveExprContext *ctx) {
    this->visit(ctx->expr(0));
    this->visit(ctx->expr(1));
    return 0;
}

// Visite d'une multiplication, division ou modulo (visite les deux opérandes)
antlrcpp::Any SymbolTableVisitor::visitMultiplicativeExpr(ifccParser::MultiplicativeExprContext *ctx) {
    this->visit(ctx->expr(0));
    this->visit(ctx->expr(1));
    return 0;
}

// Visite d'une expression unaire (visite l'opérande)
antlrcpp::Any SymbolTableVisitor::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
    this->visit(ctx->expr());
    return 0;
}

// Visite d'une expression entre parenthèses
antlrcpp::Any SymbolTableVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx) {
    this->visit(ctx->expr());
    return 0;
}

// Visite d'un return (marque la fonction comme ayant un return)
antlrcpp::Any SymbolTableVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    std::cerr << "Traitement d'une instruction return dans la fonction '" << currentFunction << "'..." << std::endl;
    
    // Marquer que la fonction courante a un return
    functionsWithReturn.insert(currentFunction);
    std::cerr << "Fonction '" << currentFunction << "' marquée comme ayant un return" << std::endl;
    
    // Traiter l'expression si elle existe
    if (ctx->expr()) {
        std::cerr << "Return avec expression..." << std::endl;
        this->visit(ctx->expr());
    } else {
        std::cerr << "Return sans expression..." << std::endl;
    }

    return 0;
}

// Visite d'un ET binaire (&)
antlrcpp::Any SymbolTableVisitor::visitBitwiseAndExpr(ifccParser::BitwiseAndExprContext *ctx) {
    this->visit(ctx->expr(0));
    this->visit(ctx->expr(1));
    return 0;
}

// Visite d'un XOR binaire (^)
antlrcpp::Any SymbolTableVisitor::visitBitwiseXorExpr(ifccParser::BitwiseXorExprContext *ctx) {
    this->visit(ctx->expr(0));
    this->visit(ctx->expr(1));
    return 0;
}

// Visite d'un OU binaire (|)
antlrcpp::Any SymbolTableVisitor::visitBitwiseOrExpr(ifccParser::BitwiseOrExprContext *ctx) {
    this->visit(ctx->expr(0));
    this->visit(ctx->expr(1));
    return 0;
}

// Visite d'une déclaration globale (ajoute à la table des symboles globale)
antlrcpp::Any SymbolTableVisitor::visitGlobal_decl(ifccParser::Global_declContext *ctx)
{
    std::string varName = ctx->VAR()->getText();

    // Vérifier si la variable globale est déjà déclarée
    if (globalVars.find(varName) != globalVars.end())
    {
        std::cerr << "ERREUR: Variable globale '" << varName << "' déclarée plusieurs fois!" << std::endl;
        hasErrors = true;
        return 0;
    }

    // Ajouter la variable globale à la table des symboles
    // Pour les variables globales, on peut utiliser des offsets spéciaux ou une section .data
    symbolTable[varName] = 0; // Offset temporaire pour les variables globales
    globalVars.insert(varName);

    std::cerr << "Déclaration globale: Variable '" << varName << "' ajoutée à la table des symboles" << std::endl;

    // Si il y a une initialisation, visiter l'expression
    if (ctx->expr())
    {
        std::cerr << "Initialisation globale de '" << varName << "' avec expression..." << std::endl;
        this->visit(ctx->expr());
    }

    return 0;
}

// Visite d'un caractère (rien à faire, traité comme une constante)
antlrcpp::Any SymbolTableVisitor::visitCharExpr(ifccParser::CharExprContext *ctx) {
    return 0;
}

// Visite d'une instruction expression
antlrcpp::Any SymbolTableVisitor::visitExpr_stmt(ifccParser::Expr_stmtContext *ctx) {
    this->visit(ctx->expr());
    return 0;
}

// Visite d'une égalité (==, !=)
antlrcpp::Any SymbolTableVisitor::visitEqualityExpr(ifccParser::EqualityExprContext *ctx)
{
    this->visit(ctx->expr(0));
    this->visit(ctx->expr(1));
    return 0;
}

// Visite d'une comparaison (<, >, <=, >=)
antlrcpp::Any SymbolTableVisitor::visitRelationalExpr(ifccParser::RelationalExprContext *ctx)
{
    this->visit(ctx->expr(0));
    this->visit(ctx->expr(1));
    return 0;
}

// Visite d'une liste d'arguments d'appel de fonction
antlrcpp::Any SymbolTableVisitor::visitArg_list(ifccParser::Arg_listContext *ctx) {
    for (auto expr : ctx->expr()) {
        this->visit(expr);
    }
    return 0;
}

// Visite d'un ET logique paresseux (&&)
antlrcpp::Any SymbolTableVisitor::visitLogicalAndExpr(ifccParser::LogicalAndExprContext *ctx) {
    this->visit(ctx->expr(0));
    this->visit(ctx->expr(1));
    return 0;
}

// Visite d'un OU logique paresseux (||)
antlrcpp::Any SymbolTableVisitor::visitLogicalOrExpr(ifccParser::LogicalOrExprContext *ctx) {
    this->visit(ctx->expr(0));
    this->visit(ctx->expr(1));
    return 0;
}

// Visite d'un if/else (analyse la condition et les branches)
antlrcpp::Any SymbolTableVisitor::visitIf_stmt(ifccParser::If_stmtContext *ctx)
{
    this->visit(ctx->expr());
    this->visit(ctx->stmt(0));
    if (ctx->stmt(1))
    {
        this->visit(ctx->stmt(1));
    }
    return 0;
}

// Visite d'un bloc d'instructions
antlrcpp::Any SymbolTableVisitor::visitBlock_stmt(ifccParser::Block_stmtContext *ctx)
{
    for (auto stmt : ctx->stmt())
    {
        this->visit(stmt);
    }
    return 0;
}