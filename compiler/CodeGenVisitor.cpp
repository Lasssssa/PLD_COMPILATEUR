#include "CodeGenVisitor.h"

CodeGenVisitor::CodeGenVisitor(const std::map<std::string, int> &symbols) : symbolTable(symbols) {
}

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    // Prologue de la fonction main
#ifdef __APPLE__
    std::cout << ".globl _main\n";
    std::cout << "_main:\n";
#else
    std::cout << ".globl main\n";
    std::cout << "main:\n";
#endif

    // Setup du stack frame
    std::cout << "    pushq %rbp\n";
    std::cout << "    movq %rsp, %rbp\n";

    // Allouer l'espace pour les variables locales
    int maxNegOffset = 0;
    for (const auto &p: symbolTable) {
        if (p.second < maxNegOffset)
            maxNegOffset = p.second;
    }
    int stackSize = -maxNegOffset; // taille en octets (multiples de 4)
    if (stackSize > 0) {
        // Arrondir à 16 pour l'alignement
        int aligned = ((stackSize + 15) / 16) * 16;
        std::cout << "    subq $" << aligned << ", %rsp\n";
    }

    // Initialiser la valeur de retour à 0 (convention GCC)
    std::cout << "    movl $0, -4(%rbp)\n";

    // Visiter tous les statements (sans allocation explicite de pile)
    for (auto stmt: ctx->stmt()) {
        this->visit(stmt);
    }

    // Épilogue
    std::cout << "    movq %rbp, %rsp\n";
    std::cout << "    popq %rbp\n";
    std::cout << "    ret\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturn_stmt(ifccParser::Return_stmtContext *ctx) {
    // Évaluer l'expression de retour et placer le résultat dans %eax
    this->visit(ctx->expr());

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitExpr_stmt(ifccParser::Expr_stmtContext *ctx) {
    // Évaluer l'expression (peut avoir des effets de bord comme les affectations)
    this->visit(ctx->expr());

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDecl_stmt(ifccParser::Decl_stmtContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    int offset = symbolTable.at(varName);

    // Si il y a une initialisation
    if (ctx->expr()) {
        // Optimisation: si c'est une constante, l'écrire directement comme GCC
        if (auto constExpr = dynamic_cast<ifccParser::ConstExprContext *>(ctx->expr())) {
            int value = std::stoi(constExpr->CONST()->getText());
            std::cout << "    movl $" << value << ", " << offset << "(%rbp)\n";
        } else {
            // Sinon, évaluer l'expression normalement
            this->visit(ctx->expr());
            std::cout << "    movl %eax, " << offset << "(%rbp)\n";
        }
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
    std::string varName = ctx->VAR()->getText();
    int offset = symbolTable.at(varName);

    // Charger la valeur de la variable dans %eax
    std::cout << "    movl " << offset << "(%rbp), %eax\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
    int value = std::stoi(ctx->CONST()->getText());

    // Charger la constante dans %eax
    std::cout << "    movl $" << value << ", %eax\n";

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignExpr(ifccParser::AssignExprContext *ctx) {
    // Pour une affectation expr = expr
    // D'abord évaluer l'expression de droite (résultat dans %eax)
    this->visit(ctx->expr(1));

    // Le côté gauche doit être une variable
    if (auto varExpr = dynamic_cast<ifccParser::VarExprContext *>(ctx->expr(0))) {
        std::string varName = varExpr->VAR()->getText();
        int offset = symbolTable.at(varName);

        // Stocker directement %eax dans la variable (plus efficace)
        std::cout << "    movl %eax, " << offset << "(%rbp)\n";

        // Laisser la valeur assignée dans %eax pour chaînage éventuel
    } else {
        std::cerr << "ERREUR: Le côté gauche d'une affectation doit être une variable" << std::endl;
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitParensExpr(ifccParser::ParensExprContext *ctx) {
    // Simplement évaluer l'expression entre parenthèses
    this->visit(ctx->expr());

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitAdditiveExpr(ifccParser::AdditiveExprContext *ctx) {
    // Évaluer l'expression de gauche
    this->visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";

    // Évaluer l'expression de droite
    this->visit(ctx->expr(1));

    // récupérer la valeur de gauche
    std::cout << "    popq %rcx\n";

    std::string op = ctx->children[1]->getText();
    if (op == "+") {
        // a + b  ->  %ecx + %eax
        std::cout << "    addl %ecx, %eax\n";
    } else if (op == "-") {
        // a - b
        std::cout << "    subl %eax, %ecx\n";
        std::cout << "    movl %ecx, %eax\n";
    }

    return 0;
}

// Nettoyage de visitMultiplicativeExpr (plus de modulo)
antlrcpp::Any CodeGenVisitor::visitMultiplicativeExpr(ifccParser::MultiplicativeExprContext *ctx) {
    // Évaluer l'expression de gauche
    this->visit(ctx->expr(0));
    std::cout << "    pushq %rax\n";

    // Évaluer l'expression de droite
    this->visit(ctx->expr(1));

    // Récupérer la valeur de gauche
    std::cout << "    popq %rcx\n";

    std::string op = ctx->children[1]->getText();
    if (op == "*") {
        std::cout << "    imull %ecx, %eax\n";
    } else if (op == "/") {
        // a / b  ->  %ecx / %eax
        std::cout << "    movl %eax, %ebx\n"; // diviseur dans %ebx
        std::cout << "    movl %ecx, %eax\n"; // dividend dans %eax
        std::cout << "    cdq\n";             // signe
        std::cout << "    idivl %ebx\n";      // résultat dans %eax
    }

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
    // Évaluer l'expression interne
    this->visit(ctx->expr());

    std::string op = ctx->children[0]->getText();
    if (op == "-") {
        // Négation arithmétique : %eax = -%eax
        std::cout << "    negl %eax\n";
    }
    // Pour le plus unaire, on ne fait rien

    return 0;
}
