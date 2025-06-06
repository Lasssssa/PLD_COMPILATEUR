#include "DefFonction.h"
#include "IR.h"
#include <iostream>

using namespace std;

DefFonction::DefFonction(const string& name, const Type& returnType, const vector<Param>& params)
    : name(name), type(returnType), parameters(params), cfg(nullptr) {
    // Le CFG sera créé plus tard lors de la génération de code
}

void DefFonction::gen_asm_x86(ostream& o) const {
    // Ne rien faire ici, la génération est gérée dans visitProg
} 