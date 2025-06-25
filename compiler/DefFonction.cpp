#include "DefFonction.h"
#include "IR.h"
#include <iostream>

DefFonction::DefFonction(const string& name, const Type& returnType, const vector<Param>& params)
    : name(name), type(returnType), parameters(params), cfg(nullptr) {
    // Le CFG sera créé plus tard lors de la génération de code
}