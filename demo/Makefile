# Makefile pour la démo du compilateur

CC = gcc
COMPILER = ../compiler/ifcc

DEMO_CS := $(wildcard *.c)
DEMO_SS := $(DEMO_CS:.c=.s)
DEMO_BINS := $(DEMO_CS:.c=.bin)

# Compile chaque .c en .s avec notre compilateur
%.s: %.c $(COMPILER)
	$(COMPILER) $< > $@

# Compile chaque .s en binaire exécutable avec gcc
%.bin: %.s
	$(CC) $< -o $@

# Cible pour compiler un fichier spécifique
demo:
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make demo FILE=<nom_du_fichier.c>"; \
		echo "Example: make demo FILE=01_return42.c"; \
		exit 1; \
	fi
	@if [ ! -f "$(FILE)" ]; then \
		echo "Error: File $(FILE) not found"; \
		exit 1; \
	fi
	$(COMPILER) $(FILE) > $(FILE:.c=.s)
	$(CC) $(FILE:.c=.s) -o $(FILE:.c=.bin)
	@echo "Compilation terminée: $(FILE) -> $(FILE:.c=.bin)"

# Exécute le binaire généré
demo-run: demo
	@./$(FILE:.c=.bin)
	@echo "[retour: $$?]"

# Affiche l'arbre de parse visuel (utilise la fonctionnalité gui du Makefile principal)
demo-gi:
	@if [ -z "$(FILE)" ]; then \
		echo "Usage: make demo-gi FILE=<nom_du_fichier.c>"; \
		exit 1; \
	fi
	@cd .. && make gui FILE=demo/$(FILE)

# Compile et exécute en une commande
demo-all: demo demo-run

# Nettoie les fichiers générés dans demo/
demo-clean:
	rm -f *.s *.bin 