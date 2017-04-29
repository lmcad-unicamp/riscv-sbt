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

set (SBT_RISCV_MASTER_LIBS
  LLVMRISCVMasterDisassembler
  LLVMRISCVMasterCodeGen
  LLVMRISCVMasterAsmParser
  LLVMRISCVMasterDesc
  LLVMRISCVMasterInfo
  LLVMRISCVMasterAsmPrinter
  )

# set debug options
message(STATUS "This is a ${CMAKE_BUILD_TYPE} build")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set (CMAKE_VERBOSE_MAKEFILE ON)
  message(STATUS "SBT_INCLUDE_DIRS = [${SBT_INCLUDE_DIRS}]")
  message(STATUS "SBT_COMPILE_OPTIONS = [${SBT_COMPILE_OPTIONS}]")
  message(STATUS "SBT_COMPILE_DEFINITIONS = [${SBT_COMPILE_DEFINITIONS}]")
  message(STATUS "SBT_LINK_DIRS = [${SBT_LINK_DIRS}]")
  message(STATUS "SBT_SYS_LIBS = [${SBT_SYS_LIBS}]")
  message(STATUS "SBT_LIBS = [${SBT_LIBS}]")
else()
  set (SBT_COMPILE_DEFINITIONS ${SBT_COMPILE_DEFINITIONS} NDEBUG)
endif()

# enable AT&T assembler
enable_language (ASM-ATT)
set (can_use_assembler TRUE)

# x86 syscall code
add_library (x86-syscall OBJECT x86-syscall.s)
target_compile_options (x86-syscall PRIVATE --32)

# SBT

set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
include_directories (${SBT_INCLUDE_DIRS})
link_directories (${SBT_LINK_DIRS})
add_executable (riscv-sbt
  Constants.cpp
  Object.cpp
  Register.cpp
  SBTError.cpp
  Translator.cpp
  Utils.cpp
  sbt.cpp
  )
target_compile_options (riscv-sbt PRIVATE ${SBT_COMPILE_OPTIONS})
target_compile_definitions(riscv-sbt PRIVATE ${SBT_COMPILE_DEFINITIONS})
target_link_libraries (riscv-sbt
  ${SBT_RISCV_MASTER_LIBS}
  ${SBT_LIBS}
  ${SBT_SYS_LIBS}
)

set (LIBC_CPP ${PROJECT_SOURCE_DIR}/libc.cpp)

# libc.bc
add_custom_command(OUTPUT libc.bc
  COMMAND clang -O0 -std=c++11 -femit-all-decls -emit-llvm -c ${LIBC_CPP}
  MAIN_DEPENDENCY ${LIBC_CPP})
add_custom_target(libc DEPENDS libc.bc)
add_dependencies(riscv-sbt libc)

# rvsc
add_custom_command(OUTPUT rvsc.bc
  COMMAND ${CMAKE_BINARY_DIR}/riscv-sbt -gen-sc-handler -o rvsc.bc
  DEPENDS riscv-sbt)

add_custom_command(OUTPUT x86-rvsc.s
  COMMAND llc -O3 -o x86-rvsc.s -march x86 rvsc.bc
  DEPENDS rvsc.bc)

add_library (x86-rvsc OBJECT x86-rvsc.s)
target_compile_options (x86-rvsc PRIVATE --32)

# install

install (TARGETS riscv-sbt DESTINATION bin)
install (FILES ${CMAKE_BINARY_DIR}/libc.bc DESTINATION share/riscv-sbt)
install (FILES ${CMAKE_BINARY_DIR}/CMakeFiles/x86-syscall.dir/x86-syscall.s.o
  RENAME x86-syscall.o DESTINATION share/riscv-sbt)
install (FILES ${CMAKE_BINARY_DIR}/CMakeFiles/x86-rvsc.dir/x86-rvsc.s.o
  RENAME x86-rvsc.o DESTINATION share/riscv-sbt)
