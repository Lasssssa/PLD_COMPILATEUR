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
12. **12_putchar_advanced.c** — putchar avancé (affichage de chaîne)
13. **14_getchar_basic.c** — getchar basique (lecture d'un caractère)
14. **15_getchar_echo.c** — getchar + putchar (echo de caractère)

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

## Démonstrations putchar et getchar

### putchar - Affichage de caractères

#### putchar basique (08_putchar.c)
```bash
make demo-all FILE=08_putchar.c
# Affiche : Z
```

#### putchar avancé (12_putchar_advanced.c)
```bash
make demo-all FILE=12_putchar_advanced.c
# Affiche : Hello World!
#          (avec nouvelle ligne)
```

### getchar - Lecture de caractères

#### getchar basique (14_getchar_basic.c)
```bash
echo "A" | make demo-all FILE=14_getchar_basic.c
# Retourne le code ASCII de 'A' (65)
```

#### getchar avec echo (15_getchar_echo.c)
```bash
echo "X" | make demo-all FILE=15_getchar_echo.c
# Affiche : X
```

### Utilisation interactive

Pour tester les programmes getchar de manière interactive :

```bash
# Compiler le programme
make demo FILE=15_getchar_echo.c

# Lancer en mode interactif
./15_getchar_echo.bin
# Puis taper un caractère et appuyer sur Entrée
```

### Points techniques

- **putchar(int c)** : Affiche le caractère correspondant au code ASCII `c`
- **getchar()** : Lit un caractère depuis l'entrée standard (stdin)
- **Codes ASCII** : 'A' = 65, 'a' = 97, '0' = 48, '\n' = 10
- **Conversion de casse** : 'a' - 32 = 'A', 'A' + 32 = 'a'
- **getchar retourne -1** : En cas d'erreur ou fin de fichier (EOF)
- **Limitation** : Le compilateur ne supporte pas les valeurs négatives, ce qui limite certaines opérations arithmétiques sur les caractères
- **Caractères d'échappement** : Le compilateur ne supporte pas les caractères d'échappement comme `\n`. Utilisez `putchar(10)` pour une nouvelle ligne

## Notes techniques

- Les fichiers `.s` contiennent le code assembleur généré
- Les fichiers `.bin` sont les exécutables générés
- Le compilateur se trouve dans `../compiler/ifcc`
- Tous les fichiers utilisent la syntaxe C simplifiée supportée par le compilateur

## Comprendre l'assembleur généré

### Architecture cible
Le compilateur génère du code assembleur **ARM64/AArch64** (architecture 64 bits ARM). Cette architecture est utilisée sur les Mac M1/M2, les serveurs ARM, et de nombreux appareils mobiles.

### Structure générale d'une fonction

Chaque fonction générée suit cette structure :

```asm
	.text                    # Section code
	.globl	_main           # Rendre la fonction visible
_main:                      # Label de la fonction
	stp x29, x30, [sp, #-32]!  # Prologue : sauvegarde frame pointer et link register
	mov x29, sp               # Définir le frame pointer
main_BB_0:                   # Premier bloc de base (Basic Block)
	# ... instructions de la fonction ...
	ldp x29, x30, [sp], #32   # Épilogue : restauration des registres
	ret                       # Retour de fonction
```

### Conventions d'appel ARM64

#### Registres principaux
- **w0-w7** : Paramètres d'entrée (8 premiers paramètres)
- **w0** : Valeur de retour
- **x29** : Frame pointer (pointeur de pile)
- **x30** : Link register (adresse de retour)
- **sp** : Stack pointer (pointeur de pile)

#### Gestion de la pile
```
+---------------------+
| param N (>8)        |  (sur la pile, offset positif)
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

### Instructions ARM64 courantes

#### Chargement et stockage
```asm
mov w0, #42          # Charger la constante 42 dans w0
str w0, [sp, #16]    # Stocker w0 à l'adresse sp+16
ldr w0, [sp, #16]    # Charger depuis sp+16 dans w0
```

#### Opérations arithmétiques
```asm
add w0, w0, w1       # w0 = w0 + w1
sub w0, w0, w1       # w0 = w0 - w1
mul w0, w0, w1       # w0 = w0 * w1
sdiv w0, w0, w1      # w0 = w0 / w1 (division signée)
```

#### Comparaisons et branchements
```asm
cmp w0, w1           # Comparer w0 et w1
cset w0, gt          # w0 = 1 si w0 > w1, sinon 0
cset w0, lt          # w0 = 1 si w0 < w1, sinon 0
cset w0, eq          # w0 = 1 si w0 == w1, sinon 0
cset w0, ne          # w0 = 1 si w0 != w1, sinon 0
b.eq label           # Brancher si égal
b label              # Branchement inconditionnel
```

#### Opérations bit-à-bit
```asm
and w0, w0, w1       # w0 = w0 & w1 (ET bit-à-bit)
orr w0, w0, w1       # w0 = w0 | w1 (OU bit-à-bit)
eor w0, w0, w1       # w0 = w0 ^ w1 (OU exclusif)
```

### Analyse des exemples réels

#### 1. Programme simple (01_return42.c)
```c
int main() {
    return 42;
}
```

Assembleur généré :
```asm
	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-32]!  # Prologue
	mov x29, sp
main_BB_0:
	mov w0, #42                # Charger 42 dans w0 (registre de retour)
	str w0, [sp, #16]          # Stocker temporairement
	ldr w0, [sp, #16]          # Recharger (pattern standard du compilateur)
	ldp x29, x30, [sp], #32    # Épilogue
	ret
```

**Analyse** : Le compilateur suit un pattern systématique de stockage/rechargement pour toutes les valeurs, même les constantes simples.

#### 2. Variables et arithmétique (02_var_arith.c)
```c
int main() {
    int a = 5;
    int b = a + 3;
    return b * 2;
}
```

Assembleur généré :
```asm
	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-96]!  # Prologue avec plus d'espace
	mov x29, sp
main_BB_0:
	mov w0, #5                 # Initialiser a = 5
	str w0, [sp, #24]          # Stocker a
	ldr w0, [sp, #24]          # Recharger a
	str w0, [sp, #16]          # Copie temporaire
	ldr w0, [sp, #16]          # Recharger
	str w0, [sp, #48]          # Stocker pour calcul
	mov w0, #3                 # Constante 3
	str w0, [sp, #56]          # Stocker 3
	ldr w0, [sp, #48]          # Charger a
	ldr w1, [sp, #56]          # Charger 3
	add w0, w0, w1             # a + 3
	str w0, [sp, #40]          # Stocker résultat
	ldr w0, [sp, #40]          # Recharger
	str w0, [sp, #32]          # Stocker b
	ldr w0, [sp, #32]          # Recharger b
	str w0, [sp, #72]          # Stocker pour calcul
	mov w0, #2                 # Constante 2
	str w0, [sp, #80]          # Stocker 2
	ldr w0, [sp, #72]          # Charger b
	ldr w1, [sp, #80]          # Charger 2
	mul w0, w0, w1             # b * 2
	str w0, [sp, #64]          # Stocker résultat
	ldr w0, [sp, #64]          # Charger pour retour
	ldp x29, x30, [sp], #96    # Épilogue
	ret
```

**Analyse** : Chaque variable est stockée à un offset différent sur la pile. Le compilateur fait beaucoup de stockages/rechargements qui pourraient être optimisés.

#### 3. Structures de contrôle (03_if_else.c)
```c
int main() {
    int a = 2;
    if (a > 1) {
        return 10;
    } else {
        return 20;
    }
}
```

Assembleur généré :
```asm
	.text
	.globl	_main
_main:
	stp x29, x30, [sp, #-80]!  # Prologue
	mov x29, sp
main_BB_0:
	mov w0, #2                 # Initialiser a = 2
	str w0, [sp, #24]          # Stocker a
	ldr w0, [sp, #24]          # Recharger a
	str w0, [sp, #16]          # Copie temporaire
	ldr w0, [sp, #16]          # Recharger
	str w0, [sp, #32]          # Stocker pour comparaison
	mov w0, #1                 # Constante 1
	str w0, [sp, #40]          # Stocker 1
	ldr w0, [sp, #32]          # Charger a
	ldr w1, [sp, #40]          # Charger 1
	cmp w0, w1                 # Comparer a > 1
	cset w0, gt                # w0 = 1 si a > 1, sinon 0
	str w0, [sp, #48]          # Stocker résultat comparaison
	cmp w0, #0                 # Tester si condition vraie
	b.eq BB_3                  # Si faux, aller au else
	b BB_1                     # Si vrai, aller au if
BB_3:                          # Bloc else
	mov w0, #20                # Retourner 20
	str w0, [sp, #64]          # Stocker
	ldr w0, [sp, #64]          # Recharger
BB_1:                          # Bloc if
	mov w0, #10                # Retourner 10
	str w0, [sp, #56]          # Stocker
	ldr w0, [sp, #56]          # Recharger
	ldp x29, x30, [sp], #80    # Épilogue
	ret
```

**Analyse** : Le compilateur utilise `cset` pour convertir les comparaisons en valeurs booléennes, puis teste si le résultat est non-zéro pour décider du branchement.

#### 4. Appels de fonction (04_function_call.c)
```c
int f(int x, int y) {
    return x + y;
}

int main() {
    return f(3, 4);
}
```

Assembleur généré :
```asm
	.text
	.globl	f
f:                             # Fonction f
	stp x29, x30, [sp, #-64]!  # Prologue
	mov x29, sp
f_BB_0:
	str w0, [sp, #16]          # Sauvegarder paramètre x
	str w1, [sp, #24]          # Sauvegarder paramètre y
	ldr w0, [sp, #16]          # Charger x
	str w0, [sp, #40]          # Stocker pour calcul
	ldr w0, [sp, #24]          # Charger y
	str w0, [sp, #48]          # Stocker pour calcul
	ldr w0, [sp, #40]          # Charger x
	ldr w1, [sp, #48]          # Charger y
	add w0, w0, w1             # x + y
	str w0, [sp, #32]          # Stocker résultat
	ldr w0, [sp, #32]          # Charger pour retour
	ldp x29, x30, [sp], #64    # Épilogue
	ret
	.globl	_main
_main:                         # Fonction main
	stp x29, x30, [sp, #-48]!  # Prologue
	mov x29, sp
main_BB_2:
	mov w0, #3                 # Premier paramètre (x)
	str w0, [sp, #24]          # Stocker
	mov w0, #4                 # Deuxième paramètre (y)
	str w0, [sp, #32]          # Stocker
	ldr w0, [sp, #24]          # Charger paramètre 1
	ldr w1, [sp, #32]          # Charger paramètre 2
	bl f                       # Appel de fonction f
	str w0, [sp, #16]          # Stocker valeur de retour
	ldr w0, [sp, #16]          # Charger pour retour
	ldp x29, x30, [sp], #48    # Épilogue
	ret
```

**Analyse** : Les paramètres sont passés dans les registres `w0` et `w1`, puis sauvegardés sur la pile dans la fonction appelée. L'instruction `bl f` effectue l'appel de fonction.

#### 5. Fonction à plusieurs paramètres (05_multi_param.c)
```c
int sum(int a, int b, int c) {
    return a + b + c;
}

int main() {
    return sum(1, 2, 3);
}
```

**Points clés** :
- Les 3 paramètres sont passés dans `w0`, `w1`, `w2`
- L'addition se fait en deux étapes : `(a + b) + c`
- Chaque étape suit le pattern stockage/rechargement

#### 6. Opérations bit-à-bit (09_logical_bitwise.c)
```c
int main() {
    int x = 1 | 2;
    int y = x & 3;
    return y ^ 1;
}
```

**Instructions utilisées** :
- `orr w0, w0, w1` pour l'opération OU (`|`)
- `and w0, w0, w1` pour l'opération ET (`&`)
- `eor w0, w0, w1` pour l'opération OU exclusif (`^`)

#### 7. Opérateurs logiques (10_logical_if.c)
```c
int main() {
    int a = 0;
    int b = 1;
    if (a || b) {
        return 1;
    } else {
        return 0;
    }
}
```

**Analyse** : Le compilateur utilise `cset w0, ne` pour convertir chaque opérande en booléen (non-zéro), puis `orr` pour effectuer l'opération logique OU.

#### 8. Conditions complexes (11_if_complex.c)
```c
int main() {
    int a = 2;
    int b = 3;
    if (a > 1 && b < 5) {
        if (a + b == 5) {
            return 42;
        } else {
            return 1;
        }
    } else {
        return 0;
    }
}
```

**Analyse** : Le compilateur gère les conditions complexes en :
1. Évaluant chaque comparaison avec `cset`
2. Combinant les résultats avec `and` ou `orr`
3. Créant des blocs de base séparés pour chaque branche
4. Utilisant des branchements conditionnels pour naviguer entre les blocs

### Patterns observés dans le code généré

#### 1. Gestion des variables locales
- Chaque variable est stockée à un offset différent sur la pile
- Pattern systématique : `mov` → `str` → `ldr` → `str` → `ldr`
- Les offsets augmentent de 8 en 8 (alignement 64 bits)

#### 2. Optimisations manquantes
- Beaucoup de stockages/rechargements redondants
- Variables temporaires inutiles
- Pas d'optimisation des constantes

#### 3. Gestion des blocs de base
- Chaque bloc a un label unique (`main_BB_0`, `BB_1`, etc.)
- Les branchements utilisent ces labels
- L'ordre des blocs peut être différent de l'ordre dans le code source

#### 4. Conventions d'appel respectées
- Paramètres dans `w0-w7`
- Valeur de retour dans `w0`
- Sauvegarde/restauration de `x29` et `x30`
- Alignement de la pile sur 16 octets

### Points d'attention

1. **Redondances** : Le code généré contient beaucoup de stockages/rechargements qui pourraient être éliminés
2. **Espace pile** : Le compilateur réserve souvent plus d'espace que nécessaire
3. **Optimisations futures** : Possibilité d'améliorer significativement la qualité du code généré
4. **Débogage** : Les offsets de pile facilitent le débogage mais rendent le code moins efficace 