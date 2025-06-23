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

// Helper functions for codegen
namespace x86_codegen {
    constexpr const char* ARG_REGS[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    constexpr const char* CALLER_SAVED[] = {"%rax", "%rcx", "%rdx", "%rsi", "%rdi", "%r8", "%r9", "%r10", "%r11"};

    void mov_mem_to_reg(ostream &o, const string &mem, const string &reg) {
        o << "\tmovl\t" << mem << ", " << reg << "\n";
    }
    void mov_reg_to_mem(ostream &o, const string &reg, const string &mem) {
        o << "\tmovl\t" << reg << ", " << mem << "\n";
    }
    void mov_reg_to_reg(ostream &o, const string &src, const string &dst) {
        o << "\tmovl\t" << src << ", " << dst << "\n";
    }
    void push_regs(ostream &o, const char* const regs[], size_t n) {
        for (size_t i = 0; i < n; ++i) o << "\tpushq\t" << regs[i] << "\n";
    }
    void pop_regs_reverse(ostream &o, const char* const regs[], size_t n) {
        for (size_t i = n; i-- > 0;) o << "\tpopq\t" << regs[i] << "\n";
    }
    void emit_cmp_set(ostream &o, const string &lhs, const string &rhs, const char* set_instr, const string &dst) {
        mov_mem_to_reg(o, lhs, "%eax");
        o << "\tcmpl\t" << rhs << ", %eax\n";
        o << "\t" << set_instr << "\t%al\n";
        o << "\tmovzbl\t%al, %eax\n";
        mov_reg_to_mem(o, "%eax", dst);
    }
}

void IRInstr::gen_asm_x86(ostream &o)
{
    switch (op)
    {
    case ldconst:
        o << "\tmovl\t$" << params[1] << ", %eax\n";
        x86_codegen::mov_reg_to_mem(o, "%eax", IR_reg_to_asm(params[0]));
        break;
    case add:
        x86_codegen::mov_mem_to_reg(o, IR_reg_to_asm(params[1]), "%eax");
        o << "\taddl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        x86_codegen::mov_reg_to_mem(o, "%eax", IR_reg_to_asm(params[0]));
        break;
    case sub:
        x86_codegen::mov_mem_to_reg(o, IR_reg_to_asm(params[1]), "%eax");
        o << "\tsubl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        x86_codegen::mov_reg_to_mem(o, "%eax", IR_reg_to_asm(params[0]));
        break;
    case mul:
        x86_codegen::mov_mem_to_reg(o, IR_reg_to_asm(params[1]), "%eax");
        o << "\timull\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        x86_codegen::mov_reg_to_mem(o, "%eax", IR_reg_to_asm(params[0]));
        break;
    case div:
        x86_codegen::mov_mem_to_reg(o, IR_reg_to_asm(params[1]), "%eax");
        o << "\tcltd\n";
        o << "\tidivl\t" << IR_reg_to_asm(params[2]) << "\n";
        x86_codegen::mov_reg_to_mem(o, "%eax", IR_reg_to_asm(params[0]));
        break;
    case rmem:
        if (params[1][0] == '%')
            x86_codegen::mov_reg_to_mem(o, params[1], IR_reg_to_asm(params[0]));
        else {
            x86_codegen::mov_mem_to_reg(o, IR_reg_to_asm(params[1]), "%eax");
            x86_codegen::mov_reg_to_mem(o, "%eax", IR_reg_to_asm(params[0]));
        }
        break;
    case wmem:
        if (params[0] == params[1]) break;
        if (params[1][0] == '%')
            x86_codegen::mov_reg_to_mem(o, params[1], IR_reg_to_asm(params[0]));
        else {
            x86_codegen::mov_mem_to_reg(o, IR_reg_to_asm(params[1]), "%eax");
            x86_codegen::mov_reg_to_mem(o, "%eax", IR_reg_to_asm(params[0]));
        }
        break;
    case cmp_eq:
        x86_codegen::emit_cmp_set(o, IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]), "sete", IR_reg_to_asm(params[0]));
        break;
    case cmp_ne:
        x86_codegen::emit_cmp_set(o, IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]), "setne", IR_reg_to_asm(params[0]));
        break;
    case cmp_lt:
        x86_codegen::emit_cmp_set(o, IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]), "setl", IR_reg_to_asm(params[0]));
        break;
    case cmp_gt:
        x86_codegen::emit_cmp_set(o, IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]), "setg", IR_reg_to_asm(params[0]));
        break;
    case cmp_le:
        x86_codegen::emit_cmp_set(o, IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]), "setle", IR_reg_to_asm(params[0]));
        break;
    case cmp_ge:
        x86_codegen::emit_cmp_set(o, IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]), "setge", IR_reg_to_asm(params[0]));
        break;
    case ret:
        x86_codegen::mov_mem_to_reg(o, IR_reg_to_asm(params[0]), "%eax");
        o << "\tleave\n";
        o << "\tret\n";
        break;
    case call:
        x86_codegen::push_regs(o, x86_codegen::CALLER_SAVED, sizeof(x86_codegen::CALLER_SAVED)/sizeof(x86_codegen::CALLER_SAVED[0]));
        for (size_t i = 2; i < params.size(); i++) {
            if (i - 2 < sizeof(x86_codegen::ARG_REGS)/sizeof(x86_codegen::ARG_REGS[0]))
                x86_codegen::mov_mem_to_reg(o, IR_reg_to_asm(params[i]), x86_codegen::ARG_REGS[i-2]);
            else
                o << "\tpushq\t" << IR_reg_to_asm(params[i]) << "\n";
        }
        o << "\tcall\t" << params[0] << "\n";
        x86_codegen::mov_reg_to_mem(o, "%eax", IR_reg_to_asm(params[1]));
        x86_codegen::pop_regs_reverse(o, x86_codegen::CALLER_SAVED, sizeof(x86_codegen::CALLER_SAVED)/sizeof(x86_codegen::CALLER_SAVED[0]));
        break;
    }
}

