#include "Object.h"

#include "SBTError.h"

#include <llvm/Object/ELFObjectFile.h>

using namespace llvm;

namespace sbt {

// For now, only ELF 32 LE object files are supported.
typedef object::ELFObjectFile<
  object::ELFType<support::little, false>> ELFObj;

// symbol flags
class Flags
{
public:
  // symbol flags to string
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
  // all flags vector
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

// symbol type to string
static std::string getTypeStr(Symbol::Type Type)
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
  ConstObjectPtr Obj,
  object::SectionRef Sec,
  Error &E)
  :
  Obj(Obj),
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

uint64_t Section::getELFOffset() const
{
  object::DataRefImpl Impl = Sec.getRawDataRefImpl();
  auto EI = reinterpret_cast<ELFObj::Elf_Shdr *>(Impl.p);
  return EI->sh_offset;
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
      << " Name=[" << name() << "]";
  // dump symbols
  for (ConstSymbolPtr Sym : symbols())
    SS << "\n    " << Sym->str();
  SS << " }";
  return S;
}

///
/// Symbol
///

Symbol::Symbol(
  ConstObjectPtr Obj,
  object::SymbolRef Sym,
  Error &E)
  :
  Obj(Obj),
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
  std::string Prefix("Symbol(");
  Prefix += Name;
  Prefix += ")";
  SBTError SE(Prefix);

  // Get Section
  auto ExpSecI = Sym.getSection();
  if (!ExpSecI) {
    SE << "Could not get Section";
    E = error(SE);
    return;
  }
  object::section_iterator SI = ExpSecI.get();
  if (SI != Obj->sectionEnd())
    Sec = Obj->section(*SI);

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
  TheType = ExpType.get();
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
      << "], Type=[" << getTypeStr(type())
      << "], Name=[" << name()
      << "], Flags=[" << TheFlags->str(flags())
      << "] }";
  return S;
}

///
/// Relocation
///

Relocation::Relocation(
  ConstObjectPtr Obj,
  object::RelocationRef Reloc,
  Error &E)
  :
  Obj(Obj),
  Reloc(Reloc)
{
  // Symbol
  object::symbol_iterator I = Reloc.getSymbol();
  if (I != Obj->symbolEnd())
    Sym = Obj->symbol(*I);
  else
    DBGS << "Relocation: symbol info not found\n";
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
      << "], Section=[" << section()->name()
      << "], Symbol=[" << symbol()->name()
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

Object::Object(
  const StringRef &FilePath,
  Error &E)
{
  SBTError SE(FilePath);

  // Atempt to open the binary.
  auto ExpObj = object::createBinary(FilePath);
  if (!ExpObj) {
    SE << ExpObj.takeError() << "Failed to open binary file";
    E = error(SE);
    return;
  }
  OB = std::move(ExpObj.get());
  object::Binary *Bin = OB.getBinary();

  if (!ELFObj::classof(Bin)) {
    SE << "Unsupported file type";
    E = error(SE);
    return;
  }

  Obj = dyn_cast<object::ObjectFile>(Bin);
  FileName = Obj->getFileName();

  // object::ObjectFile created, now process its symbols
  E = readSymbols();
}

Error Object::readSymbols()
{
  SBTError SE(FileName);
  const Object *ConstThis = this;
  SectionPtrVec Sections;
  // TODO Speed up this by using Section address
  //      instead of Name.
  Map<StringRef, ConstSymbolPtrVec> SectionToSymbols;

  // Read Sections
  for (const object::SectionRef &S : Obj->sections()) {
    auto ExpSec = create<Section*>(ConstThis, S);
    if (!ExpSec)
      return ExpSec.takeError();
    Section *Sec = ExpSec.get();
    SectionPtr NCPtr(Sec);
    ConstSectionPtr Ptr(NCPtr);
    // add to maps
    NameToSection(Sec->name(), ConstSectionPtr(Ptr));
    PtrToSection(S.getRawDataRefImpl().p, ConstSectionPtr(Ptr));
    // add to sections vector
    Sections.push_back(NCPtr);
  }

  // Read Symbols
  for (const object::SymbolRef &S : Obj->symbols()) {
    auto ExpSym = create<Symbol*>(ConstThis, S);
    if (!ExpSym)
      return ExpSym.takeError();
    Symbol *Sym = ExpSym.get();
    ConstSymbolPtr Ptr(Sym);
    // skip debug symbols
    if (Sym->type() == object::SymbolRef::ST_Debug)
      continue;

    NameToSymbol(Sym->name(), ConstSymbolPtr(Ptr));
    PtrToSymbol(S.getRawDataRefImpl().p, ConstSymbolPtr(Ptr));

    // Add Symbol to corresponding Section vector
    if (Sym->section()) {
      StringRef SectionName = Sym->section()->name();
      ConstSymbolPtrVec *P = SectionToSymbols[SectionName];
      // New section
      if (!P)
        SectionToSymbols(SectionName, { Ptr });
      // Existing section
      else
        P->push_back(Ptr);
    }
  }

  // Set Symbols in Sections
  for (SectionPtr Sec : Sections) {
    StringRef SectionName = Sec->name();
    ConstSymbolPtrVec *Vec = SectionToSymbols[SectionName];
    if (Vec) {
      // Sort symbols by address
      std::sort(Vec->begin(), Vec->end(),
        [](const ConstSymbolPtr &S1, const ConstSymbolPtr &S2) {
          return S1->address() < S2->address();
        });
      Sec->symbols(std::move(*Vec));
    }
  }

  // Relocs
  for (const object::SectionRef &S : Obj->sections()) {
    for (auto RB = S.relocations().begin(),
         RE = S.relocations().end();
         RB != RE; ++RB)
    {
      const object::RelocationRef &R = *RB;
      auto ExpRel = create<Relocation*>(ConstThis, R);
      if (!ExpRel)
        return ExpRel.takeError();
      Relocation *Rel = ExpRel.get();
      ConstRelocationPtr Ptr(Rel);
      Relocs.push_back(Ptr);
    }
  }

  return Error::success();
}

void Object::dump() const
{
  raw_ostream &OS = outs();

  // basic info
  OS << "FileName: " << fileName() << "\n";
  OS << "FileFormat: " << fileFormatName() << "\n";

  // Sections
  Section::header(OS);
  for (ConstSectionPtr Sec : sections())
    OS << Sec->str() << "\n";

  /*
  // Symbols
  Symbol::header(OS);
  for (ConstSymbolPtr Sym : symbols())
    OS << Sym->str() << "\n";
   */

  // Relocations
  Relocation::header(OS);
  for (ConstRelocationPtr Rel : Relocs)
    OS << Rel->str() << "\n";
}

} // sbt
