#include "Object.h"

#include "SBTError.h"
#include "Utils.h"

#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/Error.h>

#include <map>

using namespace llvm;

namespace sbt {

class Symbol
{
public:
  Symbol(
    const std::string &Name)
    :
    Name(Name)
  {
  }

  std::string Name;
};

typedef std::map<std::string, Symbol> SymbolMap;

class ObjDumpImpl
{
public:
  ObjDumpImpl(raw_ostream &OS) :
    OS(OS),
    E(Error::success())
  {}

  ~ObjDumpImpl()
  {
    if (E)
      errs() << "Uncaught error on ObjDumpImpl\n";
  }

  Error dump(const object::SectionRef &Section);
  Error dump(const object::SymbolRef &Symbol);
  Error dumpRelocations(const object::SectionRef &Section);

  Error takeError()
  {
    return std::move(E);
  }

private:
  raw_ostream &OS;
  mutable Error E;

  // Flags
  typedef object::BasicSymbolRef BSR;
  std::vector<std::pair<uint32_t, std::string>> AllFlags = {
    { BSR::SF_Undefined,  "Undefined" },      // Symbol is defined in another object file
    { BSR::SF_Global,     "Global" },         // Global symbol
    { BSR::SF_Weak,       "Weak" },           // Weak symbol
    { BSR::SF_Absolute,   "Absolute" },       // Absolute symbol
    { BSR::SF_Common,     "Common" },         // Symbol has common linkage
    { BSR::SF_Indirect,   "Indirect" },       // Symbol is an alias to another symbol
    { BSR::SF_Exported,   "Exported" },       // Symbol is visible to other DSOs
    { BSR::SF_FormatSpecific, "FormatSpecific" }, // Specific to the object file format
                                              // (e.g. section symbols)
    { BSR::SF_Thumb,      "Thumb" },          // Thumb symbol in a 32-bit ARM binary
    { BSR::SF_Hidden,     "Hidden" },         // Symbol has hidden visibility
    { BSR::SF_Const,      "Const" },          // Symbol value is constant
    { BSR::SF_Executable, "Executable" },     // Symbol points to an executable section
                                              // (IR only)
  };

  bool getSectionName(
    const object::SectionRef &Section,
    StringRef &SectionName) const
  {
    if (Section.getName(SectionName)) {
      SBTError SE;
      SE << "Failed to get SectionName";
      E = error(SE);
      return false;
    }
    return true;
  }

  bool getSymbolName(
    const object::SymbolRef &Symbol,
    StringRef &SymbolName) const
  {
    auto Exp = Symbol.getName();
    if (!Exp) {
      SBTError SE;
      SE << "Failed to get SymbolName";
      E = error(SE);
      return false;
    }
    SymbolName = Exp.get();
    return true;
  }

  typedef llvm::object::SymbolRef SR;
  std::string getTypeStr(SR::Type Type) const
  {
    std::string TypeStr;
    switch (Type) {
      default:
      case SR::ST_Unknown:   TypeStr = "Unk";  break;
      case SR::ST_Data:      TypeStr = "Data"; break;
      case SR::ST_Debug:     TypeStr = "Dbg";  break;
      case SR::ST_File:      TypeStr = "File"; break;
      case SR::ST_Function:  TypeStr = "Func"; break;
      case SR::ST_Other:     TypeStr = "Oth";  break;
    }
    return TypeStr;
  }

