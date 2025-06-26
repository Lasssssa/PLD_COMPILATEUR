# PLD_COMPILATEUR — Présentation technique

## Notions clés et définitions

### AST (Abstract Syntax Tree)
- **Définition** : Arbre de syntaxe abstraite. Structure arborescente représentant la structure syntaxique du code source, sans détails superflus (parenthèses, points-virgules, etc.).
- **Rôle** : Sert de base à toutes les analyses ultérieures (sémantique, génération de code, etc.).

### IR (Intermediate Representation)
- **Définition** : Représentation intermédiaire du programme, indépendante de l'architecture cible. Sert de pont entre le front-end (analyse du code source) et le back-end (génération de code machine).
- **Rôle** : Facilite les analyses, les optimisations, et la génération de code pour différentes architectures.

#### IR 3-adresses
- **Définition** : Un IR 3-adresses est une forme d'IR où chaque instruction manipule au plus deux opérandes sources et une destination (d'où "3 adresses").
- **Exemple** :
  ```
  t1 = a + b
  t2 = t1 * c
  x = t2
  ```
  Ici, chaque instruction IR a la forme : `destination = source1 op source2`.
- **Intérêt** : Cette forme simplifie la génération d'assembleur et les analyses intermédiaires (optimisations, etc.).
- **Pourquoi ce choix ?** :
  - **Simplicité** : Chaque opération est atomique et explicite, ce qui rend l'IR facile à lire, à analyser et à transformer.
  - **Génération d'assembleur facilitée** : La plupart des instructions machine manipulent 2 ou 3 opérandes, donc l'IR 3-adresses se traduit presque directement en assembleur.
  - **Optimisations** : Les passes d'optimisation sont plus simples à implémenter sur un IR 3-adresses.
  - **Indépendance de l'architecture** : L'IR 3-adresses ne dépend pas de la machine cible, ce qui facilite le reciblage (x86, ARM, etc.).
  - **Débogage** : Il est plus facile de comprendre et de tracer l'exécution d'un programme au niveau IR 3-adresses.

### CFG (Control Flow Graph)
- **Définition** : Graphe de flot de contrôle. Structure représentant toutes les possibilités d'exécution d'un programme, sous forme de blocs de base (BasicBlocks) reliés par des arcs (branches, sauts, etc.).
- **Rôle** : Permet de modéliser précisément le flot d'exécution, de gérer les branchements complexes, et de préparer des analyses/optimisations globales.

### IRInstruction
- **Définition** : Une instruction élémentaire de l'IR (ex : addition, affectation, lecture mémoire, etc.), indépendante de l'architecture cible.
- **Rôle** : Sert de brique de base pour la génération de code assembleur. Chaque IRInstruction peut être traduite en une ou plusieurs instructions machine selon l'architecture.

## Organisation du code/fichiers

