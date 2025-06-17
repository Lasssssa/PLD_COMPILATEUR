#include "IR.h"

#include <sstream>

using std::endl;
using std::ostream;
using std::string;
using std::to_string;
using std::vector;

string IRInstr::IR_reg_to_asm(string reg)
{
    if (reg[0] == '%')
    {
        // If it's already a register, return it as is
        return reg;
    }
    // If it's a local variable (format "!X")
    if (reg[0] == '!')
    {
        int offset = stoi(reg.substr(1));
#ifdef ARM
        // Local variables are stored at positive offsets from sp
        // Ensure 4-byte alignment for 32-bit integers
        return to_string(4 * (offset + 1)); // +1 to account for return value
#else
        // Local variables are stored at negative offsets from %rbp
        return to_string(-4 * (offset + 1)) + "(%rbp)";
#endif
    }
    // For other cases, return the register as is
    return reg;
}

// Implémentation de IRInstr
IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t, vector<string> params)
    : bb(bb_), op(op), t(t), params(params) {}

void IRInstr::gen_asm_x86(ostream &o)
{
    switch (op)
    {
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
        o << "\tcltd\n"; // Sign extend eax into edx
        o << "\tidivl\t" << IR_reg_to_asm(params[2]) << "\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case rmem:
        if (params[1][0] == '%')
        {
            // Lire depuis un registre
            o << "\tmovl\t" << params[1] << ", " << IR_reg_to_asm(params[0]) << "\n";
        }
        else
        {
            // Lire depuis la mémoire
            o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        }
        break;
    case wmem:
        if (params[0] == params[1])
        {
            break;
        }
        if (params[1][0] == '%')
        {
            // Cas où le deuxième paramètre est un registre
            o << "\tmovl\t" << params[1] << ", " << IR_reg_to_asm(params[0]) << "\n";
        }
        else
        {
            o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        }
        break;
    case cmp_eq:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\tcmpl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "\tsete\t%al\n";
        o << "\tmovzbl\t%al, %eax\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case cmp_ne:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\tcmpl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "\tsetne\t%al\n";
        o << "\tmovzbl\t%al, %eax\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case cmp_lt:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\tcmpl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "\tsetl\t%al\n";
        o << "\tmovzbl\t%al, %eax\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case cmp_gt:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\tcmpl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "\tsetg\t%al\n";
        o << "\tmovzbl\t%al, %eax\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case cmp_le:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\tcmpl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "\tsetle\t%al\n";
        o << "\tmovzbl\t%al, %eax\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case cmp_ge:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\tcmpl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "\tsetge\t%al\n";
        o << "\tmovzbl\t%al, %eax\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case ret:
        o << "\tmovl\t" << IR_reg_to_asm(params[0]) << ", %eax\n";
        break;
    case call:
        // Sauvegarder les registres qui pourraient être modifiés
        o << "\tpushq\t%rax\n";
        o << "\tpushq\t%rcx\n";
        o << "\tpushq\t%rdx\n";
        o << "\tpushq\t%rsi\n";
        o << "\tpushq\t%rdi\n";
        o << "\tpushq\t%r8\n";
        o << "\tpushq\t%r9\n";
        o << "\tpushq\t%r10\n";
        o << "\tpushq\t%r11\n";

        // Passer les paramètres dans les registres (convention x86_64)
        // params[0] = nom de la fonction, params[1] = variable de retour
        // params[2..] = arguments
        for (size_t i = 2; i < params.size(); i++)
        {
            if (i == 2)
                o << "\tmovl\t" << IR_reg_to_asm(params[i]) << ", %edi\n";
            else if (i == 3)
                o << "\tmovl\t" << IR_reg_to_asm(params[i]) << ", %esi\n";
            else if (i == 4)
                o << "\tmovl\t" << IR_reg_to_asm(params[i]) << ", %edx\n";
            else if (i == 5)
                o << "\tmovl\t" << IR_reg_to_asm(params[i]) << ", %ecx\n";
            else if (i == 6)
                o << "\tmovl\t" << IR_reg_to_asm(params[i]) << ", %r8d\n";
            else if (i == 7)
                o << "\tmovl\t" << IR_reg_to_asm(params[i]) << ", %r9d\n";
            else
            {
                // Pour plus de 6 paramètres, les empiler sur la pile
                o << "\tpushq\t" << IR_reg_to_asm(params[i]) << "\n";
            }
        }

        // Appeler la fonction
        o << "\tcall\t" << params[0] << "\n";

        // Stocker le résultat
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[1]) << "\n";

        // Restaurer les registres
        o << "\tpopq\t%r11\n";
        o << "\tpopq\t%r10\n";
        o << "\tpopq\t%r9\n";
        o << "\tpopq\t%r8\n";
        o << "\tpopq\t%rdi\n";
        o << "\tpopq\t%rsi\n";
        o << "\tpopq\t%rdx\n";
        o << "\tpopq\t%rcx\n";
        o << "\tpopq\t%rax\n";
        break;
    }
}

