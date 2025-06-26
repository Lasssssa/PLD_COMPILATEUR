# Démo du Compilateur

Ce dossier contient les fichiers de démonstration du compilateur, organisés du plus simple au plus complexe selon l'ordre de développement des fonctionnalités.

## Organisation des fichiers

1. **01_return42.c** — Programme minimal (return 42)
2. **02_var_arith.c** — Déclarations, affectations, arithmétique
3. **03_if_else.c** — Structures de contrôle (if/else)
4. **04_function_call.c** — Appel de fonction simple
5. **05_multi_param.c** — Fonction à plusieurs paramètres
6. **06_nested_function.c** — Appel de fonction imbriqué
7. **07_char.c** — Gestion du type char
8. **08_putchar.c** — Appel à une fonction externe (putchar)
9. **09_logical_bitwise.c** — Opérateurs logiques et bit-à-bit
10. **10_logical_if.c** — Opérateurs logiques dans un if
11. **11_if_complex.c** — If imbriqué et conditions complexes

## Utilisation du Makefile

### Pour un fichier spécifique
```bash
# Compiler un fichier
make demo FILE=01_return42.c

# Exécuter le binaire généré
make demo-run FILE=01_return42.c

# Afficher le code source
make demo-gi FILE=01_return42.c

# Compiler et exécuter en une commande
make demo-all FILE=01_return42.c
```

### Pour tous les fichiers
```bash
# Compiler tous les fichiers
make demo-all-files

# Exécuter tous les binaires
make demo-run-all

# Afficher tous les sources
make demo-gi-all
```

### Nettoyage
```bash
# Supprimer les fichiers générés (.s et .bin)
make demo-clean
```

## Flow de démo recommandé

1. **Introduction** : `make demo-gi FILE=01_return42.c`
2. **Compilation** : `make demo FILE=01_return42.c`
3. **Exécution** : `make demo-run FILE=01_return42.c`
4. **Répéter** pour chaque fichier dans l'ordre

## Notes techniques

- Les fichiers `.s` contiennent le code assembleur généré
- Les fichiers `.bin` sont les exécutables générés
- Le compilateur se trouve dans `../compiler/ifcc`
- Tous les fichiers utilisent la syntaxe C simplifiée supportée par le compilateur 