- **ifcc.g4** : Grammaire ANTLR du langage C simplifié. Définit la syntaxe reconnue par le compilateur.
- **SymbolTableVisitor.cpp/h** : Visiteur ANTLR pour la construction de la table des symboles et l'analyse sémantique (déclarations, types, erreurs, etc.).
- **visitor_ir.cpp/h** : Visiteur ANTLR pour la génération de l'IR (3-adresses) et du CFG à partir de l'AST.
- **IR.cpp/h** : Définition et gestion des instructions IR, des BasicBlocks, du CFG, et génération de code assembleur (x86/ARM).
- **DefFonction.cpp/h** : Structure représentant une fonction (nom, type, paramètres, CFG associé).
- **testfiles/** : Dossier contenant tous les fichiers de tests (cas simples, erreurs, cas limites, etc.).

## FAQ technique

**Q : Pourquoi un IR 3-adresses ?**
R : Simplicité, génération d'assembleur facilitée, indépendance de l'architecture, optimisation plus facile.

**Q : Pourquoi un CFG ?**
R : Pour modéliser précisément le flot d'exécution, gérer les branchements complexes, préparer des optimisations.

**Q : Comment sont gérés les paramètres de fonction ?**
R : Dans les registres (x86 : %edi, %esi, etc. / ARM : w0-w7), puis copiés sur la pile locale.

**Q : Pourquoi des offsets de 4 pour les variables locales ?**
R : Un int fait 4 octets, cela garantit l'alignement et la compatibilité ABI.

**Q : Que faudrait-il changer pour supporter les floats ?**
R : Adapter la table des types, l'IR, la génération d'assembleur, et l'alignement mémoire.

**Q : Comment ajouter un nouvel opérateur ?**
R : Modifier la grammaire, ajouter la visite correspondante dans VisitorIR et SymbolTableVisitor, et gérer l'opération dans IRInstr.

**Q : Comment le compilateur gère-t-il les erreurs ?**
R : Les erreurs sémantiques sont détectées lors de la visite de l'AST (SymbolTableVisitor), les erreurs de syntaxe par ANTLR.

## Points d'originalité, limites et perspectives

- **Originalité** :
  - Séparation claire front/middle/back-end
  - Support multi-plateforme (x86/ARM)
  - Table des symboles robuste et extensible
  - IR 3-adresses et CFG bien structurés
- **Limites** :
  - Pas de support des tableaux, pointeurs, struct
  - Pas d'optimisations avancées (propagation de constantes, dead code elimination)
  - Gestion partielle des variables globales
- **Perspectives** :
  - Ajout d'optimisations IR
  - Support de nouveaux types (float, double)
  - Gestion complète des variables globales et du .data
  - Extension vers d'autres architectures

## 1. Introduction et objectifs

Ce projet est un compilateur C simplifié, développé dans le cadre du cours de compilation. Il prend en entrée un sous-ensemble du langage C, effectue des analyses statiques, génère un Intermediate Representation (IR), construit un Control Flow Graph (CFG), puis produit du code assembleur pour x86_64 (et ARM en option). L'objectif est de couvrir toutes les étapes classiques d'un compilateur moderne, de la grammaire à la génération de code, en passant par l'analyse sémantique et l'optimisation intermédiaire.

## 2. Architecture générale

Le compilateur est structuré en trois grandes parties :
- **Front-end** : Parsing, grammaire ANTLR, construction de l'AST
- **Middle-end** : Analyses statiques (table des symboles, vérifications sémantiques), génération de l'IR et du CFG
- **Back-end** : Génération de code assembleur, gestion du reciblage (x86/ARM)

Schéma :
```
C source → [ANTLR] → AST → [SymbolTableVisitor] → Table des symboles/analyses → [VisitorIR] → IR/CFG → [IR] → Assembleur
```

## 2bis. Choix des offsets et des registres (conventions d'appel)

### Variables locales et offsets (x86_64)
- **x86_64** : Les variables locales sont stockées à des offsets négatifs par rapport à `%rbp` (ex : `-4(%rbp)`, `-8(%rbp)`, ...). Cela permet de réserver de l'espace sur la pile pour chaque variable locale, chaque `int` occupant 4 octets. Ce choix respecte la convention d'appel System V AMD64, standard sur Linux/macOS.
- **Pourquoi négatif ?** : `%rbp` pointe au sommet de la pile au début de la fonction. On réserve de l'espace en décrémentant `%rsp`, donc les variables locales sont accessibles à des adresses négatives par rapport à `%rbp`.

### Paramètres de fonction et registres (x86_64)
- **x86_64** : Les 6 premiers paramètres sont passés dans les registres (dans l'ordre) : `%rdi`, `%rsi`, `%rdx`, `%rcx`, `%r8`, `%r9` (ou leur version 32 bits pour les `int`). Les suivants sont passés sur la pile à des offsets positifs (rarement utilisé ici).
- **Pourquoi ces registres ?** : C'est la convention d'appel System V AMD64, qui optimise la vitesse des appels de fonction en évitant la pile pour les premiers arguments.

### Résumé visuel (x86_64)
```
+---------------------+
| ...                |  <- adresses croissantes
| param N (>6)       |  (sur la pile, offset positif)
| ...                |
| param 6            |  (dans %r9)
| param 5            |  (dans %r8)
| param 4            |  (dans %rcx)
| param 3            |  (dans %rdx)
| param 2            |  (dans %rsi)
| param 1            |  (dans %rdi)
| return address     |
| old %rbp           |  <- %rbp pointe ici
|---------------------|
| local var 1        |  (offset -4(%rbp))
| local var 2        |  (offset -8(%rbp))
| ...                |  (offsets négatifs)
+---------------------+
```

### Variables locales et offsets (ARM64/AArch64)
- **ARM64/AArch64** : Les variables locales sont stockées à des offsets **positifs** depuis `sp` (stack pointer), après la sauvegarde du frame pointer (`x29`) et du link register (`x30`).
- **Alignement** : L'espace réservé pour les variables locales est aligné sur 16 octets pour respecter l'ABI ARM64.
- **Pourquoi positif ?** : Sur ARM, la pile descend aussi, mais les accès mémoire pour les variables locales se font à des offsets positifs à partir de la base du frame (`sp` après le prologue). Cela garantit l'alignement et la compatibilité avec les outils de débogage et d'autres compilateurs.

### Paramètres de fonction et registres (ARM64/AArch64)
- **ARM64/AArch64** : Les **8 premiers paramètres** d'une fonction sont passés dans les registres **w0 à w7** (pour les entiers 32 bits). Les suivants sont passés sur la pile à des offsets positifs depuis `sp`.
- **Pourquoi ces registres ?** : Les registres sont beaucoup plus rapides que la mémoire, donc on optimise les appels de fonction en utilisant les registres pour les premiers arguments. Cette convention est standard sur toutes les plateformes ARM64 (Linux, macOS M1/M2, Android…).

### Résumé visuel (ARM64/AArch64)
```
+---------------------+
| param N (>8)        |  (sur la pile, offset positif depuis sp)
| ...                 |
| param 8             |  (dans w7)
| param 7             |  (dans w6)
| ...                 |
| param 1             |  (dans w0)
| return address      |
| x29 (frame pointer) |
| x30 (link register) |
|---------------------|
| local var 1         |  (offset +16(sp))
| local var 2         |  (offset +24(sp))
| ...                 |  (offsets positifs)
+---------------------+
```
- Les 16 premiers octets sont réservés pour `x29` et `x30` (frame pointer et link register).
- Les variables locales commencent à l'offset 16, puis 24, 32, etc.

### Pourquoi ce choix ?
- Respect des conventions d'appel pour la compatibilité avec le C standard et les ABI des OS modernes.
- Les registres sont plus rapides que la mémoire, d'où leur utilisation pour les premiers paramètres.
- Les offsets négatifs (x86) ou positifs (ARM) évitent d'écraser les paramètres ou la zone de retour.
- Cela facilite le débogage, la portabilité et la maintenance.
- L'alignement mémoire strict sur ARM64 évite les erreurs et garantit la portabilité.

### Dans le code
- Ajout d'une variable locale :
  - x86 : on décrémente l'offset courant de 4 à chaque déclaration (`currentOffset -= 4`).
  - ARM : on réserve de l'espace sur la pile, chaque variable ayant un offset positif (ex : 16, 24, 32…).
- Ajout d'un paramètre :
  - x86 : on lit la valeur dans le registre correspondant.
  - ARM : si c'est l'un des 8 premiers, il est dans `w0` à `w7` (copie dans la variable locale), sinon sur la pile à un offset positif.
- Lecture/écriture mémoire :
  - x86 : on utilise l'offset négatif pour générer l'adresse mémoire dans l'assembleur (`movl %eax, -4(%rbp)`).
  - ARM : on utilise l'offset positif (`str w0, [sp, #16]`).

## 2ter. Sens de lecture des instructions IR et assembleur

### Convention IR
- Les instructions IR sont toujours de la forme :
  `op destination, source1, source2`
- Cela rend la génération de code systématique et lisible, quelle que soit l'architecture cible.

### x86_64 : sens de lecture
- En assembleur x86, la convention est souvent :
  `op source, destination`
- Pour les opérations arithmétiques, on utilise souvent `%eax` comme registre temporaire :
  ```asm
  movl source1, %eax
  addl source2, %eax
  movl %eax, destination
  ```
- **Sens de lecture** :
  1. On charge la première source dans `%eax`
  2. On applique l'opération avec la deuxième source
  3. On stocke le résultat dans la destination

### ARM64/AArch64 : sens de lecture
- En assembleur ARM, la convention est :
  `op destination, source1, source2`
- Exemple :
  ```asm
  ldr w0, [sp, #offset_source1]
  ldr w1, [sp, #offset_source2]
  add w0, w0, w1
  str w0, [sp, #offset_destination]
  ```
- **Sens de lecture** :
  1. On charge les sources dans des registres temporaires (`w0`, `w1`)
  2. On effectue l'opération et on stocke le résultat dans la destination

### Comparaisons et accès mémoire
- Pour les comparaisons, le résultat est mis dans un registre temporaire (`%al` sur x86, `w0` sur ARM), puis étendu ou stocké.
- Pour les accès mémoire, la convention est toujours **source → destination**.

### Appels de fonction
- Les paramètres sont placés dans les registres selon la convention d'appel de l'architecture (voir section précédente).

### Résumé
- **x86** : "On lit les sources, on fait l'opération dans `%eax`, puis on écrit le résultat dans la destination."
- **ARM** : "On lit les sources dans des registres temporaires, on fait l'opération, et on écrit directement dans la destination."
- **Dans tous les cas** : "Le sens de lecture est toujours source → destination, ce qui rend le code IR lisible et la génération d'assembleur systématique."

## 2quater. Fonctionnement global du compilateur et flow des étapes

### Étapes principales du compilateur

1. **Parsing (Front-end)**
   - Le code source C est analysé par ANTLR à l'aide de la grammaire `ifcc.g4`.
   - Un AST (arbre de syntaxe abstraite) est généré.
   - Cette étape vérifie la syntaxe du programme.

2. **Analyse sémantique et table des symboles (Middle-end)**
   - Le visiteur `SymbolTableVisitor` parcourt l'AST.
   - Il construit la table des symboles (variables, fonctions, paramètres).
   - Il vérifie les règles sémantiques : déclarations, utilisations, types, nombre de paramètres, etc.
   - Il signale les erreurs et warnings (ex : variable non utilisée, fonction main manquante).

3. **Génération de l'IR et du CFG (Middle-end)**
   - Le visiteur `VisitorIR` parcourt l'AST et génère un IR (Intermediate Representation) de type 3-adresses.
   - Pour chaque fonction, un **CFG (Control Flow Graph)** est construit : il s'agit d'un graphe de BasicBlocks, chaque bloc représentant une séquence d'instructions sans branchement interne.
   - Le CFG permet de modéliser précisément le flot d'exécution (if/else, return, etc.), de préparer d'éventuelles optimisations, et de faciliter la génération d'assembleur.

4. **Génération de code assembleur (Back-end)**
   - Le CFG est parcouru (ordre topologique/post-ordre).
   - Chaque instruction IR génère son code assembleur selon l'architecture cible (x86/ARM).
   - Le prologue et l'épilogue de chaque fonction sont générés pour gérer la pile et les registres.

### Schéma du flow du compilateur

```
C source
   │
   ▼
[ANTLR Parsing]
   │
   ▼
AST (arbre de syntaxe abstraite)
   │
   ├──► [SymbolTableVisitor] → Table des symboles + vérifications sémantiques
   │
   └──► [VisitorIR] → IR 3-adresses + CFG (par fonction)
                        │
                        ▼
                 Génération assembleur (x86/ARM)
```

### Rôle et intérêt du CFG et de l'IR
- **IR (Intermediate Representation)** : Permet de séparer la logique du langage source de la génération de code cible. Il facilite l'extension, la maintenance, et d'éventuelles optimisations intermédiaires.
- **CFG (Control Flow Graph)** : Permet de modéliser le flot d'exécution réel du programme, de gérer les branchements complexes (if/else imbriqués, retours multiples), et de préparer des analyses ou optimisations futures (ex : détection de code mort, propagation de constantes).

### Résumé
- Le compilateur suit un pipeline classique : parsing → analyse sémantique → IR/CFG → génération de code.
- Chaque étape est séparée pour faciliter la maintenance, la détection d'erreurs, et l'extension du projet.
- Le CFG et l'IR sont des outils puissants pour la robustesse et l'évolutivité du compilateur.

---

## 3. Front-end : grammaire, parsing, choix syntaxiques

- **Grammaire** : Fichier `ifcc.g4` (ANTLR4), très commenté. Elle définit la syntaxe supportée (fonctions, variables, expressions, if/else, opérateurs arithmétiques, logiques, bit-à-bit, etc.).
- **Choix syntaxiques** : Syntaxe proche du C, mais simplifiée (pas de pointeurs, pas de tableaux, pas de struct, etc.). Ajout du support des caractères (`'a'`), des opérateurs logiques (`&&`, `||`), et des fonctions à paramètres multiples.
- **AST** : Utilisation de l'AST généré par ANTLR, pas d'AST maison. Les visiteurs ANTLR sont utilisés pour parcourir l'AST et générer l'IR ou la table des symboles.
- **Originalité** : La grammaire est conçue pour être facilement extensible (ajout de nouveaux types, de nouvelles constructions syntaxiques).

## 4. Middle-end : analyses statiques, table des symboles, IR, CFG

- **Table des symboles** : Gérée par `SymbolTableVisitor`. Vérifie :
  - Déclarations multiples/interdites
  - Utilisation de variables non déclarées
  - Variables non utilisées (warning)
  - Présence de la fonction `main`
  - Appels de fonctions avec le bon nombre d'arguments
- **Analyses statiques** : Détection d'erreurs sémantiques avant la génération de code. Affichage d'erreurs et de warnings détaillés.
- **IR (Intermediate Representation)** : Généré par `VisitorIR` à partir de l'AST. L'IR est de type 3-adresses, inspiré de LLVM, et permet de séparer la logique du langage source de la génération de code cible.
- **CFG (Control Flow Graph)** : Chaque fonction possède un CFG, composé de BasicBlocks. Permet une gestion fine du flot de contrôle (if/else, return, etc.) et prépare le terrain pour d'éventuelles optimisations.
- **Originalité** : Séparation claire entre analyses statiques et génération d'IR. Possibilité d'extension pour d'autres analyses (constantes, portée, etc.).

## 5. Back-end : génération de code, reciblage, ARM/x86

- **Génération de code** : Fichiers `IR.cpp`/`IR.h`. Chaque instruction IR sait générer son code assembleur pour x86_64 (et ARM en option). Le CFG orchestre la génération du prologue, de l'épilogue, et des blocs de base.
- **Reciblage** : Le back-end est conçu pour être facilement adaptable à d'autres architectures (ARM déjà partiellement supporté). Les conventions d'appel sont respectées (registres, pile).
- **Points forts** : Gestion des opérateurs avancés (logiques, bit-à-bit, modulo, etc.), support des fonctions à paramètres multiples, gestion des appels externes (`putchar`, `getchar`).
- **Points faibles** : Pas de support des tableaux, pointeurs, ni d'optimisations avancées (inlining, propagation de constantes, etc.).

## 6. Validation et tests

- **Stratégie de tests** : Plus de 100 fichiers de tests dans `testfiles/`, couvrant :
  - Cas de base (return, opérations, if/else)
  - Cas d'erreur (variables non déclarées, redeclarations, mauvais types, etc.)
  - Cas avancés (fonctions, opérateurs logiques, bit-à-bit, etc.)
- **Tests de non-régression** : Les tests sont relancés à chaque modification majeure pour éviter les régressions.
- **Bugs connus** : Certains tests complexes (ex : appels de fonctions imbriqués, cas limites sur les types) peuvent échouer ou ne pas être gérés. Ces cas sont documentés et montrés en démo.

## 7. Démo et points forts/faibles

- **Démo** : Compilation de plusieurs fichiers tests (simples, avancés, erreurs). Affichage du code assembleur généré, exécution, et comparaison avec le résultat attendu.
- **Points forts** :
  - Robustesse de la table des symboles
  - IR et CFG bien structurés, facilitant l'extension
  - Support multi-plateforme (x86/ARM)
  - Gestion des erreurs et warnings explicites
- **Points faibles** :
  - Pas de support des types avancés ni des optimisations
  - Quelques cas limites non gérés (voir tests)

--- 