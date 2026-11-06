cmake_minimum_required(VERSION 3.16)
project(sorting_benchmark VERSION 1.0 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Options (mirror the Makefile: DEBUG / STRICT / SANITIZE)
# ---------------------------------------------------------------------------
option(SB_STRICT "Enable extra warnings (Makefile: STRICT=1)" OFF)
option(SB_SANITIZE "Enable AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(SB_WERROR "Treat warnings as errors" ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
  message(WARNING
    "Compiler '${CMAKE_C_COMPILER_ID}' is untested. The benchmark supports GCC "
    "(MinGW/MSYS2/Linux) and Clang; MSVC is not supported yet.")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF) # -std=c17, not gnu17 (same as the Makefile)

# C++ is used only for the std::sort / std::stable_sort baselines.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

# The Makefile builds with plain -O2 (no -DNDEBUG). CMake's default Release
# flags are -O3 -DNDEBUG, which would make new results incomparable with old
# ones, so pin them explicitly.
set(CMAKE_C_FLAGS_RELEASE "-O2")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")

# ---------------------------------------------------------------------------
# Warnings and sanitizers
# ---------------------------------------------------------------------------
set(SB_WARNINGS
  -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow
  -Wformat=2 -Wcast-align -Wmissing-declarations -Wstrict-overflow=5 -Wundef)
if(SB_WERROR)
  list(APPEND SB_WARNINGS -Werror)
endif()

if(SB_STRICT)
  list(APPEND SB_WARNINGS
    -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough -Wcast-qual
    -Wstrict-prototypes -Wmissing-prototypes -Werror=implicit-function-declaration)
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    list(APPEND SB_WARNINGS -Wduplicated-cond -Wduplicated-branches -Wlogical-op)
  endif()
endif()

set(SB_SANITIZER_FLAGS "")
if(SB_SANITIZE)
  set(SB_SANITIZER_FLAGS -fsanitize=address,undefined -fstack-protector-strong)
endif()

# Flags recorded in the CSV "flags" column, in the same spirit as the Makefile:
# "<std> <opt> <warnings> <sanitizers>" with spaces replaced by underscores.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(SB_OPT_FLAGS -O0 -g)
else()
  set(SB_OPT_FLAGS -O2)
endif()
set(SB_FLAG_LIST -std=c17 ${SB_OPT_FLAGS} ${SB_WARNINGS} ${SB_SANITIZER_FLAGS})
string(REPLACE ";" "_" SB_FLAGS_STRING "${SB_FLAG_LIST}")

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
file(GLOB SB_CORE_SOURCES CONFIGURE_DEPENDS
  src/benchmark/*.c
  src/data/*.c
  src/metrics/*.c
  src/output/*.c
  src/sorting/*.c
  src/sorting/*.cpp)
list(APPEND SB_CORE_SOURCES src/utils.c)

add_library(sb_core STATIC ${SB_CORE_SOURCES})
target_include_directories(sb_core PUBLIC include)
# -Wstrict-prototypes / -Wmissing-prototypes are C-only: keep them off C++.
set(SB_CXX_WARNINGS ${SB_WARNINGS})
list(REMOVE_ITEM SB_CXX_WARNINGS -Wstrict-prototypes -Wmissing-prototypes
  -Werror=implicit-function-declaration)
target_compile_options(sb_core PUBLIC
  "$<$<COMPILE_LANGUAGE:C>:${SB_WARNINGS}>"
  "$<$<COMPILE_LANGUAGE:CXX>:${SB_CXX_WARNINGS}>"
  ${SB_SANITIZER_FLAGS})
target_link_options(sb_core PUBLIC ${SB_SANITIZER_FLAGS})
target_compile_definitions(sb_core PUBLIC "COMPILATION_FLAGS=\"${SB_FLAGS_STRING}\"")
if(NOT WIN32)
  target_link_libraries(sb_core PUBLIC m)
endif()

add_executable(benchmark src/main.c)
target_link_libraries(benchmark PRIVATE sb_core)

add_executable(test_sorting tests/test_sorting.c)
target_link_libraries(test_sorting PRIVATE sb_core)

# ---------------------------------------------------------------------------
# Tests (ctest): unit + golden counters, and a tiny end-to-end smoke run.
# The smoke run writes results/ inside the build directory, never the source.
# ---------------------------------------------------------------------------
enable_testing()
add_test(NAME unit_tests COMMAND test_sorting)
add_test(NAME benchmark_smoke
  COMMAND benchmark --seed 42 --repetitions 2 --sizes 10,100 --dataset all
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(benchmark_smoke PROPERTIES
  PASS_REGULAR_EXPRESSION "Experiments finished")

# Tests for the Python report tooling (standard library only). Skipped when no
# interpreter is available so the C build never depends on Python.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  add_test(NAME report_scripts
    COMMAND ${Python3_EXECUTABLE} -m unittest discover
            -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_report.py)
else()
  message(STATUS "Python 3 not found: report script tests are skipped")
endif()

message(STATUS "Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}; flags recorded in CSV: ${SB_FLAGS_STRING}")
