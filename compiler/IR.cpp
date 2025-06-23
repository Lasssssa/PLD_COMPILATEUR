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
        if (params[0][0] == 'w' && isdigit(params[0][1])) {
            // Destination is a register: ldr wN, [sp, #offset]
            o << "\tldr\t" << params[0] << ", [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        } else {
            // Destination is a stack slot: ldr w8, [sp, #offset_src]; str w8, [sp, #offset_dst]
            o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
            o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        }
        break;
    case wmem:
        if (params[1][0] == 'w' && isdigit(params[1][1])) {
            // Source is a register: str wN, [sp, #offset]
            o << "\tstr\t" << params[1] << ", [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        } else {
            // Source is a stack slot: ldr w8, [sp, #offset_src]; str w8, [sp, #offset_dst]
            o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
            o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
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
    case ret: {
        o << "\tmovl\t" << IR_reg_to_asm(params[0]) << ", %eax\n";
        o << "\tleave\n";
        o << "\tret\n";
        break;
    }
    case call:
        // Pass parameters in registers (ARM64 calling convention)
        // params[0] = function name, params[1] = return variable
        // params[2..] = arguments
        for (size_t i = 2; i < params.size(); i++)
        {
            if (i == 2)
                o << "\tldr\tw0, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 3)
                o << "\tldr\tw1, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 4)
                o << "\tldr\tw2, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 5)
                o << "\tldr\tw3, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 6)
                o << "\tldr\tw4, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 7)
                o << "\tldr\tw5, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else
            {
                // For more than 6 parameters, push them onto the stack
                o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
                o << "\tstr\tw8, [sp, #-16]!\n";
            }
        }

        // Call the function
        o << "\tbl\t" << params[0] << "\n";

        // Store the result
        o << "\tstr\tw0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        break;
    }
}

