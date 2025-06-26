// IR.CPP : Implémentation des instructions IR, BasicBlock et CFG
// Ce fichier fait partie du middle-end et du back-end du compilateur.
// Il définit :
//   - L'IR (Intermediate Representation) : instructions de type 3-adresses, indépendantes de l'architecture cible
//   - Le CFG (Control Flow Graph) : graphe de BasicBlocks, modélisant le flot d'exécution réel du programme
//   - La génération de code assembleur à partir de l'IR et du CFG
// Le flow global :
//   1. VisitorIR génère l'IR et le CFG à partir de l'AST
//   2. Chaque fonction possède son propre CFG (un graphe de BasicBlocks)
//   3. Chaque BasicBlock contient une séquence d'instructions IR
//   4. Le CFG est parcouru pour générer le code assembleur, bloc par bloc, dans l'ordre d'exécution
// Ce découpage permet d'isoler la logique du langage source, de préparer des optimisations, et de faciliter le reciblage assembleur.

#include "IR.h"
#include <set>

using std::endl;
using std::ostream;
using std::string;
using std::to_string;
using std::vector;

// Liste des fonctions externes supportées (pour la gestion des appels externes)
static const std::set<std::string> externalFunctions = {"putchar", "getchar"};

// Convertit un registre IR ou une variable en format assembleur (x86/ARM)
string IRInstr::IR_reg_to_asm(string reg)
{
    if (reg[0] == '%')
    {
        // Si c'est déjà un registre, on le retourne tel quel
        return reg;
    }
    // Si c'est une variable locale (format !X)
    if (reg[0] == '!')
    {
        int offset = stoi(reg.substr(1));
#ifdef ARM
        // Convention ARM64/AArch64 :
        // Les variables locales sont stockées à des offsets positifs depuis sp, après 16 octets pour x29/x30
        // 8 octets par variable, alignement 16 octets pour respecter l'ABI
        // Exemple : la première variable locale est à [sp, #16], la suivante à [sp, #24], etc.
        return to_string(16 + 8 * offset);
#else
        // x86 : variables locales à offset négatif depuis %rbp
        return to_string(-4 * (offset + 1)) + "(%rbp)";
#endif
    }
    // Autres cas : retourne tel quel
    return reg;
}

// Classe IRInstr : représente une instruction IR de type 3-adresses
// Chaque instruction IR est indépendante de l'architecture cible et peut être traduite en assembleur x86 ou ARM
// Les instructions IR sont ajoutées dans les BasicBlocks du CFG
IRInstr::IRInstr(BasicBlock *bb_, Operation op, Type t, vector<string> params)
    : bb(bb_), op(op), t(t), params(params) {}

// Génère le code assembleur x86 pour cette instruction IR
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
    case mod:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\tcltd\n"; // Sign extend eax into edx
        o << "\tidivl\t" << IR_reg_to_asm(params[2]) << "\n";
        o << "\tmovl\t%edx, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case rmem:
        if (params[1][0] == '%')
        {
            // Lire depuis un registre
            o << "\tmovl\t" << params[1] << ", %eax\n";
            o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
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
    case not_op:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\tcmpl\t$0, %eax\n";
        o << "\tsete\t%al\n";
        o << "\tmovzbl\t%al, %eax\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case bit_and:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\tandl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case bit_xor:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\txorl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
        o << "\tmovl\t%eax, " << IR_reg_to_asm(params[0]) << "\n";
        break;
    case bit_or:
        o << "\tmovl\t" << IR_reg_to_asm(params[1]) << ", %eax\n";
        o << "\torl\t" << IR_reg_to_asm(params[2]) << ", %eax\n";
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

        // Appeler la fonction (underscore only for external functions on macOS)
#ifdef __APPLE__
        if (externalFunctions.count(params[0])) {
            o << "\tcall\t_" << params[0] << "\n";
        } else {
            o << "\tcall\t" << params[0] << "\n";
        }
#else
        o << "\tcall\t" << params[0] << "\n";
#endif

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
    case logical_and:
    {
        // Pour && paresseux, on compare le premier opérande à 0
        // Si c'est 0, on retourne 0, sinon on évalue le deuxième opérande
        string dest = params[0];
        string left = params[1];
        string right = params[2];
        
        // Générer des labels uniques et valides (sans le !)
        string label_base = dest.substr(1); // Enlever le ! du début
        string false_label = "label_" + label_base + "_false";
        string end_label = "label_" + label_base + "_end";
        
        // Charger le premier opérande
        o << "\tmovl\t" << IR_reg_to_asm(left) << ", %eax" << endl;
        // Comparer avec 0
        o << "\tcmpl\t$0, %eax" << endl;
        o << "\tje\t" << false_label << endl;
        // Si non-zéro, évaluer le deuxième opérande
        o << "\tmovl\t" << IR_reg_to_asm(right) << ", %eax" << endl;
        o << "\tcmpl\t$0, %eax" << endl;
        o << "\tmovl\t$0, %eax" << endl;
        o << "\tsetne\t%al" << endl;
        o << "\tmovl\t%eax, " << IR_reg_to_asm(dest) << endl;
        o << "\tjmp\t" << end_label << endl;
        o << false_label << ":" << endl;
        o << "\tmovl\t$0, " << IR_reg_to_asm(dest) << endl;
        o << end_label << ":" << endl;
        break;
    }
    case logical_or:
    {
        // Pour || paresseux, on compare le premier opérande à 0
        // Si c'est non-zéro, on retourne 1, sinon on évalue le deuxième opérande
        string dest = params[0];
        string left = params[1];
        string right = params[2];
        
        // Générer des labels uniques et valides (sans le !)
        string label_base = dest.substr(1); // Enlever le ! du début
        string true_label = "label_" + label_base + "_true";
        string end_label = "label_" + label_base + "_end";
        
        // Charger le premier opérande
        o << "\tmovl\t" << IR_reg_to_asm(left) << ", %eax" << endl;
        // Comparer avec 0
        o << "\tcmpl\t$0, %eax" << endl;
        o << "\tjne\t" << true_label << endl;
        // Si zéro, évaluer le deuxième opérande
        o << "\tmovl\t" << IR_reg_to_asm(right) << ", %eax" << endl;
        o << "\tcmpl\t$0, %eax" << endl;
        o << "\tmovl\t$0, %eax" << endl;
        o << "\tsetne\t%al" << endl;
        o << "\tmovl\t%eax, " << IR_reg_to_asm(dest) << endl;
        o << "\tjmp\t" << end_label << endl;
        o << true_label << ":" << endl;
        o << "\tmovl\t$1, " << IR_reg_to_asm(dest) << endl;
        o << end_label << ":" << endl;
        break;
    }
    }
}

