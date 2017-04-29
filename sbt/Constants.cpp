#include "Constants.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/FileSystem.h>

#include <cstdlib>

namespace sbt {

const std::string* BIN_NAME = nullptr;
const std::string* LIBC_BC = nullptr;

void initConstants()
{
  BIN_NAME = new std::string("riscv-sbt");

  std::string path = getenv("PATH");
  size_t p0 = 0;
  size_t p1 = 0;
  // for each PATH dir
  do {
    // find next :
    p1 = path.find_first_of(':', p0);

    // get len
    size_t len;
    if (p1 == std::string::npos)
      len = p1;
    else
      len = p1 - p0;

    // get dir
    std::string dir = path.substr(p0, len);

    // update P0
    if (p1 == std::string::npos)
      p0 = p1;
    else if (p1 == path.size() - 1)
      p0 = std::string::npos;
    else
      p0 = p1 + 1;
    if (dir.empty())
      continue;

    // check if riscv-sbt exists
    std::string file = dir + "/" + *BIN_NAME;
    if (!llvm::sys::fs::exists(file))
      continue;

    // strip last dir
    size_t p = dir.find_last_of('/');
    if (p == std::string::npos)
      continue;

    std::string baseDir = dir.substr(0, p);
    std::string shareDir = baseDir + "/share/" + *BIN_NAME;
    std::string libCBC = shareDir + "/libc.bc";
    if (llvm::sys::fs::exists(libCBC)) {
      LIBC_BC = new std::string(libCBC);
      break;
    }
  } while (p0 != std::string::npos);

  // Note: LIBC_BC can be NULL if it was not found above
}


void destroyConstants()
{
  delete BIN_NAME;
  delete LIBC_BC;
}


void initLLVMConstants(llvm::LLVMContext& ctx)
{
  Void = llvm::Type::getVoidTy(ctx);

  I8 = llvm::Type::getInt8Ty(ctx);
  I16 = llvm::Type::getInt16Ty(ctx);
  I32 = llvm::Type::getInt32Ty(ctx);

  I8Ptr = llvm::Type::getInt8PtrTy(ctx);
  I16Ptr = llvm::Type::getInt16PtrTy(ctx);
  I32Ptr = llvm::Type::getInt32PtrTy(ctx);

  VoidFun = llvm::FunctionType::get(Void, !VAR_ARG);

  ZERO = llvm::ConstantInt::get(I32, 0);
}

} // sbt