void IRInstr::gen_asm_arm(ostream &o)
{
    switch (op)
    {
    case ldconst: {
        int value = std::stoi(params[1]);
        if (value >= 0 && value <= 65535) {
            o << "\tmov\tw8, #" << value << "\n";
        } else {
            int low16 = value & 0xFFFF;
            int high16 = (value >> 16) & 0xFFFF;
            o << "\tmov\tw8, #" << low16 << "\n";
            if (high16 != 0) {
                o << "\tmovk\tw8, #" << high16 << ", lsl #16\n";
            }
        }
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    }
    case add:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tadd\tw8, w8, w9\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case sub:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tsub\tw8, w8, w9\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case mul:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tmul\tw8, w8, w9\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case div:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tsdiv\tw8, w8, w9\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case rmem:
        if (params[0].size() == 2 && params[0][0] == 'w' && isdigit(params[0][1])) {
            // rmem: destination is a register, source is a stack slot
            o << "\tldr\t" << params[0] << ", [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        } else if (params[1].size() == 2 && params[1][0] == 'w' && isdigit(params[1][1])) {
            // rmem: source is a register, destination is a stack slot
            o << "\tstr\t" << params[1] << ", [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        } else {
            // Default: stack to stack (via w8)
            o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
            o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        }
        break;
    case wmem:
        if (params[1].size() == 2 && params[1][0] == 'w' && isdigit(params[1][1])) {
            // wmem: source is a register, destination is a stack slot
            o << "\tstr\t" << params[1] << ", [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        } else if (params[0].size() == 2 && params[0][0] == 'w' && isdigit(params[0][1])) {
            // wmem: destination is a register, source is a stack slot
            o << "\tldr\t" << params[0] << ", [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        } else {
            // Default: stack to stack (via w8)
            o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
            o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        }
        break;
    case cmp_eq:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp\tw8, w9\n";
        o << "\tcset\tw8, eq\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_ne:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp\tw8, w9\n";
        o << "\tcset\tw8, ne\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_lt:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp\tw8, w9\n";
        o << "\tcset\tw8, lt\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_gt:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp\tw8, w9\n";
        o << "\tcset\tw8, gt\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_le:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp\tw8, w9\n";
        o << "\tcset\tw8, le\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_ge:
        o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr\tw9, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp\tw8, w9\n";
        o << "\tcset\tw8, ge\n";
        o << "\tstr\tw8, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case ret: {
        o << "\tldr\tw0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        int stack_size = ((bb->cfg->get_symbol_count() + 1) * 4 + 15) & ~15;
        if (stack_size > 0) {
            o << "\tadd\tsp, sp, #" << stack_size << "\n";
        }
        o << "\tldp\tx29, x30, [sp], #16\n";
        o << "\tret\n";
        break;
    }
    case call:
        // Pass parameters in registers (ARM64 calling convention)
        // params[0] = function name, params[1] = return variable
        // params[2..] = arguments
        for (size_t i = 2; i < params.size(); i++)
        {
            if (i == 2)
                o << "\tldr\tw0, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 3)
                o << "\tldr\tw1, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 4)
                o << "\tldr\tw2, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 5)
                o << "\tldr\tw3, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 6)
                o << "\tldr\tw4, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else if (i == 7)
                o << "\tldr\tw5, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
            else
            {
                // For more than 6 parameters, push them onto the stack
                o << "\tldr\tw8, [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
                o << "\tstr\tw8, [sp, #-16]!\n";
            }
        }

        // Call the function
        o << "\tbl\t" << params[0] << "\n";

        // Store the result
        o << "\tstr\tw0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
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
    o << label << ":" << endl;
    for (size_t i = 0; i < instrs.size(); ++i)
    {
#ifdef ARM
        instrs[i]->gen_asm_arm(o);
#else
        instrs[i]->gen_asm_x86(o);
#endif
        if (instrs[i]->op == IRInstr::ret) return;
    }
    if (exit_true == nullptr) {
        // No epilogue here; ret handles it
    } else if (exit_false == nullptr) {
        o << "\tjmp " << exit_true->label << endl;
    } else {
        o << "\tcmpl $0, %eax" << endl;
        o << "\tje " << exit_false->label << endl;
        o << "\tjmp " << exit_true->label << endl;
    }
}

void BasicBlock::gen_asm_arm(ostream &o)
{
    o << label << ":\n";
    for (size_t i = 0; i < instrs.size(); ++i)
    {
        instrs[i]->gen_asm_arm(o);
        if (instrs[i]->op == IRInstr::ret) return;
    }
    if (exit_true == nullptr) {
        // No epilogue here; ret handles it
    } else if (exit_false == nullptr) {
        o << "\tb\t" << exit_true->get_label() << "\n";
    } else {
        o << "\tldr\tw8, [sp, #" << IRInstr::IR_reg_to_asm(test_var) << "]\n";
        o << "\tcbz\tw8, " << exit_false->get_label() << "\n";
        o << "\tb\t" << exit_true->get_label() << "\n";
    }
}

string BasicBlock::get_label()
{
    return label;
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
    // Save frame pointer and link register
    o << "\tstp\tx29, x30, [sp, #-16]!\n";
    o << "\tmov\tx29, sp\n";
    // Allocate stack space for local variables (aligned to 16 bytes)
    int stack_size = ((nextFreeSymbolIndex + 1) * 4 + 15) & ~15;
    if (stack_size > 0)
    {
        o << "\tsub\tsp, sp, #" << stack_size << "\n";
    }
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
    // No-op: epilogue is now handled by ret IRInstr
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

static void gen_asm_arm_epilogue(ostream &o, int nextFreeSymbolIndex)
{
    // Calculate stack size for deallocation
    int stack_size = ((nextFreeSymbolIndex + 1) * 4 + 15) & ~15;
    if (stack_size > 0)
    {
        o << "\tadd\tsp, sp, #" << stack_size << "\n";
    }
    
    // Restore frame pointer and link register
    o << "\tldp\tx29, x30, [sp], #16\n";
}