// Génère le code assembleur ARM pour cette instruction IR
void IRInstr::gen_asm_arm(ostream &o)
{
    switch (op)
    {
    case ldconst: {
        int value = std::stoi(params[1]);
        uint32_t uval = static_cast<uint32_t>(value);
        if (value >= 0 && value <= 65535) {
            o << "\tmov w0, #" << value << "\n";
        } else {
            uint16_t low = uval & 0xFFFF;
            uint16_t mid = (uval >> 16) & 0xFFFF;
            o << "\tmovz w0, #" << low << "\n";
            if (mid)
                o << "\tmovk w0, #" << mid << ", lsl #16\n";
        }
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    }
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
    case mod:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tsdiv w2, w0, w1\n";
        o << "\tmul w2, w2, w1\n";
        o << "\tsub w0, w0, w2\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case rmem:
        if (params[1].size() >= 2 && params[1][0] == 'w' && isdigit(params[1][1])) {
            // Read from a register (w0-w7)
            o << "\tmov w0, " << params[1] << "\n";
            o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        } else {
            // Read from memory
            o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
            o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        }
        break;
    case wmem:
        if (params[1].size() >= 2 && params[1][0] == 'w' && isdigit(params[1][1])) {
            // Write from register to memory (w0-w7)
            o << "\tstr " << params[1] << ", [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        } else if (params[0] == params[1]) {
            break;
        } else {
            o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
            o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        }
        break;
    case ret:
        // Always move the return value to w0
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case not_op:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tcmp w0, #0\n";
        o << "\tcset w0, eq\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case bit_and:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tand w0, w0, w1\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case bit_xor:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\teor w0, w0, w1\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case bit_or:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\torr w0, w0, w1\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case call:
        // Parameter passing: w0-w7, stack for more
        for (size_t i = 2; i < params.size() && i < 10; ++i) {
            o << "\tldr w" << (i-2) << ", [sp, #" << IR_reg_to_asm(params[i]) << "]\n";
        }
        // For >8 params, push to stack (not implemented here)
#ifdef __APPLE__
        if (externalFunctions.count(params[0])) {
            o << "\tbl _" << params[0] << "\n";
        } else {
            o << "\tbl " << params[0] << "\n";
        }
#else
        o << "\tbl " << params[0] << "\n";
#endif
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        break;
    case logical_and:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tcmp w0, #0\n";
        o << "\tcset w0, ne\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp w1, #0\n";
        o << "\tcset w1, ne\n";
        o << "\tand w0, w0, w1\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case logical_or:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tcmp w0, #0\n";
        o << "\tcset w0, ne\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp w1, #0\n";
        o << "\tcset w1, ne\n";
        o << "\torr w0, w0, w1\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_eq:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp w0, w1\n";
        o << "\tcset w0, eq\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_ne:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp w0, w1\n";
        o << "\tcset w0, ne\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_lt:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp w0, w1\n";
        o << "\tcset w0, lt\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_gt:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp w0, w1\n";
        o << "\tcset w0, gt\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_le:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp w0, w1\n";
        o << "\tcset w0, le\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    case cmp_ge:
        o << "\tldr w0, [sp, #" << IR_reg_to_asm(params[1]) << "]\n";
        o << "\tldr w1, [sp, #" << IR_reg_to_asm(params[2]) << "]\n";
        o << "\tcmp w0, w1\n";
        o << "\tcset w0, ge\n";
        o << "\tstr w0, [sp, #" << IR_reg_to_asm(params[0]) << "]\n";
        break;
    }
}

