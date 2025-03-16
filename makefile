CC = gcc
CXX = g++

# Détection du système d'exploitation
OS := $(shell uname -s 2>/dev/null || echo Windows_NT)

# Options communes
CFLAGS = -Wall -std=c99 -Wno-missing-braces -O1
CXXFLAGS = -Wall -std=c++11 -O1  # Ajout des options pour C++
INCLUDE = -Iinclude/

SRC = main.cpp
OBJ_C = $(SRC_C:.c=.o)

ifeq ($(OS), Windows_NT)
    # Compilation pour Windows (statique)
    LDFLAGS = -Llib/ -lraylib -lopengl32 -lgdi32 -lwinmm
    OUTPUT = main.exe
    RM = del /Q
else
    # Compilation pour Linux (dynamique)
    LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    OUTPUT = main
    RM = rm -f
endif

# Règle principale
all:
	$(CXX) $(SRC)  -o $(OUTPUT) $(CXXFLAGS) $(INCLUDE) $(LDFLAGS)

# Nettoyer les fichiers exécutables 	$(CC) $(SRC) -o $(OUTPUT) $(CFLAGS) $(INCLUDE) $(LDFLAGS)
clean:
	$(RM) $(OUTPUT)
