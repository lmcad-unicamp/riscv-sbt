cmake_minimum_required (VERSION 3.0)
project (riscv-sbt)

# get build options from llvm-config

# CaptureStartOrSpace
set (CStOrSp "\\(^\\| \\)")
# CaptureNonSpace0ormore
set (CNSp0 "\\([^ ]*\\)")
# CaptureNonSpace1ormore
set (CNSp1 "\\([^ ]\\+\\)")
##
# FilterD
set (FilterD "s/${CStOrSp}-D[^ ]*//g")
# FilterI
set (FilterI "s/${CStOrSp}-I[^ ]*//g")
# FilterNonI
set (FilterNonI "s/${CStOrSp}-[^I][^ ]*//g")
# FilterNonD
set (FilterNonD "s/${CStOrSp}-[^D][^ ]*//g")
# FilterNewLine
set (FilterNL tr -d "\n")
# Trim
set (Trim "s/^ *//g;  s/ *$//g; s/ \\+/ /g")
# Sep
set (Sep "s/ /;/g")
# FormatLibs
set (FormatLibs "s/${CStOrSp}-l${CNSp1}/\\1\\2/g; ${Trim}; ${Sep}")

# include dirs
execute_process(
  COMMAND llvm-config --cxxflags
  COMMAND sed "s/${CStOrSp}-I${CNSp1}/\\1\\2/g; ${FilterNonI}; ${Trim}; ${Sep}"
  COMMAND ${FilterNL}
  RESULT_VARIABLE RC1
  OUTPUT_VARIABLE SBT_INCLUDE_DIRS
)

# compile options
execute_process(
  COMMAND llvm-config --cxxflags
  COMMAND sed "${FilterD}; ${FilterI}; ${Trim}; ${Sep}"
  COMMAND ${FilterNL}
  RESULT_VARIABLE RC2
  OUTPUT_VARIABLE SBT_COMPILE_OPTIONS
)

set (DefsSed "s/${CStOrSp}-D${CNSp1}/\\1\\2/g; ${FilterNonD}; ${Trim}; ${Sep}; "
  "s/NDEBUG;\\?//")

# compile defs
execute_process(
  COMMAND llvm-config --cxxflags
  COMMAND sed "${DefsSed}"
  COMMAND ${FilterNL}
  RESULT_VARIABLE RC3
  OUTPUT_VARIABLE SBT_COMPILE_DEFINITIONS
)

# link dirs
execute_process(
  COMMAND llvm-config --ldflags
  COMMAND sed "s/-L//g; ${Trim}; ${Sep}"
  COMMAND ${FilterNL}
  RESULT_VARIABLE RC4
  OUTPUT_VARIABLE SBT_LINK_DIRS
)

# sys libs
execute_process(
  COMMAND llvm-config --system-libs
  COMMAND sed "${FormatLibs}"
  COMMAND ${FilterNL}
  RESULT_VARIABLE RC5
  OUTPUT_VARIABLE SBT_SYS_LIBS
)

# libs
execute_process(
  COMMAND llvm-config --libs arm bitwriter core object riscv support target x86
  COMMAND sed "${FormatLibs}"
  COMMAND ${FilterNL}
  RESULT_VARIABLE RC6
  OUTPUT_VARIABLE SBT_LIBS
)

# abort if llvm-config failed
if (RC1 OR RC2 OR RC3 OR RC4 OR RC5 OR RC6)
  message(FATAL_ERROR "Failed to execute llvm-config")
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
  #  set (SBT_COMPILE_OPTIONS ${SBT_COMPILE_OPTIONS} -g -O0 -rdynamic)
  set (SBT_INCLUDE_DIRS ${SBT_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/../../llvm/debug)
  set (CMAKE_VERBOSE_MAKEFILE ON)
  message(STATUS "SBT_INCLUDE_DIRS = [${SBT_INCLUDE_DIRS}]")
  message(STATUS "SBT_COMPILE_OPTIONS = [${SBT_COMPILE_OPTIONS}]")
  message(STATUS "SBT_COMPILE_DEFINITIONS = [${SBT_COMPILE_DEFINITIONS}]")
  message(STATUS "SBT_LINK_DIRS = [${SBT_LINK_DIRS}]")
  message(STATUS "SBT_SYS_LIBS = [${SBT_SYS_LIBS}]")
  message(STATUS "SBT_LIBS = [${SBT_LIBS}]")
else()
  set (SBT_COMPILE_DEFINITIONS ${SBT_COMPILE_DEFINITIONS} NDEBUG)
  set (SBT_INCLUDE_DIRS ${SBT_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/../../llvm/release)
endif()

# enable AT&T assembler
enable_language (ASM-ATT)
set (can_use_assembler TRUE)

# x86 syscall code
add_library (x86_syscall OBJECT x86_syscall.s)
target_compile_options (x86_syscall PRIVATE --32)

# SBT
include_directories (${SBT_INCLUDE_DIRS})
link_directories (${SBT_LINK_DIRS})
add_executable (riscv-sbt
  Constants.cpp
  Object.cpp
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

# install
install (TARGETS riscv-sbt DESTINATION bin)
