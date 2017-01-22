#include "Utils.h"

#include "Constants.h"

#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

namespace sbt {

raw_ostream &logs(bool error)
{
  return (error? errs() : outs()) << *BIN_NAME << ": ";
}

/*
Expected<SymbolVec> getSymbolsList(
  const object::ObjectFile *Obj,
  const object::SectionRef &Section)
{
  uint64_t SectionAddr = Section.getAddress();

  std::error_code EC;
  // Make a list of all the symbols in this section.
  SymbolVec Symbols;
  for (object::SymbolRef Symbol : Obj->symbols()) {
    // skip symbols not present in this Section
    if (!Section.containsSymbol(Symbol))
      continue;

    // get only global symbols
    if (!(Symbol.getFlags() & object::SymbolRef::SF_Global))
      continue;

    // address
    auto Exp = Symbol.getAddress();
    if (!Exp)
      return Exp.takeError();
    uint64_t &Address = Exp.get();
    Address -= SectionAddr;

    // name
    auto Exp2 = Symbol.getName();
    if (!Exp2)
      return Exp2.takeError();
    StringRef Name = Exp2.get().trim(' ');
    Symbols.push_back(std::make_pair(Address, Name));
  }

  // Sort the symbols by address, just in case they didn't come in that way.
  array_pod_sort(Symbols.begin(), Symbols.end());
  return Symbols;
}
*/

}
