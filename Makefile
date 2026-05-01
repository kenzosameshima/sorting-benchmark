# Compiler and flags
CC := gcc
CSTD ?= -std=c17
INCLUDES := -Iinclude -Iexperiments
WARNINGS := -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow \
	-Wformat=2 -Wcast-align -Wmissing-declarations -Wstrict-overflow=5 \
	-Wundef -Werror
STRICT_WARNINGS := -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough \
	-Wcast-qual -Wduplicated-cond -Wduplicated-branches -Wlogical-op \
	-Wstrict-prototypes -Wmissing-prototypes -Werror=implicit-function-declaration

# Debug mode: set DEBUG=1 to compile with -O0 -g
DEBUG ?= 0
BUILD_MODE := release
ifeq ($(DEBUG),1)
OPT := -O0 -g
BUILD_MODE := debug
else
OPT := -O2
endif

STRICT ?= 0
ifeq ($(STRICT),1)
WARNINGS += $(STRICT_WARNINGS)
endif

SANITIZE ?= 0
ifeq ($(SANITIZE),1)
SANITIZER_FLAGS := -fsanitize=address,undefined -fstack-protector-strong
BUILD_MODE := sanitize
endif

ifeq ($(STRICT),1)
BUILD_MODE := $(BUILD_MODE)-strict
endif

CFLAGS := $(CSTD) $(OPT) $(INCLUDES) $(WARNINGS) $(SANITIZER_FLAGS) -MMD -MP
LDFLAGS := $(SANITIZER_FLAGS)
LDLIBS := -lm

# Project files
BUILD_ROOT := build
BUILD_DIR := $(BUILD_ROOT)/$(BUILD_MODE)
TARGET := benchmark
TEST_TARGET := unit_tests

COMMON_SOURCES := $(wildcard data/*.c metrics/*.c sorting/*.c src/utils.c)
BENCH_SOURCES := src/main.c experiments/tests.c $(COMMON_SOURCES)
UNIT_SOURCES := tests/unit_tests.c $(COMMON_SOURCES)

BENCH_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(BENCH_SOURCES))
UNIT_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(UNIT_SOURCES))
DEPS := $(BENCH_OBJECTS:.o=.d) $(UNIT_OBJECTS:.o=.d)

# Platform-dependent settings
ifeq ($(OS),Windows_NT)
BIN := $(TARGET).exe
TEST_BIN := $(TEST_TARGET).exe
RUN_CMD := .\$(BIN)
TEST_RUN_CMD := .\$(TEST_BIN)
MKDIR_CMD = if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
CLEAN_CMD := if exist "$(BUILD_ROOT)" rmdir /S /Q "$(BUILD_ROOT)" & if exist "$(BIN)" del /Q "$(BIN)" & if exist "$(TEST_BIN)" del /Q "$(TEST_BIN)"
else
BIN := $(TARGET)
TEST_BIN := $(TEST_TARGET)
RUN_CMD := ./$(BIN)
TEST_RUN_CMD := ./$(TEST_BIN)
MKDIR_CMD = mkdir -p $(dir $@)
CLEAN_CMD := rm -rf $(BUILD_ROOT) $(BIN) $(TEST_BIN)
endif

# Phony targets
.PHONY: all run test asan-test clean rebuild help

# Default target
all: $(BIN)

# Link executables
$(BIN): $(BENCH_OBJECTS)
	$(CC) $(LDFLAGS) $(BENCH_OBJECTS) -o $@ $(LDLIBS)

$(TEST_BIN): $(UNIT_OBJECTS)
	$(CC) $(LDFLAGS) $(UNIT_OBJECTS) -o $@ $(LDLIBS)

# Compile individual object files
$(BUILD_DIR)/%.o: %.c
	$(MKDIR_CMD)
	$(CC) $(CFLAGS) -c $< -o $@

# Run the program
run: $(BIN)
	$(RUN_CMD) $(ARGS)

# Build and run unit tests
test: $(TEST_BIN)
	$(TEST_RUN_CMD)

asan-test:
	$(MAKE) clean
	ASAN_OPTIONS=detect_leaks=0 $(MAKE) SANITIZE=1 test

# Clean build artifacts (ignore errors with leading dash)
clean:
	-$(CLEAN_CMD)

# Rebuild from scratch
rebuild: clean all

# Help target
help:
	@echo "Makefile targets disponiveis:"
	@echo "  all      - Compila o benchmark"
	@echo "  run      - Executa o benchmark"
	@echo "  test     - Compila e executa os testes unitarios"
	@echo "  asan-test - Executa testes com sanitizers e LeakSanitizer desativado"
	@echo "  clean    - Remove binarios, objetos e dependencias"
	@echo "  rebuild  - Limpa e recompila tudo"
	@echo "  help     - Mostra esta mensagem"
	@echo "Variaveis uteis:"
	@echo "  DEBUG=1  - Compila em modo debug (-O0 -g)"
	@echo "  STRICT=1 - Ativa avisos extras de compilacao"
	@echo "  SANITIZE=1 - Ativa AddressSanitizer e UndefinedBehaviorSanitizer"

-include $(DEPS)