// ARM codegen helpers
namespace arm_codegen {
    constexpr const char* ARM_ARG_REGS[] = {"w0", "w1", "w2", "w3", "w4", "w5"};
    constexpr const char* ARM_TMP_REGS[] = {"w8", "w9"};

    void arm_load_stack_to_reg(ostream &o, const string &reg, const string &slot) {
        o << "\tldr\t" << reg << ", [sp, #" << slot << "]\n";
    }
    void arm_store_reg_to_stack(ostream &o, const string &reg, const string &slot) {
        o << "\tstr\t" << reg << ", [sp, #" << slot << "]\n";
    }
    void arm_binop(ostream &o, const char* op, const string &dst, const string &lhs, const string &rhs) {
        arm_load_stack_to_reg(o, ARM_TMP_REGS[0], lhs);
        arm_load_stack_to_reg(o, ARM_TMP_REGS[1], rhs);
        o << "\t" << op << "\t" << ARM_TMP_REGS[0] << ", " << ARM_TMP_REGS[0] << ", " << ARM_TMP_REGS[1] << "\n";
        arm_store_reg_to_stack(o, ARM_TMP_REGS[0], dst);
    }
    void arm_cmp_set(ostream &o, const string &cond, const string &dst, const string &lhs, const string &rhs) {
        arm_load_stack_to_reg(o, ARM_TMP_REGS[0], lhs);
        arm_load_stack_to_reg(o, ARM_TMP_REGS[1], rhs);
        o << "\tcmp\t" << ARM_TMP_REGS[0] << ", " << ARM_TMP_REGS[1] << "\n";
        o << "\tcset\t" << ARM_TMP_REGS[0] << ", " << cond << "\n";
        arm_store_reg_to_stack(o, ARM_TMP_REGS[0], dst);
    }
    void arm_pass_args(ostream &o, const vector<string> &params, size_t start) {
        for (size_t i = start; i < params.size(); ++i) {
            if (i - start < sizeof(ARM_ARG_REGS)/sizeof(ARM_ARG_REGS[0]))
                arm_load_stack_to_reg(o, ARM_ARG_REGS[i-start], IRInstr::IR_reg_to_asm(params[i]));
            else {
                arm_load_stack_to_reg(o, ARM_TMP_REGS[0], IRInstr::IR_reg_to_asm(params[i]));
                o << "\tstr\t" << ARM_TMP_REGS[0] << ", [sp, #-16]!\n";
            }
        }
    }
    void arm_mov_const(ostream &o, int value, const string &reg) {
        if (value >= 0 && value <= 65535) {
            o << "\tmov\t" << reg << ", #" << value << "\n";
        } else {
            int low16 = value & 0xFFFF;
            int high16 = (value >> 16) & 0xFFFF;
            o << "\tmov\t" << reg << ", #" << low16 << "\n";
            if (high16 != 0) {
                o << "\tmovk\t" << reg << ", #" << high16 << ", lsl #16\n";
            }
        }
    }
}

