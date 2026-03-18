# Compiler and flags
CC := gcc
CSTD := -std=c11
INCLUDES := -Iinclude -Iexperiments
WARNINGS := -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow \
	-Wformat=2 -Wcast-align -Wmissing-declarations -Wstrict-overflow=5 \
	-Wundef -Werror

# Debug mode: set DEBUG=1 to compile with -O0 -g
DEBUG ?= 0
ifeq ($(DEBUG),1)
OPT := -O0 -g
else
OPT := -O2
endif

CFLAGS := $(CSTD) $(OPT) $(INCLUDES) $(WARNINGS)

# Project files
TARGET := test
SOURCES := $(wildcard src/*.c data/*.c metrics/*.c sorting/*.c experiments/*.c)
OBJECTS := $(SOURCES:.c=.o)

# Platform-dependent settings
ifeq ($(OS),Windows_NT)
BIN := $(TARGET).exe
RUN_CMD := .\$(BIN)
WIN_OBJECTS := $(subst /,\\,$(OBJECTS))
CLEAN_CMD := if exist "$(BIN)" del /Q "$(BIN)" & for %%f in ($(WIN_OBJECTS)) do @if exist "%%f" del /Q "%%f"
else
BIN := $(TARGET)
RUN_CMD := ./$(BIN)
CLEAN_CMD := rm -f $(BIN) $(OBJECTS)
endif

# Phony targets
.PHONY: all run clean rebuild help

# Default target
all: $(BIN)

# Link executable
$(BIN): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(BIN)

# Compile individual object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the program
run: $(BIN)
	$(RUN_CMD)

# Clean build artifacts (ignore errors with leading dash)
clean:
	-$(CLEAN_CMD)

# Rebuild from scratch
rebuild: clean all

# Help target
help:
	@echo "Makefile targets disponiveis:"
	@echo "  all      - Compila o projeto"
	@echo "  run      - Executa o binario"
	@echo "  clean    - Remove binario e objetos"
	@echo "  rebuild  - Limpa e recompila tudo"
	@echo "  help     - Mostra esta mensagem"
	@echo "Variaveis uteis:"
	@echo "  DEBUG=1  - Compila em modo debug (-O0 -g)"
