#include "Constants.h"

#include <llvm/Support/FileSystem.h>

#include <cstdlib>

namespace sbt {

const std::string *BIN_NAME = nullptr;
const std::string *LIBC_BC = nullptr;

void initConstants()
{
  BIN_NAME = new std::string("riscv-sbt");

  std::string Path = getenv("PATH");
  size_t P0 = 0;
  size_t P1 = 0;
  // for each PATH dir
  do {
    // get dir
    P1 = Path.find_first_of(':', P0);
    std::string Dir = Path.substr(P0, P1);
    P0 = P1;
    if (Dir.empty())
      continue;

    // check if riscv-sbt exists
    std::string File = Dir + "/" + *BIN_NAME;
    if (!llvm::sys::fs::exists(File))
      continue;

    // strip last dir
    size_t P = Dir.find_last_of('/');
    if (P == std::string::npos)
      continue;

    std::string BaseDir = Dir.substr(0, P);
    std::string ShareDir = BaseDir + "/share/" + *BIN_NAME;
    std::string LibCBC = ShareDir + "/libc.bc";
    if (llvm::sys::fs::exists(LibCBC)) {
      LIBC_BC = new std::string(LibCBC);
      break;
    }
  } while (P1 != std::string::npos);

  // Note: LIBC_BC can be NULL if it was not found above
}

void destroyConstants()
{
  delete BIN_NAME;
  delete LIBC_BC;
}

} // sbt
