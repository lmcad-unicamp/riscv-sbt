cmake_minimum_required (VERSION 3.0)
project (riscv-sbt)

enable_language (ASM-ATT)
set (can_use_assembler TRUE)

add_library (x86_syscall OBJECT x86_syscall.s)
target_compile_options (x86_syscall PRIVATE --32)

include_directories ("${PROJECT_SOURCE_DIR}/../toolchain/include")
link_directories ("${PROJECT_SOURCE_DIR}/../toolchain/lib")

add_executable (riscv-sbt sbt.cpp)
target_compile_options (riscv-sbt PRIVATE
  -fPIC -fvisibility-inlines-hidden -Wall -W -Wno-unused-parameter
  -Wwrite-strings -Wcast-qual -Wno-missing-field-initializers -pedantic
  -Wno-long-long -Wno-maybe-uninitialized -Wdelete-non-virtual-dtor
  -Wno-comment -Werror=date-time -std=c++11 -ffunction-sections
  -fdata-sections -O3 -fno-exceptions -fno-rtti)
target_compile_definitions(riscv-sbt PRIVATE
  NDEBUG _GNU_SOURCE __STDC_CONSTANT_MACROS __STDC_FORMAT_MACROS
  __STDC_LIMIT_MACROS)
target_link_libraries (riscv-sbt
  rt dl tinfo z m pthread LLVMBitWriter LLVMCore LLVMSupport LLVMDemangle)

install (TARGETS riscv-sbt DESTINATION bin)
