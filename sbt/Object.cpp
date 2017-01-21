#include "Object.h"

#include "SBTError.h"
#include "Utils.h"

#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/Error.h>

using namespace llvm;

namespace sbt {

class Flags
{
public:
  std::string str(uint32_t Flags) const
  {
    std::string FlagsStr;
    for (const auto &F : AllFlags) {
      if (Flags & F.first) {
        if (!FlagsStr.empty())
          FlagsStr += ",";
        FlagsStr += F.second;
      }
    }
    return FlagsStr;
  }

private:
  // Flags
  typedef object::BasicSymbolRef BSR;
  std::vector<std::pair<uint32_t, StringRef>> AllFlags = {
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
};

static Flags *TheFlags = nullptr;

static StringRef getTypeStr(object::SymbolRef::Type Type)
{
  typedef llvm::object::SymbolRef SR;
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


///
/// Section
///

Section::Section(
  const Object &Obj,
  object::SectionRef Sec,
  Error &E)
  :
  Sec(Sec)
{
  // Get SectionName
  if (Sec.getName(Name)) {
    SBTError SE;
    SE << "Failed to get SectionName";
    E = error(SE);
    return;
  }
}

void Section::header(llvm::raw_ostream &OS)
{
  OS << "Sections:\n";
}

std::string Section::str() const
{
  std::string S;
  llvm::raw_string_ostream SS(S);
  SS  << "Section{"
      << " Name=[" << name()
      << "] }";
  return S;
}

///
/// Symbol
///

Symbol::Symbol(
  const Object &Obj,
  object::SymbolRef Sym,
  Error &E)
  :
  Sym(Sym)
{
  // Get Name
  auto ExpSymName = Sym.getName();
  if (!ExpSymName) {
    SBTError SE;
    SE << "Failed to get SymbolName";
    E = error(SE);
    return;
  }
  Name = std::move(ExpSymName.get());

  // SE
  Twine Tw("Symbol(", Name);
  Tw.concat(")");
  SBTError SE(Tw.str());

  // Get Section
  auto ExpSecI = Sym.getSection();
  if (!ExpSecI) {
    SE << "Could not get Section";
    E = error(SE);
    return;
  }
  object::section_iterator SI = ExpSecI.get();
  if (SI != Obj.sectionEnd())
    Sec = Obj.getSection(&*SI);

  // Address
  auto ExpAddr = Sym.getAddress();
  if (!ExpAddr) {
    SE << "Could not get Address";
    E = error(SE);
    return;
  }
  Address = ExpAddr.get();

  // Type
  auto ExpType = Sym.getType();
  if (!ExpType) {
    SE << "Could not get Type";
    E = error(SE);
    return;
  }
  Type = ExpType.get();
}

void Symbol::header(llvm::raw_ostream &OS)
{
  OS << "Symbols:\n";
  // OS << "Addr\tType\tSect\tSymbol\tFlags\n";
}

std::string Symbol::str() const
{
  std::string S;
  llvm::raw_string_ostream SS(S);
  SS  << "Symbol{"
      << " Addr=[" << Address
      << "], Type=[" << getTypeStr(Type)
      << "], " << (section()? section()->str() + ", " : "")
      << "Name=[" << name()
      << "], Flags=[" << TheFlags->str(flags())
      << "] }";
  return S;
}

///
/// Relocation
///

Relocation::Relocation(
  const Object &Obj,
  object::RelocationRef Reloc,
  Error &E)
  :
  Reloc(Reloc)
{
  // Symbol
  auto I = Reloc.getSymbol();
  if (I != Obj.symbolEnd())
    Sym = Obj.getSymbol(&*I);
  else
    DBGS << "Relocation: symbol info not available\n";
}

void Relocation::header(raw_ostream &OS)
{
  OS << "Relocations:\n";
  // OS << "Offs\tType\tSymbol\n";
}

std::string Relocation::str() const
{
  std::string S;
  raw_string_ostream SS(S);
  SS  << "Reloc{"
      << " Offs=[" << offset()
      << "], Type=[" << typeName()
      << "], Symbol=[" << (symbol()? symbol()->name() : "")
      << "] }";
  return S;
}

///
/// Object
///

void Object::init()
{
  TheFlags = new Flags;
}

void Object::finish()
{
  delete TheFlags;
}

Object::Object(const StringRef &File, Error &E)
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

  // For now, only ELF object files are supported.
  typedef object::ELFObjectFile<
    object::ELFType<support::little, false>> ELFObj;

  if (!ELFObj::classof(Bin)) {
    SE << "Unsupported file type";
    E = error(SE);
    return;
  }

  Obj = dyn_cast<object::ObjectFile>(Bin);
  FileName = Obj->getFileName();

  E = readSymbols();
}

Error Object::readSymbols()
{
  SBTError SE(FileName);

  // Read Sections
  for (const object::SectionRef &S : Obj->sections()) {
    auto ExpSec = create<Section>(*this, S);
    if (!ExpSec)
      return ExpSec.takeError();
    Section &Sec = ExpSec.get();
    Sections(Sec.name(), std::move(Sec));
    SectionPtrs(S.getRawDataRefImpl().p, Sec.name());
  }

  // Read Symbols
  for (const object::SymbolRef &S : Obj->symbols()) {
    auto ExpSym = create<Symbol>(*this, S);
    if (!ExpSym)
      return ExpSym.takeError();
    Symbol &Sym = ExpSym.get();
    Symbols(Sym.name(), std::move(Sym));
    SymbolPtrs(&S, Sym.name());
  }

  // Relocs
  for (const object::SectionRef &S : Obj->sections()) {
    for (auto RB = S.relocations().begin(),
         RE = S.relocations().end();
         RB != RE; ++RB)
    {
      const object::RelocationRef &R = *RB;
      auto ExpRel = create<Relocation>(*this, R);
      if (!ExpRel)
        return ExpRel.takeError();
      Relocation &Rel = ExpRel.get();
      Relocs.emplace_back(std::move(Rel));
    }
  }

  return Error::success();
}

void Object::dump() const
{
  raw_ostream &OS = outs();

  // basic info
  OS << "FileName: " << FileName << "\n";
  OS << "FileFormat: " << Obj->getFileFormatName() << "\n";

  // Sections
  Section::header(OS);
  for (const Section &Sec : Sections)
    OS << Sec.str() << "\n";

  // Symbols
  Symbol::header(OS);
  for (const Symbol &Sym : Symbols)
    OS << Sym.str() << "\n";

  // Relocations
  Relocation::header(OS);
  for (const Relocation &Rel : Relocs)
    OS << Rel.str() << "\n";
}

} // sbt