void IRInstr::gen_asm_arm(ostream &o)
{
    switch (op)
    {
    case ldconst:
        o << "\tmov w0, #" << params[1] << "\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case add:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tadd w0, w0, w1\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case sub:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tsub w0, w0, w1\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case mul:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tmul w0, w0, w1\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case div:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tsdiv w0, w0, w1\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case rmem:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case wmem:
        if (params[0] == params[1])
        {
            break;
        }
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case ret:
        o << "\tmovl\t" << IR_reg_to_asm(params[0]) << ", %eax\n";
        break;
    }
}

BasicBlock::BasicBlock(CFG *cfg, string
                                     entry_label)
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr)
{
    cfg->add_bb(this);
}

void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, vector<string> params)
{
    IRInstr *instr = new IRInstr(this, op, t, params);
    instrs.push_back(instr);
}

void BasicBlock::gen_asm_x86(ostream &o)
{
    // Génère le label du bloc
    o << label << ":" << endl;

    // Génère le code pour chaque instruction
    for (IRInstr *instr : instrs)
    {
#ifdef ARM
        instr->gen_asm_arm(o);
#else
        instr->gen_asm_x86(o);
#endif
    }

    // Gestion des branches
    if (exit_true == nullptr)
    {
        // Fin de fonction - ne pas générer l'épilogue ici
        // L'épilogue sera généré séparément dans visitProg
    }
    else if (exit_false == nullptr)
    {
        // Branchement inconditionnel
        o << "\tjmp " << exit_true->label << endl;
    }
    else
    {
        // Branchement conditionnel
        o << "\tcmpl $0, %eax" << endl;
        o << "\tje " << exit_false->label << endl;
        o << "\tjmp " << exit_true->label << endl;
    }
}

// Implémentation de CFG
CFG::CFG(DefFonction *ast)
    : ast(ast), nextFreeSymbolIndex(0), nextBBnumber(0), current_bb(nullptr) {}

void CFG::add_bb(BasicBlock *bb)
{
    bbs.push_back(bb);
}

void CFG::gen_asm_x86(ostream &o)
{
    // Génère le prologue
    gen_asm_prologue(o);

    // Génère le code pour chaque bloc
    for (BasicBlock *bb : bbs)
    {
        bb->gen_asm_x86(o);
    }
}

static void gen_asm_arm_prologue(ostream &o, int nextFreeSymbolIndex)
{
    // Calculate total stack size needed (including alignment)
    int totalSize = 16 + (nextFreeSymbolIndex * 4); // 16 for frame + variables
    // Round up to 16-byte alignment
    totalSize = ((totalSize + 15) & ~15);

    o << "\tsub sp, sp, #" << totalSize << "\n"; // Allocate all stack space at once
}

static void gen_asm_x86_prologue(ostream &o, int nextFreeSymbolIndex)
{
    o << "\tpushq %rbp" << endl;
    o << "\tmovq %rsp, %rbp" << endl;
    o << "\tsubq $" << (nextFreeSymbolIndex * 4) << ", %rsp" << endl;
}

void CFG::gen_asm_prologue(ostream &o)
{
#ifdef ARM
    gen_asm_arm_prologue(o, nextFreeSymbolIndex);
#else
    gen_asm_x86_prologue(o, nextFreeSymbolIndex);
#endif
}

void CFG::gen_asm_epilogue(ostream &o)
{
#ifdef ARM
    // Calculate total stack size needed (including alignment)
    int totalSize = 16 + (nextFreeSymbolIndex * 4); // 16 for frame + variables
    // Round up to 16-byte alignment
    totalSize = ((totalSize + 15) & ~15);

    o << "\tadd sp, sp, #" << totalSize << "\n"; // Restore all stack space at once
    o << "\tret\n";
#else
    o << "\tleave" << endl;
    o << "\tret" << endl;
#endif
}

void CFG::add_to_symbol_table(string name, Type t)
{
    SymbolType[name] = t;
    SymbolIndex[name] = nextFreeSymbolIndex++;
}

string CFG::create_new_tempvar(Type t)
{
    string name = "!" + to_string(nextFreeSymbolIndex);
    add_to_symbol_table(name, t);
    return name;
}

int CFG::get_var_index(string name)
{
    return SymbolIndex[name];
}

Type CFG::get_var_type(string name)
{
    return SymbolType[name];
}

string CFG::new_BB_name()
{
    return "BB_" + to_string(nextBBnumber++);
}