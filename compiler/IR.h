#ifndef IR_H
#define IR_H

#include <vector>
#include <string>
#include <iostream>
#include <map>
#include <initializer_list>

// Déclaration des namespaces
using std::map;
using std::ostream;
using std::string;
using std::vector;

// Forward declarations
class BasicBlock;
class CFG;
class DefFonction;

// Declarations from the parser -- replace with your own
#include "type.h"
#include "symbole.h"

// Classe représentant une instruction IR (3-adresses)
class IRInstr
{

public:
    /** Enumération des opérations IR supportées */
    enum Operation
    {
        ldconst,      // Chargement d'une constante
        add,          // Addition
        sub,          // Soustraction
        mul,          // Multiplication
        div,          // Division
        mod,          // Modulo
        rmem,         // Lecture mémoire
        wmem,         // Écriture mémoire
        call,         // Appel de fonction
        cmp_eq,       // Comparaison ==
        cmp_ne,       // Comparaison !=
        cmp_lt,       // Comparaison <
        cmp_gt,       // Comparaison >
        cmp_le,       // Comparaison <=
        cmp_ge,       // Comparaison >=
        not_op,       // Négation logique (!)
        bit_and,      // ET binaire
        bit_xor,      // XOR binaire
        bit_or,       // OU binaire
        logical_and,  // ET logique paresseux (&&)
        logical_or,   // OU logique paresseux (||)
        ret           // Retour de fonction
    };

    /** Constructeur (voir les paramètres dans les attributs) */
    IRInstr(BasicBlock *bb_, Operation op, Type t, vector<string> params);

    /** Génération du code assembleur x86 pour cette instruction IR */
    void gen_asm_x86(ostream &o);
    /** Génération du code assembleur ARM pour cette instruction IR */
    void gen_asm_arm(ostream &o);

protected:
    // Convertit un registre IR ou une variable en format assembleur
    string IR_reg_to_asm(string reg);

private:
    BasicBlock *bb; // BasicBlock auquel appartient cette instruction
    Operation op;   // Opération IR
    Type t;         // Type de l'opération
    vector<string> params; // Paramètres de l'instruction (voir commentaires)
};

/**  The class for a basic block */

/* A few important comments.
     IRInstr has no jump instructions.
     cmp_* instructions behaves as an arithmetic two-operand instruction (add or mult),
      returning a boolean value (as an int)

     Assembly jumps are generated as follows:
     BasicBlock::gen_asm() first calls IRInstr::gen_asm() on all its instructions, and then
            if  exit_true  is a  nullptr,
            the epilogue is generated
        else if exit_false is a nullptr,
          an unconditional jmp to the exit_true branch is generated
                else (we have two successors, hence a branch.
                      But then we know the last code generated in this BB was the code for the test
                            and its boolean result is in !reg)
          an instruction comparing the value of !reg to true is generated,
                    followed by a conditional branch to the exit_false branch,
                    followed by an unconditional branch to the exit_true branch

Possible optimization:
     Add a cmp_* comparison instructions.
           If it is the last instruction of its block,
       it generates an actual assembly comparison
       followed by a conditional jump to the exit_false branch

*/

class BasicBlock
{
public:
    BasicBlock(CFG *cfg, string entry_label);

    void gen_asm(ostream &o); /**< x86 assembly code generation for this basic block (very simple) */

    void add_IRInstr(IRInstr::Operation op, Type t, vector<string> params);

    // No encapsulation whatsoever here. Feel free to do better.
    BasicBlock *exit_true;    /**< pointer to the next basic block, true branch. If nullptr, return from procedure */
    BasicBlock *exit_false;   /**< pointer to the next basic block, false branch. If null_ptr, the basic block ends with an unconditional jump */
    string label;             /**< label of the BB, also will be the label in the generated code */
    CFG *cfg;                 /** < the CFG where this block belongs */
    vector<IRInstr *> instrs; /** < the instructions themselves. */

    // Méthodes pour accéder aux instructions
    const vector<IRInstr *> &get_instrs() const { return instrs; }
    const string &get_name() const { return label; }

protected:
    string name;
};

/** The class for the control flow graph, also includes the symbol table */

/* A few important comments:
     The entry block is the one with the same label as the AST function name.
       (it could be the first of bbs, or it could be defined by an attribute value)
     The exit block is the one with both exit pointers equal to nullptr.
     (again it could be identified in a more explicit way)

 */
class CFG
{
public:
    /* All this was obviously written in a time when we had an explicit AST data structure:
         to be adapted to ANTLR */
    CFG(DefFonction *ast);

    DefFonction *ast; /**< The AST this CFG comes from */

    void add_bb(BasicBlock *bb);


    void gen_asm_prologue(ostream &o);
    void gen_asm_epilogue(ostream &o);

    // symbol table methods
    void add_to_symbol_table(string name, Type t);
    string create_new_tempvar(Type t);
    int get_var_index(string name);
    Type get_var_type(string name);

    // basic block management
    string new_BB_name();
    BasicBlock *current_bb;

    // Ajouter cette méthode pour accéder aux blocs de base
    const vector<BasicBlock *> &get_bbs() const { return bbs; }

    // Add this method to access the symbol count
    int get_symbol_count() const { return nextFreeSymbolIndex; }

protected:
    map<string, Type> SymbolType; /**< part of the symbol table  */
    map<string, int> SymbolIndex; /**< part of the symbol table  */
    int nextFreeSymbolIndex;      /**< to allocate new symbols in the symbol table */
    int nextBBnumber;             /**< just for naming */

    vector<BasicBlock *> bbs; /**< all the basic blocks of this CFG*/
};

#endif