void IRInstr::gen_asm_arm(ostream &o)
{
    switch (op)
    {
    case ldconst: {
        int value = std::stoi(params[1]);
        arm_codegen::arm_mov_const(o, value, arm_codegen::ARM_TMP_REGS[0]);
        arm_codegen::arm_store_reg_to_stack(o, arm_codegen::ARM_TMP_REGS[0], IR_reg_to_asm(params[0]));
        break;
    }
    case add:
        arm_codegen::arm_binop(o, "add", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case sub:
        arm_codegen::arm_binop(o, "sub", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case mul:
        arm_codegen::arm_binop(o, "mul", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case div:
        arm_codegen::arm_binop(o, "sdiv", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case rmem:
        if (params[0].size() == 2 && params[0][0] == 'w' && isdigit(params[0][1])) {
            arm_codegen::arm_load_stack_to_reg(o, params[0], IR_reg_to_asm(params[1]));
        } else if (params[1].size() == 2 && params[1][0] == 'w' && isdigit(params[1][1])) {
            arm_codegen::arm_store_reg_to_stack(o, params[1], IR_reg_to_asm(params[0]));
        } else {
            arm_codegen::arm_load_stack_to_reg(o, arm_codegen::ARM_TMP_REGS[0], IR_reg_to_asm(params[1]));
            arm_codegen::arm_store_reg_to_stack(o, arm_codegen::ARM_TMP_REGS[0], IR_reg_to_asm(params[0]));
        }
        break;
    case wmem:
        if (params[1].size() == 2 && params[1][0] == 'w' && isdigit(params[1][1])) {
            arm_codegen::arm_store_reg_to_stack(o, params[1], IR_reg_to_asm(params[0]));
        } else if (params[0].size() == 2 && params[0][0] == 'w' && isdigit(params[0][1])) {
            arm_codegen::arm_load_stack_to_reg(o, params[0], IR_reg_to_asm(params[1]));
        } else {
            arm_codegen::arm_load_stack_to_reg(o, arm_codegen::ARM_TMP_REGS[0], IR_reg_to_asm(params[1]));
            arm_codegen::arm_store_reg_to_stack(o, arm_codegen::ARM_TMP_REGS[0], IR_reg_to_asm(params[0]));
        }
        break;
    case cmp_eq:
        arm_codegen::arm_cmp_set(o, "eq", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case cmp_ne:
        arm_codegen::arm_cmp_set(o, "ne", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case cmp_lt:
        arm_codegen::arm_cmp_set(o, "lt", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case cmp_gt:
        arm_codegen::arm_cmp_set(o, "gt", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case cmp_le:
        arm_codegen::arm_cmp_set(o, "le", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case cmp_ge:
        arm_codegen::arm_cmp_set(o, "ge", IR_reg_to_asm(params[0]), IR_reg_to_asm(params[1]), IR_reg_to_asm(params[2]));
        break;
    case ret: {
        arm_codegen::arm_load_stack_to_reg(o, "w0", IR_reg_to_asm(params[0]));
        int stack_size = ((bb->cfg->get_symbol_count() + 1) * 4 + 15) & ~15;
        if (stack_size > 0) {
            o << "\tadd\tsp, sp, #" << stack_size << "\n";
        }
        o << "\tldp\tx29, x30, [sp], #16\n";
        o << "\tret\n";
        break;
    }
    case call:
        arm_codegen::arm_pass_args(o, params, 2);
        o << "\tbl\t" << params[0] << "\n";
        arm_codegen::arm_store_reg_to_stack(o, "w0", IR_reg_to_asm(params[1]));
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
        instrs[i]->gen_asm_x86(o);
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