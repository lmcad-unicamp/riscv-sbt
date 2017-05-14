#include "Constants.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/FileSystem.h>

#include <cstdlib>

namespace sbt {

static std::string getLibCBC(const std::string& binName)
{
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
    std::string file = dir + "/" + binName;
    if (!llvm::sys::fs::exists(file))
      continue;

    // strip last dir
    size_t p = dir.find_last_of('/');
    if (p == std::string::npos)
      continue;

    std::string baseDir = dir.substr(0, p);
    std::string shareDir = baseDir + "/share/" + binName;
    std::string libCBC = shareDir + "/libc.bc";
    if (llvm::sys::fs::exists(libCBC))
      return libCBC;
  } while (p0 != std::string::npos);

  // Note: LIBC_BC can be NULL if it was not found above
  return "";
}


void Constants::init(llvm::LLVMContext* ctx)
{
  _ctx = ctx;
  _libCBC = getLibCBC(BIN_NAME);
  ZERO = i32(0);
}

} // sbt
