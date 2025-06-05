# Include your machine-specific config (contains paths to ANTLR etc.)
include config.mk.local

CC = g++
CCFLAGS = -g -c -std=c++17 -I$(ANTLRINC) -Wno-attributes
LDFLAGS = -g

# Entry point
default: all
all: ifcc

##########################################
# Object files to compile
OBJECTS = \
	compiler/build/ifccBaseVisitor.o \
	compiler/build/ifccLexer.o \
	compiler/build/ifccVisitor.o \
	compiler/build/ifccParser.o \
	compiler/build/main.o \
	compiler/build/SymbolTableVisitor.o \
	compiler/build/Visitors.o


# Final binary
ifcc: $(OBJECTS)
	@mkdir -p compiler/build
	$(CC) $(LDFLAGS) $(OBJECTS) $(ANTLRLIB) -o compiler/ifcc

##########################################
# Compile hand-written C++ code
compiler/build/%.o: compiler/%.cpp compiler/generated/ifccParser.cpp
	@mkdir -p compiler/build
	$(CC) $(CCFLAGS) -MMD -o $@ $<

##########################################
# Compile ANTLR-generated code
compiler/build/%.o: compiler/generated/%.cpp
	@mkdir -p compiler/build
	$(CC) $(CCFLAGS) -MMD -o $@ $<

# Auto dependency management
-include compiler/build/*.d
compiler/build/%.d:

##########################################
# ANTLR file generation
compiler/generated/ifccLexer.cpp compiler/generated/ifccVisitor.cpp compiler/generated/ifccBaseVisitor.cpp compiler/generated/ifccParser.cpp: compiler/ifcc.g4
	@mkdir -p compiler/generated
	cd compiler && java -jar $(ANTLRJAR) -visitor -no-listener -Dlanguage=Cpp -o generated ifcc.g4

# Prevent cleanup of intermediate files
.PRECIOUS: compiler/generated/ifcc%.cpp

##########################################
# Visual parse tree
FILE ?= testfiles/01_return42.c

gui:
	@mkdir -p compiler/generated compiler/build
	cd compiler && java -jar $(ANTLRJAR) -Dlanguage=Java -o generated ifcc.g4
	javac -cp $(ANTLRJAR) -d compiler/build compiler/generated/*.java
	java -cp $(ANTLRJAR):compiler/build org.antlr.v4.gui.TestRig ifcc axiom -gui $(FILE)

##########################################
# Run tests
test:
	python3 ./testfiles/ifcc-test.py ./testfiles

##########################################
# Clean everything
clean:
	rm -rf compiler/build compiler/generated
	rm -f compiler/ifcc
