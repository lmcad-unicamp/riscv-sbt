#ifndef SBT_OBJECT_H
#define SBT_OBJECT_H

#include "Map.h"
#include "Utils.h"

#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>

#include <string>

namespace sbt {

class Object;
class Relocation;
class Symbol;

/// Section
class Section
{
public:
  Section(
    const Object &Obj,
    llvm::object::SectionRef Sec,
    llvm::Error &E);

  const llvm::StringRef &name() const
  {
    return Name;
  }

  const llvm::object::SectionRef *operator->() const
  {
    return &Sec;
  }

  const llvm::object::SectionRef &section() const
  {
    return Sec;
  }

  const std::vector<Symbol *> &symbols() const
  {
    return Symbols;
  }

  static void header(llvm::raw_ostream &OS);
  std::string str() const;

private:
  llvm::object::SectionRef Sec;
  llvm::StringRef Name;
  std::vector<Symbol *> Symbols;
};

/// Symbol
class Symbol
{
public:
  Symbol(
    const Object &Obj,
    llvm::object::SymbolRef Sym,
    llvm::Error &E);

  const llvm::StringRef &name() const
  {
    return Name;
  }

  const Section *section() const
  {
    return Sec;
  }

  uint32_t flags() const
  {
    return Sym.getFlags();
  }

  void reloc(Relocation *R)
  {
    Reloc = R;
  }

  const Relocation *reloc() const
  {
    return Reloc;
  }

  uint64_t address() const
  {
    return Address;
  }

  static void header(llvm::raw_ostream &OS);
  std::string str() const;

private:
  llvm::object::SymbolRef Sym;
  llvm::StringRef Name;
  const Section *Sec = nullptr;
  uint64_t Address = 0;
  llvm::object::SymbolRef::Type Type;
  Relocation *Reloc = nullptr;
};

/// Relocation

class Relocation
{
public:
  Relocation(
    const Object &Obj,
    llvm::object::RelocationRef Reloc,
    llvm::Error &E);

  const Symbol *symbol() const
  {
    return Sym;
  }

  uint64_t offset() const
  {
    return Reloc.getOffset();
  }

  typedef llvm::SmallVector<char, 128> TypeVec;
  TypeVec typeName() const
  {
    TypeVec V;
    Reloc.getTypeName(V);
    return V;
  }

  static void header(llvm::raw_ostream &OS);
  std::string str() const;

private:
  llvm::object::RelocationRef Reloc;
  const Symbol *Sym = nullptr;
};

/// Object
class Object
{
public:
  typedef Map<llvm::StringRef, Symbol> SymbolMap;
  typedef Map<const llvm::object::SymbolRef *, std::string> SymbolPtrMap;
  typedef Map<llvm::StringRef, Section> SectionMap;
  typedef Map<uintptr_t, std::string> SectionPtrMap;

  static void init();
  static void finish();

  Object(const llvm::StringRef &File, llvm::Error &E);
  Object(const Object &) = delete;
  Object(Object &&O) = default;
  ~Object() = default;
  Object &operator=(const Object &) = delete;

  const SymbolMap &symbols() const
  {
    return Symbols;
  }

  const SectionMap &sections() const
  {
    return Sections;
  }

  const Section *getSection(const llvm::object::SectionRef *S) const
  {
    const std::string *SecName = SectionPtrs[S->getRawDataRefImpl().p];
    if (!SecName)
      return nullptr;
    return Sections[*SecName];
  }

  const Symbol *getSymbol(const llvm::object::SymbolRef *S) const
  {
    const std::string *SymName = SymbolPtrs[S];
    if (!SymName)
      return nullptr;
    return Symbols[*SymName];
  }

  llvm::object::symbol_iterator symbolEnd() const
  {
    return Obj->symbol_end();
  }

  llvm::object::section_iterator sectionEnd() const
  {
    return Obj->section_end();
  }

  const llvm::StringRef &fileName() const
  {
    return FileName;
  }

  llvm::StringRef fileFormat() const
  {
    return Obj->getFileFormatName();
  }


  void dump() const;

private:
  llvm::object::OwningBinary<llvm::object::Binary> OB;
  llvm::object::ObjectFile *Obj;
  llvm::StringRef FileName;
  SymbolMap Symbols;
  SymbolPtrMap SymbolPtrs;
  SectionMap Sections;
  SectionPtrMap SectionPtrs;
  std::vector<Relocation> Relocs;

  //
  llvm::Error readSymbols();

  /*
  const SectionPtrMap &sectionPtrs() const
  {
    return SectionPtrs;
  }

  const SymbolPtrMap &symbolPtrs() const
  {
    return SymbolPtrs;
  }
  */
};

} // sbt

#endif
