void CFG::gen_asm_x86(ostream& o) {
    // Ne rien faire ici, la génération est gérée dans visitProg
}

void CFG::gen_asm_x6_prologue(ostream& o) {
    o << "\tpushq\t%rbp\n";
    o << "\tmovq\t%rsp, %rbp\n";
}

void CFG::gen_asm_x6_epilogue(ostream& o) {
    o << "\tmovq\t%rbp, %rsp\n";
    o << "\tpopq\t%rbp\n";
    o << "\tret\n";
}

void BasicBlock::gen_asm_x86(ostream& o) {
    // Ne rien faire ici, la génération est gérée dans visitProg
} 