# Compiler and flags
CC := gcc
CXX := g++
CXXSTD ?= -std=c++17
CSTD ?= -std=c17
INCLUDES := -Iinclude
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

CXXFLAGS := $(CXXSTD) $(OPT) $(INCLUDES) $(filter-out -Wstrict-prototypes -Wmissing-prototypes -Werror=implicit-function-declaration,$(WARNINGS)) $(SANITIZER_FLAGS) -MMD -MP

empty :=
space := $(empty) $(empty)
CFLAGS_FOR_CSV := $(subst $(space),_,$(strip $(CSTD) $(OPT) $(WARNINGS) $(SANITIZER_FLAGS)))

CFLAGS := $(CSTD) $(OPT) $(INCLUDES) $(WARNINGS) $(SANITIZER_FLAGS) \
	-DCOMPILATION_FLAGS=\"$(CFLAGS_FOR_CSV)\" -MMD -MP
LDFLAGS := $(SANITIZER_FLAGS)
LDLIBS := -lm

# Project files
BUILD_ROOT := build
BUILD_DIR := $(BUILD_ROOT)/$(BUILD_MODE)
TARGET := benchmark
TEST_TARGET := test_sorting

COMMON_SOURCES := $(wildcard src/benchmark/*.c src/data/*.c src/metrics/*.c src/output/*.c src/sorting/*.c) src/utils.c
CXX_SOURCES := $(wildcard src/sorting/*.cpp)
COMMON_SOURCES += $(CXX_SOURCES)
BENCH_SOURCES := src/main.c $(COMMON_SOURCES)
UNIT_SOURCES := tests/test_sorting.c $(COMMON_SOURCES)

BENCH_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(patsubst %.c,$(BUILD_DIR)/%.o,$(BENCH_SOURCES)))
UNIT_OBJECTS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(patsubst %.c,$(BUILD_DIR)/%.o,$(UNIT_SOURCES)))
DEPS := $(BENCH_OBJECTS:.o=.d) $(UNIT_OBJECTS:.o=.d)

# Platform-dependent settings
ifeq ($(OS),Windows_NT)
BIN := $(TARGET).exe
TEST_BIN := $(TEST_TARGET).exe
else
BIN := $(TARGET)
TEST_BIN := $(TEST_TARGET)
endif

# mingw32-make runs recipes with sh.exe when one is on PATH (Git Bash, MSYS2)
# and with cmd.exe otherwise, so pick the command syntax from the shell that
# make actually uses, not from the OS. When sh.exe is found, SHELL is its full
# path; when it is not, SHELL stays the bare name "sh.exe" and make falls back
# to cmd.exe.
USE_CMD_SHELL := 0
ifeq ($(OS),Windows_NT)
ifeq ($(findstring /,$(subst \,/,$(SHELL))),)
USE_CMD_SHELL := 1
endif
endif

ifeq ($(USE_CMD_SHELL),1)
RUN_CMD := .\$(BIN)
TEST_RUN_CMD := .\$(TEST_BIN)
MKDIR_CMD = if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
CLEAN_CMD := (if exist "$(BUILD_ROOT)" rmdir /S /Q "$(BUILD_ROOT)") & (if exist "$(BIN)" del /Q "$(BIN)") & (if exist "$(TEST_BIN)" del /Q "$(TEST_BIN)")
else
RUN_CMD := ./$(BIN)
TEST_RUN_CMD := ./$(TEST_BIN)
MKDIR_CMD = mkdir -p $(dir $@)
CLEAN_CMD := rm -rf $(BUILD_ROOT) $(BIN) $(TEST_BIN)
endif

# Phony targets
.PHONY: all bench academic-bench run test sanitize asan-test lint format clean rebuild help

# Default target
all: $(BIN)

bench: $(BIN)

academic-bench: $(BIN)
	$(RUN_CMD) --seed 42 --repetitions 30 --skip-quadratic-after 10000

# Link executables
$(BIN): $(BENCH_OBJECTS)
	$(CXX) $(LDFLAGS) $(BENCH_OBJECTS) -o $@ $(LDLIBS)

$(TEST_BIN): $(UNIT_OBJECTS)
	$(CXX) $(LDFLAGS) $(UNIT_OBJECTS) -o $@ $(LDLIBS)

# Compile individual object files
$(BUILD_DIR)/%.o: %.c
	$(MKDIR_CMD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	$(MKDIR_CMD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the program
run: $(BIN)
	$(RUN_CMD) $(ARGS)

# Build and run unit tests
test: $(TEST_BIN)
	$(TEST_RUN_CMD)

sanitize:
	$(MAKE) clean
	$(MAKE) SANITIZE=1 all test

asan-test:
	$(MAKE) clean
	ASAN_OPTIONS=detect_leaks=0 $(MAKE) SANITIZE=1 test

lint:
	cppcheck --enable=all src/ tests/

format:
	clang-format -i src/*.c src/benchmark/*.c src/sorting/*.c src/sorting/*.cpp src/data/*.c src/metrics/*.c src/output/*.c tests/*.c include/*.h

# Clean build artifacts (ignore errors with leading dash)
clean:
	-$(CLEAN_CMD)

# Rebuild from scratch
rebuild: clean all

# Help target
help:
	@echo "Available Makefile targets:"
	@echo "  all      - Builds the benchmark"
	@echo "  bench    - Builds the benchmark"
	@echo "  academic-bench - Runs the academic profile with 30 repetitions"
	@echo "  run      - Runs the benchmark"
	@echo "  test     - Builds and runs the unit tests"
	@echo "  sanitize - Builds benchmark and tests with sanitizers"
	@echo "  asan-test - Runs tests with sanitizers and LeakSanitizer disabled"
	@echo "  lint     - Runs cppcheck"
	@echo "  format   - Formats sources and headers with clang-format (needs clang-format on PATH)"
	@echo "  clean    - Removes binaries, objects and dependencies"
	@echo "  rebuild  - Cleans and rebuilds everything"
	@echo "  help     - Shows this message"
	@echo "Useful variables:"
	@echo "  DEBUG=1  - Builds in debug mode (-O0 -g)"
	@echo "  STRICT=1 - Enables extra compiler warnings"
	@echo "  SANITIZE=1 - Enables AddressSanitizer and UndefinedBehaviorSanitizer"

-include $(DEPS)