  std::string getFlagsStr(uint32_t Flags) const
  {
    std::string FlagsStr;
    for (const auto &F : AllFlags) {
      if (Flags & F.first)
        FlagsStr += std::string(!FlagsStr.empty()? ",":"") + F.second;
    }
    return FlagsStr;
  }
};

Error ObjDumpImpl::dump(const object::SectionRef &Section)
{
  SBTError SE;

  StringRef SectionName;
  if (!getSectionName(Section, SectionName))
    return takeError();

  OS << "[" << SectionName << "]\n";

  return dumpRelocations(Section);
}

Error ObjDumpImpl::dumpRelocations(const object::SectionRef &Section)
{
  auto RB = Section.relocations().begin();
  auto RE = Section.relocations().end();
  if (RB != RE) {
    OS << "Relocations:\n";
    OS << "Offs\tType\tSymbol\n";
    auto SymbolEnd = Section.getObject()->symbol_end();

    for (auto R = RB; R != RE; ++R) {
      // Symbol
      StringRef SymbolName;
      auto I = R->getSymbol();
      if (I != SymbolEnd)
        if (!getSymbolName(*I, SymbolName))
          return takeError();

      uint64_t Offset = R->getOffset();
      SmallVector<char, 128> TypeName;
      R->getTypeName(TypeName);

      OS << Offset << "\t";
      OS << "[" << TypeName << "]\t";
      OS << "[" << SymbolName << "]\n";
    }
  }

  return Error::success();
}

Error ObjDumpImpl::dump(const object::SymbolRef &Symbol)
{
  SBTError SE;
  const object::ObjectFile *Obj = Symbol.getObject();
  auto SectionEnd = Obj->section_end();

  // Name
  StringRef SymbolName;
  if (!getSymbolName(Symbol, SymbolName))
    return takeError();

  // Section
  auto ExpSection = Symbol.getSection();
  if (!ExpSection) {
    SE << "Failed to get Section";
    return error(SE);
  }
  auto I = ExpSection.get();
  StringRef SectionName;
  if (I != SectionEnd) {
    const object::SectionRef &Section = *I;
    if (!getSectionName(Section, SectionName))
      return takeError();
  }

  // Address
  uint64_t Address = 999;
  auto ExpAddr = Symbol.getAddress();
  if (ExpAddr) {
    Address = ExpAddr.get();
  }

  // Type
  std::string TypeStr;
  auto ExpType = Symbol.getType();
  if (ExpType)
    TypeStr = getTypeStr(ExpType.get());
  else
    TypeStr = "Non";

  // Flags
  uint32_t Flags = Symbol.getFlags();
  std::string FlagsStr = getFlagsStr(Flags);

  // Print
  OS << Address << "\t";
  OS << TypeStr << "\t";
  if (!SectionName.empty())
    OS << SectionName;
  OS << "\t[" << SymbolName << "]\t";
  OS << "[" << FlagsStr << "]\n";

  return Error::success();
}

/// Object ///

Expected<Object> Object::create(const std::string &File)
{
  Error E = Error::success();
  Object O(File, E);
  if (E)
    return std::move(E);
  else
    return std::move(O);
}

Object::Object(const std::string &File, Error &E)
{
  SBTError SE(File);

  // Atempt to open the binary.
  auto Exp = object::createBinary(File);
  if (!Exp) {
    SE << Exp.takeError() << "Failed to open binary file";
    E = error(SE);
    return;
  }
  OB = std::move(Exp.get());
  object::Binary *Bin = OB.getBinary();

  // for now, only object files are supported
  typedef object::ELFObjectFile<
    object::ELFType<support::little, false>> ELFObj;

  if (!ELFObj::classof(Bin)) {
    SE << "Unrecognized file type";
    E = error(SE);
    return;
  }

  Obj = dyn_cast<object::ObjectFile>(Bin);
}

Error Object::dump() const
{
  StringRef FileName = Obj->getFileName();
  SBTError SE(FileName);
  raw_ostream &OS = outs();
  ObjDumpImpl OD(OS);

  // basic info
  OS << "FileName: " << FileName << "\n";
  OS << "FileFormat: " << Obj->getFileFormatName() << "\n";

  // Sections
  OS << "Sections:\n";
  Error E = Error::success();
  consumeError(std::move(E));
  for (auto Section : Obj->sections()) {
    E = OD.dump(Section);
    if (E)
      return E;
  }

  OS << "Symbols:\n";
  OS << "Addr\tType\tSect\tSymbol\tFlags\n";
  for (auto Symbol : Obj->symbols()) {
    E = OD.dump(Symbol);
    if (E)
      return E;
  }

  return Error::success();
}

} // sbt
