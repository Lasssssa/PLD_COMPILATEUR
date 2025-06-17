#include "IR.h"
#include <sstream>

using std::endl;
using std::to_string;

string IR_reg_to_asm(string reg) {
    if (reg[0] == '%') {
        // Si c'est déjà un registre assembleur, le retourner tel quel
        return reg;
    }
    // Si c'est une variable locale (format "!X")
    if (reg[0] == '!') {
        int offset = stoi(reg.substr(1));
        // Les variables locales sont stockées à des offsets négatifs par rapport à %rbp
        return to_string(-8 * offset) + "(%rbp)";
    }
    // Pour les autres cas, retourner le registre tel quel
    return reg;
}

// Implémentation de IRInstr
IRInstr::IRInstr(BasicBlock* bb_, Operation op, Type t, vector<string> params) 
    : bb(bb_), op(op), t(t), params(params) {}

void IRInstr::gen_asm_x86(ostream &o) {
    switch (op) {
        case ldconst:
            o << "\tmovl\t$" << params[1] << ", %eax\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
            break;
        case add:
            o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "\taddl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
            break;
        case sub:
            o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "\tsubl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
            break;
        case mul:
            o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "\timull\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
            break;
        case div:
            o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "\tcltd\n";  // Sign extend eax into edx
            o << "\tidivl\t" << IR_reg_to_asm(params[2]) << "\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
            break;
        case rmem:
            o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
            break;
        case wmem:
            o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
            break;
        case ret:
            o << "\tmovl\t" << IR_reg_to_asm(params[0]) << ", %eax\n";
            break;
    }
}

// Implémentation de BasicBlock
BasicBlock::BasicBlock(CFG* cfg, string entry_label) 
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr) {
    cfg->add_bb(this);
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, vector<string> params) {
    IRInstr* instr = new IRInstr(this, op, t, params);
    instrs.push_back(instr);
}

void BasicBlock::gen_asm_x86(ostream &o) {
    // Génère le label du bloc
    o << label << ":" << endl;
    
    // Génère le code pour chaque instruction
    for (IRInstr* instr : instrs) {
        instr->gen_asm_x86(o);
    }
    
    // Gestion des branches
    if (exit_true == nullptr) {
        // Fin de fonction
        cfg->gen_asm_x6_epilogue(o);
    } else if (exit_false == nullptr) {
        // Branchement inconditionnel
        o << "\tjmp " << exit_true->label << endl;
    } else {
        // Branchement conditionnel
        o << "\tcmpl $0, %eax" << endl;
        o << "\tje " << exit_false->label << endl;
        o << "\tjmp " << exit_true->label << endl;
    }
}

// Implémentation de CFG
CFG::CFG(DefFonction* ast) 
    : ast(ast), nextFreeSymbolIndex(0), nextBBnumber(0), current_bb(nullptr) {}

void CFG::add_bb(BasicBlock* bb) {
    bbs.push_back(bb);
}

void CFG::gen_asm_x86(ostream& o) {
    // Génère le prologue
    gen_asm_x6_prologue(o);
    
    // Génère le code pour chaque bloc
    for (BasicBlock* bb : bbs) {
        bb->gen_asm_x86(o);
    }
}

void CFG::gen_asm_x6_prologue(ostream& o) {
    o << "\tpushq %rbp" << endl;
    o << "\tmovq %rsp, %rbp" << endl;
    o << "\tsubq $" << (nextFreeSymbolIndex * 4) << ", %rsp" << endl;
}

void CFG::gen_asm_x6_epilogue(ostream& o) {
    o << "\tleave" << endl;
    o << "\tret" << endl;
}

void CFG::add_to_symbol_table(string name, Type t) {
    SymbolType[name] = t;
    SymbolIndex[name] = nextFreeSymbolIndex++;
}

string CFG::create_new_tempvar(Type t) {
    string name = "!" + to_string(nextFreeSymbolIndex);
    add_to_symbol_table(name, t);
    return name;
}

int CFG::get_var_index(string name) {
    return SymbolIndex[name];
}

Type CFG::get_var_type(string name) {
    return SymbolType[name];
}

string CFG::new_BB_name() {
    return "BB_" + to_string(nextBBnumber++);
} 