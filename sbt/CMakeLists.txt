cmake_minimum_required (VERSION 3.0)
project (riscv-sbt)

# get build options from llvm-config

set (LLVM_CONFIG "$ENV{TOPDIR}/scripts/llvm-config.sh")

# include dirs
execute_process(
    COMMAND ${LLVM_CONFIG} --inc
    RESULT_VARIABLE RC1
    OUTPUT_VARIABLE SBT_INCLUDE_DIRS
)

# compile options
execute_process(
    COMMAND ${LLVM_CONFIG} --flags
    RESULT_VARIABLE RC2
    OUTPUT_VARIABLE SBT_COMPILE_OPTIONS
)

# compile defs
execute_process(
    COMMAND ${LLVM_CONFIG} --defs
    RESULT_VARIABLE RC3
    OUTPUT_VARIABLE SBT_COMPILE_DEFINITIONS
)

# link dirs
execute_process(
    COMMAND ${LLVM_CONFIG} --link-dirs
    RESULT_VARIABLE RC4
    OUTPUT_VARIABLE SBT_LINK_DIRS
)

# sys libs
execute_process(
    COMMAND ${LLVM_CONFIG} --system-libs
    RESULT_VARIABLE RC5
    OUTPUT_VARIABLE SBT_SYS_LIBS
)

# libs
execute_process(
    COMMAND ${LLVM_CONFIG} --libs arm bitwriter core object riscv
      support target x86
    RESULT_VARIABLE RC6
    OUTPUT_VARIABLE SBT_LIBS
)

# abort if llvm-config failed
if (RC1 OR RC2 OR RC3 OR RC4 OR RC5 OR RC6)
    message(FATAL_ERROR "Failed to execute llvm-config.sh")
endif()

# set debug options
message(STATUS "This is a ${CMAKE_BUILD_TYPE} build")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set (CMAKE_VERBOSE_MAKEFILE ON)
    set (SBT_COMPILE_OPTIONS ${SBT_COMPILE_OPTIONS} -Werror)
    message(STATUS "SBT_INCLUDE_DIRS = [${SBT_INCLUDE_DIRS}]")
    message(STATUS "SBT_COMPILE_OPTIONS = [${SBT_COMPILE_OPTIONS}]")
    message(STATUS "SBT_COMPILE_DEFINITIONS = [${SBT_COMPILE_DEFINITIONS}]")
    message(STATUS "SBT_LINK_DIRS = [${SBT_LINK_DIRS}]")
    message(STATUS "SBT_SYS_LIBS = [${SBT_SYS_LIBS}]")
    message(STATUS "SBT_LIBS = [${SBT_LIBS}]")
else()
    set (SBT_COMPILE_DEFINITIONS ${SBT_COMPILE_DEFINITIONS} NDEBUG)
    set (SBT_COMPILE_OPTIONS ${SBT_COMPILE_OPTIONS} -g)
    #set (CMAKE_VERBOSE_MAKEFILE ON)
endif()

# enable AT&T assembler
enable_language (ASM-ATT)
set (can_use_assembler TRUE)

# x86 syscall code
add_library (x86-syscall OBJECT x86-syscall.s)
target_compile_options (x86-syscall PRIVATE --32)

# x86 counters
add_library (x86-counters OBJECT x86-counters.s)
target_compile_options (x86-counters PRIVATE --32)

# SBT

set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
include_directories (${SBT_INCLUDE_DIRS})
link_directories (${SBT_LINK_DIRS})
add_executable (riscv-sbt
    BasicBlock.cpp
    Constants.cpp
    Context.cpp
    Disassembler.cpp
    Function.cpp
    Instruction.cpp
    Module.cpp
    Object.cpp
    Options.cpp
    Relocation.cpp
    SBTError.cpp
    Section.cpp
    ShadowImage.cpp
    Stack.cpp
    Syscall.cpp
    Translator.cpp
    Types.cpp
    Utils.cpp
    XRegisters.cpp
    sbt.cpp
    )
target_compile_options (riscv-sbt PRIVATE ${SBT_COMPILE_OPTIONS})
target_compile_definitions(riscv-sbt PRIVATE ${SBT_COMPILE_DEFINITIONS})
target_link_libraries (riscv-sbt
    ${SBT_LIBS}
    ${SBT_SYS_LIBS}
)

set (LIBC_C ${PROJECT_SOURCE_DIR}/libc.c)

# libc.bc
add_custom_command(OUTPUT libc.bc
    COMMAND clang
        --target=riscv32
        -isysroot / -isystem /usr/include
        -emit-llvm -c -O3 -mllvm -disable-llvm-optzns
        ${LIBC_C}
        MAIN_DEPENDENCY ${LIBC_C})
add_custom_target(libc DEPENDS libc.bc)
add_dependencies(riscv-sbt libc)

# install

install (TARGETS riscv-sbt DESTINATION bin)
install (FILES ${CMAKE_BINARY_DIR}/libc.bc DESTINATION share/riscv-sbt)
install (FILES ${CMAKE_BINARY_DIR}/CMakeFiles/x86-syscall.dir/x86-syscall.s.o
    RENAME x86-syscall.o DESTINATION share/riscv-sbt)
install (FILES ${CMAKE_BINARY_DIR}/CMakeFiles/x86-counters.dir/x86-counters.s.o
    RENAME x86-counters.o DESTINATION share/riscv-sbt)
