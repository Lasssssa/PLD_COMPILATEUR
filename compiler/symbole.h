#ifndef SYMBOLE_H
#define SYMBOLE_H

#include "type.h"
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <stdexcept>

// Structure pour stocker les informations d'un symbole
struct SymbolInfo {
    Type type;           // Type du symbole
    int offset;          // Décalage par rapport à %rbp
    bool isParam;        // Indique si c'est un paramètre de fonction
    bool isArray;        // Indique si c'est un tableau
    int arraySize;       // Taille du tableau si c'est un tableau

    // Constructeur par défaut nécessaire pour std::map
    SymbolInfo() : type(Type()), offset(0), isParam(false), isArray(false), arraySize(0) {}

    // Constructeur avec paramètres
    SymbolInfo(Type t, int off, bool param = false, bool array = false, int size = 0)
        : type(t), offset(off), isParam(param), isArray(array), arraySize(size) {}
};

class SymbolTable {
public:
    SymbolTable() : currentOffset(0) {}
    
    // Méthodes pour gérer les variables
    void addSymbol(const std::string& name, const Type& type, bool isParam = false) {
        if (symbols.find(name) != symbols.end()) {
            throw std::runtime_error("Symbol '" + name + "' already defined");
        }
        
        // Les paramètres sont stockés à des offsets positifs
        // Les variables locales sont stockées à des offsets négatifs
        int offset;
        if (isParam) {
            offset = 16 + (paramCount * 8); // 16 pour sauvegarder %rbp et l'adresse de retour
            paramCount++;
        } else {
            currentOffset -= type.getSize();
            offset = currentOffset;
        }
        
        symbols[name] = SymbolInfo(type, offset, isParam);
    }

    // Méthode pour ajouter un tableau
    void addArray(const std::string& name, const Type& baseType, int size) {
        if (symbols.find(name) != symbols.end()) {
            throw std::runtime_error("Array '" + name + "' already defined");
        }
        
        // Pour un tableau, on alloue size * sizeof(baseType) octets
        currentOffset -= (size * baseType.getSize());
        symbols[name] = SymbolInfo(baseType, currentOffset, false, true, size);
    }

    // Méthodes pour accéder aux symboles
    bool hasSymbol(const std::string& name) const {
        return symbols.find(name) != symbols.end();
    }

    const SymbolInfo& getSymbol(const std::string& name) const {
        auto it = symbols.find(name);
        if (it == symbols.end()) {
            throw std::runtime_error("Symbol '" + name + "' not found");
        }
        return it->second;
    }

    // Méthodes pour obtenir des informations sur les symboles
    int getOffset(const std::string& name) const {
        return getSymbol(name).offset;
    }

    Type getType(const std::string& name) const {
        return getSymbol(name).type;
    }

    bool isParameter(const std::string& name) const {
        return getSymbol(name).isParam;
    }

    bool isArray(const std::string& name) const {
        return getSymbol(name).isArray;
    }

    int getArraySize(const std::string& name) const {
        const SymbolInfo& info = getSymbol(name);
        if (!info.isArray) {
            throw std::runtime_error("Symbol '" + name + "' is not an array");
        }
        return info.arraySize;
    }

    // Méthodes pour la gestion de la pile
    int getCurrentOffset() const {
        return currentOffset;
    }

    int getParamCount() const {
        return paramCount;
    }

    // Méthode pour créer une nouvelle portée
    void enterScope() {
        scopeStack.push_back(currentOffset);
    }

    // Méthode pour quitter une portée
    void exitScope() {
        if (!scopeStack.empty()) {
            currentOffset = scopeStack.back();
            scopeStack.pop_back();
        }
    }

    // Méthode pour obtenir l'expression d'accès à la variable en assembleur
    std::string getAsmAccess(const std::string& name) const {
        const SymbolInfo& info = getSymbol(name);
        std::stringstream ss;
        ss << info.offset << "(%rbp)";
        return ss.str();
    }

    // Méthode pour obtenir l'expression d'accès à un élément de tableau
    std::string getArrayElementAccess(const std::string& name, int index) const {
        const SymbolInfo& info = getSymbol(name);
        if (!info.isArray) {
            throw std::runtime_error("Symbol '" + name + "' is not an array");
        }
        if (index < 0 || index >= info.arraySize) {
            throw std::runtime_error("Array index out of bounds");
        }
        
        std::stringstream ss;
        ss << (info.offset + (index * info.type.getSize())) << "(%rbp)";
        return ss.str();
    }

private:
    std::map<std::string, SymbolInfo> symbols;  // Table des symboles
    int currentOffset;                          // Offset courant pour les variables locales
    int paramCount = 0;                         // Nombre de paramètres
    std::vector<int> scopeStack;                // Pile des portées
};

// Classe pour gérer les tables des symboles imbriquées
class SymbolTableManager {
public:
    SymbolTableManager() {
        enterScope(); // Crée la portée globale
    }

    void enterScope() {
        tables.push_back(new SymbolTable());
    }

    void exitScope() {
        if (!tables.empty()) {
            delete tables.back();
            tables.pop_back();
        }
    }

    SymbolTable* getCurrentTable() {
        if (tables.empty()) {
            throw std::runtime_error("No active symbol table");
        }
        return tables.back();
    }

    // Méthodes de délégation vers la table courante
    void addSymbol(const std::string& name, const Type& type, bool isParam = false) {
        getCurrentTable()->addSymbol(name, type, isParam);
    }

    void addArray(const std::string& name, const Type& baseType, int size) {
        getCurrentTable()->addArray(name, baseType, size);
    }

    bool hasSymbol(const std::string& name) const {
        // Recherche dans toutes les tables, de la plus récente à la plus ancienne
        for (auto it = tables.rbegin(); it != tables.rend(); ++it) {
            if ((*it)->hasSymbol(name)) {
                return true;
            }
        }
        return false;
    }

    const SymbolInfo& getSymbol(const std::string& name) const {
        // Recherche dans toutes les tables, de la plus récente à la plus ancienne
        for (auto it = tables.rbegin(); it != tables.rend(); ++it) {
            if ((*it)->hasSymbol(name)) {
                return (*it)->getSymbol(name);
            }
        }
        throw std::runtime_error("Symbol '" + name + "' not found in any scope");
    }

    ~SymbolTableManager() {
        for (auto table : tables) {
            delete table;
        }
    }

private:
    std::vector<SymbolTable*> tables;
};

#endif 