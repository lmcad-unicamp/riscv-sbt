#ifndef SBT_OBJECT_H
#define SBT_OBJECT_H

#include "Map.h"
#include "Utils.h"

#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>

#include <memory>
#include <string>
#include <vector>

namespace sbt {

// forward decls
class Object;
class Relocation;
class Section;
class Symbol;

// pointers
typedef Object *ObjectPtr;
typedef const Object *ConstObjectPtr;
typedef std::shared_ptr<Relocation> RelocationPtr;
typedef std::shared_ptr<const Relocation> ConstRelocationPtr;
typedef std::shared_ptr<Symbol> SymbolPtr;
typedef std::shared_ptr<const Symbol> ConstSymbolPtr;
typedef std::shared_ptr<Section> SectionPtr;
typedef std::shared_ptr<const Section> ConstSectionPtr;

// vectors
typedef std::vector<SectionPtr> SectionPtrVec;
typedef std::vector<ConstSymbolPtr> ConstSymbolPtrVec;
typedef std::vector<ConstRelocationPtr> ConstRelocationPtrVec;

// maps
typedef Map<llvm::StringRef, ConstSymbolPtr> NameToSymbolMap;
typedef Map<llvm::StringRef, ConstSectionPtr> NameToSectionMap;
typedef Map<uintptr_t, ConstSymbolPtr> PtrToSymbolMap;
typedef Map<uintptr_t, ConstSectionPtr> PtrToSectionMap;


/// Section
class Section
{
public:
  // ctor
  Section(
    ConstObjectPtr Obj,
    llvm::object::SectionRef Sec,
    llvm::Error &E);

  // name
  const llvm::StringRef &name() const
  {
    return Name;
  }

  // delegate to llvm::object::SectionRef
  /*
  const llvm::object::SectionRef *operator->() const
  {
    return &Sec;
  }
  */

  // get llvm::object::SectionRef
  const llvm::object::SectionRef &section() const
  {
    return Sec;
  }

  // address
  uint64_t address() const
  {
    return Sec.getAddress();
  }

  // size
  uint64_t size() const
  {
    return Sec.getSize();
  }

  bool isText() const
  {
    return Sec.isText();
  }

  std::error_code contents(llvm::StringRef &S) const
  {
    return Sec.getContents(S);
  }

  // symbols, ordered by address
  const ConstSymbolPtrVec &symbols() const
  {
    return Symbols;
  }

  // set symbols (can't be done at construction time)
  void symbols(ConstSymbolPtrVec &&S)
  {
    Symbols = std::move(S);
  }

  uint64_t getELFOffset() const;

  // print "Section header" (used by dump)
  static void header(llvm::raw_ostream &OS);

  // get string representation
  std::string str() const;

private:
  ConstObjectPtr Obj;
  llvm::object::SectionRef Sec;
  llvm::StringRef Name;
  ConstSymbolPtrVec Symbols;
};

/// Symbol
class Symbol
{
public:
  typedef llvm::object::SymbolRef::Type Type;

  // ctor
  Symbol(
    ConstObjectPtr Obj,
    llvm::object::SymbolRef Sym,
    llvm::Error &E);

  // name
  const llvm::StringRef &name() const
  {
    return Name;
  }

  // type
  Type type() const
  {
    return TheType;
  }

  // section
  ConstSectionPtr section() const
  {
    return Sec;
  }

  // address
  uint64_t address() const
  {
    return Address;
  }

  // flags
  uint32_t flags() const
  {
    return Sym.getFlags();
  }

  // print "Symbol header" (used by dump)
  static void header(llvm::raw_ostream &OS);

  // get string representation
  std::string str() const;

private:
  ConstObjectPtr Obj;
  llvm::object::SymbolRef Sym;
  llvm::StringRef Name;
  Type TheType;
  ConstSectionPtr Sec;
  uint64_t Address = 0;
};

/// Relocation
class Relocation
{
public:
  Relocation(
    ConstObjectPtr Obj,
    llvm::object::RelocationRef Reloc,
    llvm::Error &E);

  // symbol
  ConstSymbolPtr symbol() const
  {
    return Sym;
  }

  // section
  ConstSectionPtr section() const
  {
    return Sym->section();
  }

  // offset
  uint64_t offset() const
  {
    return Reloc.getOffset();
  }

  // type name
  typedef llvm::SmallVector<char, 128> TypeVec;
  TypeVec typeName() const
  {
    TypeVec V;
    Reloc.getTypeName(V);
    return V;
  }

  // print "Relocation header" (used by dump)
  static void header(llvm::raw_ostream &OS);

  // get string representation
  std::string str() const;

private:
  ConstObjectPtr Obj;
  llvm::object::RelocationRef Reloc;
  ConstSymbolPtr Sym;
};

/// Object
class Object
{
public:
  static void init();
  static void finish();

  // ctor
  Object(const llvm::StringRef &FilePath, llvm::Error &E);

  // disallow copies
  Object(const Object &) = delete;
  Object &operator=(const Object &) = delete;

  // but allow move construction
  Object(Object &&) = default;

  // get sections
  const NameToSectionMap &sections() const
  {
    return NameToSection;
  }

  // get symbols
  const NameToSymbolMap &symbols() const
  {
    return NameToSymbol;
  }

  // get section by Name
  ConstSectionPtr section(const llvm::StringRef &Name) const
  {
    const ConstSectionPtr *P = NameToSection[Name];
    if (!P)
      return ConstSectionPtr(nullptr);
    else
      return *P;
  }

  // get section by SectionRef
  ConstSectionPtr section(const llvm::object::SectionRef &S) const
  {
    const ConstSectionPtr *P = PtrToSection[S.getRawDataRefImpl().p];
    if (!P)
      return ConstSectionPtr(nullptr);
    else
      return *P;
  }

  // get symbol by Name
  ConstSymbolPtr symbol(const llvm::StringRef &Name) const
  {
    const ConstSymbolPtr *P = NameToSymbol[Name];
    if (!P)
      return ConstSymbolPtr(nullptr);
    else
      return *P;
  }

  // get symbol by SymbolRef
  ConstSymbolPtr symbol(const llvm::object::SymbolRef &S) const
  {
    const ConstSymbolPtr *P = PtrToSymbol[S.getRawDataRefImpl().p];
    if (!P)
      return ConstSymbolPtr(nullptr);
    else
      return *P;
  }

  // sectionEnd iterator
  llvm::object::section_iterator sectionEnd() const
  {
    return Obj->section_end();
  }

  // symbolEnd iterator
  llvm::object::symbol_iterator symbolEnd() const
  {
    return Obj->symbol_end();
  }

  // name of the object file
  const llvm::StringRef &fileName() const
  {
    return FileName;
  }

  // file format name
  llvm::StringRef fileFormatName() const
  {
    return Obj->getFileFormatName();
  }

  // relocs
  const ConstRelocationPtrVec relocs() const
  {
    return Relocs;
  }

  // dump object contents to stdout
  void dump() const;

private:
  llvm::object::OwningBinary<llvm::object::Binary> OB;
  llvm::object::ObjectFile *Obj;
  llvm::StringRef FileName;

  // maps
  NameToSymbolMap NameToSymbol;
  NameToSectionMap NameToSection;
  PtrToSymbolMap PtrToSymbol;
  PtrToSectionMap PtrToSection;

  ConstRelocationPtrVec Relocs;

  // initialize everything
  llvm::Error readSymbols();
};

} // sbt

#endif