// Classe BasicBlock : représente un bloc de base du CFG
// Un BasicBlock est une séquence d'instructions IR sans branchement interne
// Les BasicBlocks sont reliés entre eux dans le CFG pour modéliser le flot d'exécution (if/else, branchements, etc.)
// La génération d'assembleur se fait bloc par bloc, dans l'ordre d'exécution du CFG
BasicBlock::BasicBlock(CFG *cfg, string entry_label)
    : cfg(cfg), label(entry_label), exit_true(nullptr), exit_false(nullptr)
{
    cfg->add_bb(this);
}

// Ajoute une instruction IR à ce bloc
void BasicBlock::add_IRInstr(IRInstr::Operation op, Type t, vector<string> params)
{
    IRInstr *instr = new IRInstr(this, op, t, params);
    instrs.push_back(instr);
}

// Génère le code assembleur pour ce bloc (x86 ou ARM)
void BasicBlock::gen_asm(ostream &o)
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

    // Gestion des branches (contrôle de flot)
    if (exit_true == nullptr)
    {
        // Fin de fonction - ne pas générer l'épilogue ici
    }
    else if (exit_false == nullptr)
    {
        // Branchement inconditionnel
#ifdef ARM
        o << "\tb " << exit_true->label << endl;
#else
        o << "\tjmp " << exit_true->label << endl;
#endif
    }
    else
    {
        // Branchement conditionnel
#ifdef ARM
        o << "\tcmp w0, #0" << endl;
        o << "\tb.eq " << exit_false->label << endl;
        o << "\tb " << exit_true->label << endl;
#else
        o << "\tcmpl $0, %eax" << endl;
        o << "\tje " << exit_false->label << endl;
        o << "\tjmp " << exit_true->label << endl;
#endif
    }
}

// Classe CFG : représente le Control Flow Graph d'une fonction
// Le CFG contient tous les BasicBlocks d'une fonction et la table des symboles associée
// Il orchestre la génération du prologue, de l'épilogue, et la génération d'assembleur pour chaque bloc
// Le CFG permet aussi d'envisager des analyses ou optimisations globales sur la fonction
CFG::CFG(DefFonction *ast)
    : ast(ast), nextFreeSymbolIndex(0), nextBBnumber(0), current_bb(nullptr) {}

// Ajoute un BasicBlock au CFG
void CFG::add_bb(BasicBlock *bb)
{
    bbs.push_back(bb);
}

// Génère l'épilogue assembleur de la fonction (restaure la pile, retourne)
void CFG::gen_asm_epilogue(std::ostream &o)
{
#ifdef ARM
    int totalSize = 16 + (nextFreeSymbolIndex * 8);
    totalSize = ((totalSize + 15) & ~15);
    o << "\tldp x29, x30, [sp], #" << totalSize << "\n";
    o << "\tret\n";
#else
    o << "\tleave" << endl;
    o << "\tret" << endl;
#endif
}

// Ajoute une variable à la table des symboles
void CFG::add_to_symbol_table(string name, Type t)
{
    SymbolType[name] = t;
    SymbolIndex[name] = nextFreeSymbolIndex++;
}

// Crée une nouvelle variable temporaire
string CFG::create_new_tempvar(Type t)
{
    string name = "!" + to_string(nextFreeSymbolIndex);
    add_to_symbol_table(name, t);
    return name;
}

// Récupère l'index d'une variable
int CFG::get_var_index(string name)
{
    return SymbolIndex[name];
}

// Récupère le type d'une variable
Type CFG::get_var_type(string name)
{
    return SymbolType[name];
}

// Génère un nom unique pour un BasicBlock
string CFG::new_BB_name()
{
    return "BB_" + to_string(nextBBnumber++);
}

// Génère le prologue assembleur de la fonction (sauvegarde des registres, allocation de la pile)
void CFG::gen_asm_prologue(std::ostream &o) {
#ifdef ARM
    // 16 pour x29/x30, 8 octets par variable
    int totalSize = 16 + (nextFreeSymbolIndex * 8);
    totalSize = ((totalSize + 15) & ~15);
    o << "\tstp x29, x30, [sp, #-" << totalSize << "]!\n";
    o << "\tmov x29, sp\n";
#else
    o << "\tpushq %rbp" << endl;
    o << "\tmovq %rsp, %rbp" << endl;
    o << "\tsubq $" << (nextFreeSymbolIndex * 4) << ", %rsp" << endl;
#endif
}