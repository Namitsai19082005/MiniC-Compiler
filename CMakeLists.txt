cmake_minimum_required(VERSION 3.16)
project(minic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Warnings (GCC/Clang only)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Sources organized by component
set(COMMON_SRCS  src/common/error.cpp)
set(LEXER_SRCS   src/lexer/lexer.cpp)
set(PARSER_SRCS  src/parser/parser.cpp src/parser/ast_printer.cpp)
set(SEMA_SRCS    src/sema/sema.cpp src/sema/symbol_table.cpp)
set(IR_SRCS      src/ir/ir_gen.cpp src/ir/cfg.cpp src/ir/tac.cpp src/ir/ir_printer.cpp)
set(OPT_SRCS     src/opt/constant_folding.cpp src/opt/dead_code_elim.cpp
                  src/opt/copy_propagation.cpp)
set(CODEGEN_SRCS src/codegen/codegen.cpp src/codegen/reg_alloc.cpp
                  src/codegen/x86_emitter.cpp)
set(DEBUG_SRCS   src/debug/dwarf_emitter.cpp src/debug/source_map.cpp)
set(LINKER_SRCS  src/linker/symbol_resolver.cpp src/linker/link_helper.cpp)

add_executable(minic
    src/main.cpp
    ${COMMON_SRCS} ${LEXER_SRCS} ${PARSER_SRCS} ${SEMA_SRCS}
    ${IR_SRCS} ${OPT_SRCS} ${CODEGEN_SRCS} ${DEBUG_SRCS} ${LINKER_SRCS}
)

target_include_directories(minic PRIVATE src/)

# Custom target for Valgrind
add_custom_target(valgrind
    COMMAND valgrind --leak-check=full --show-leak-kinds=all
            --error-exitcode=1 $<TARGET_FILE:minic>
            ${CMAKE_SOURCE_DIR}/tests/inputs/basics/hello.mc -o /tmp/hello.s
    DEPENDS minic
    COMMENT "Running Valgrind memory check"
)

# Install
install(TARGETS minic DESTINATION bin)
