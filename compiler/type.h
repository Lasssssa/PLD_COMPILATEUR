#ifndef TYPE_H
#define TYPE_H

#include <string>
#include <iostream>

// Enumération des types de base supportés
enum TypeEnum {
    INT     // Type entier uniquement
};

// Classe Type pour représenter les types dans le compilateur
class Type {
public:
    // Constructeurs
    Type() : typeEnum(INT) {}
    Type(TypeEnum t) : typeEnum(t) {}
    
    // Getters
    TypeEnum getTypeEnum() const { return typeEnum; }
    
    // Méthodes utilitaires
    bool isInt() const { return typeEnum == INT; }
    
    // Méthode pour obtenir la taille du type en octets
    int getSize() const {
        switch (typeEnum) {
            case INT:
                return 4;
            default:
                return 0;
        }
    }
    
    // Méthode pour convertir le type en chaîne de caractères
    std::string toString() const {
        switch (typeEnum) {
            case INT:
                return "int";
            default:
                return "unknown";
        }
    }
    
    // Opérateur de comparaison
    bool operator==(const Type& other) const {
        return typeEnum == other.typeEnum;
    }
    
    bool operator!=(const Type& other) const {
        return !(*this == other);
    }

    // Constante de type
    static const Type INT_TYPE;  // Déclaration seulement

private:
    TypeEnum typeEnum;
};

// Opérateur de sortie pour le débogage
inline std::ostream& operator<<(std::ostream& os, const Type& type) {
    os << type.toString();
    return os;
}

#endif 