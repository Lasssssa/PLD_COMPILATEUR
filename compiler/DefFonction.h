#ifndef DEFFONCTION_H
#define DEFFONCTION_H

#include "IR.h"
#include "type.h"
#include "visitor_ir.h"  // Pour avoir accès à la structure Param
#include <string>
#include <vector>
#include <iostream>

// Forward declaration
class CFG;

// Classe pour représenter une définition de fonction
class DefFonction {
public:
    // Constructeur
    DefFonction(const std::string& n, const Type& t, const std::vector<Param>& params);
    
    // Getters
    std::string getName() const { return name; }
    Type getType() const { return type; }
    const std::vector<Param>& getParams() const { return parameters; }
    
private:
    std::string name;           // Nom de la fonction
    Type type;                  // Type de retour
    std::vector<Param> parameters;  // Paramètres
    CFG* cfg;                   // Graphe de flot de contrôle
};

#endif 