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
  Section(
    ConstObjectPtr Obj,
    const std::string &Name)
    :
    Obj(Obj),
    Name(Name)
  {}

  virtual ~Section() = default;

  // Name
  const std::string &name() const
  {
    return Name;
  }

  // Get llvm::object::SectionRef (if any)
  virtual const llvm::object::SectionRef section() const
  {
    return llvm::object::SectionRef();
  }

  // Address
  virtual uint64_t address() const
  {
    return 0;
  }

  // Size
  virtual uint64_t size() const = 0;

  // Type

  virtual bool isText() const
  {
    return false;
  }

  virtual bool isData() const
  {
    return false;
  }

  virtual bool isBSS() const
  {
    return false;
  }

  virtual bool isCommon() const
  {
    return false;
  }

  // Contents
  virtual std::error_code contents(llvm::StringRef &S) const
  {
    S = "";
    return std::error_code();
  }

  // Symbols, ordered by address
  const ConstSymbolPtrVec &symbols() const
  {
    return Symbols;
  }

  // Set symbols (can't be done at construction time)
  virtual void symbols(ConstSymbolPtrVec &&S)
  {
    Symbols = std::move(S);
  }

  // ELFOffset
  virtual uint64_t getELFOffset() const
  {
    return 0;
  }

  // Offset in Shadow Image
  uint64_t shadowOffs() const
  {
    return ShadowOffs;
  }

  // This needs to be set later...
  void shadowOffs(uint64_t Offs) const
  {
    ShadowOffs = Offs;
  }

  // print "Section header" (used by dump)
  static void header(llvm::raw_ostream &OS);

  // get string representation
  std::string str() const;

protected:
  ConstObjectPtr Obj;
  std::string Name;
  ConstSymbolPtrVec Symbols;
  mutable uint64_t ShadowOffs = 0;
};


class CommonSection : public Section
{
public:
  // ctor
  CommonSection(ConstObjectPtr Obj) :
    Section(Obj, ".common")
  {}

  // Size
  uint64_t size() const override
  {
    return Size;
  }

  // Type
  bool isCommon() const override
  {
    return true;
  }

  // Set symbols
  void symbols(ConstSymbolPtrVec &&S) override;

private:
  uint64_t Size = 0;
};

class LLVMSection : public Section
{
public:
  // ctor
  LLVMSection(
    ConstObjectPtr Obj,
    llvm::object::SectionRef Sec,
    llvm::Error &E);

  // Get llvm::object::SectionRef
  const llvm::object::SectionRef section() const override
  {
    return Sec;
  }

  // Address
  uint64_t address() const override
  {
    return Sec.getAddress();
  }

  // Size
  uint64_t size() const override
  {
    return Sec.getSize();
  }

  // Type

  bool isText() const override
  {
    return Sec.isText();
  }

  bool isData() const override
  {
    return Sec.isData();
  }

  bool isBSS() const override
  {
    return Sec.isBSS();
  }

  // Contents
  std::error_code contents(llvm::StringRef &S) const override
  {
    return Sec.getContents(S);
  }

  // ELFOffset
  uint64_t getELFOffset() const override;

private:
  ConstObjectPtr Obj;
  llvm::object::SectionRef Sec;
  mutable uint64_t ShadowOffs = 0;
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

  // Name
  const llvm::StringRef &name() const
  {
    return Name;
  }

  // Type
  Type type() const
  {
    return TheType;
  }

  // Section
  ConstSectionPtr section() const
  {
    return Sec;
  }

  void section(ConstSectionPtr S)
  {
    Sec = S;
  }

  // Address
  uint64_t address() const
  {
    return Address;
  }

  void address(uint64_t Addr)
  {
    Address = Addr;
  }

  // Flags
  uint32_t flags() const
  {
    return Sym.getFlags();
  }

  llvm::object::SymbolRef symbol() const
  {
    return Sym;
  }

  // CommonSize
  uint64_t commonSize() const
  {
    return Sym.getCommonSize();
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
    if (!symbol())
      return nullptr;
    return Sym->section();
  }

  // offset
  uint64_t offset() const
  {
    return Reloc.getOffset();
  }

  uint64_t type() const
  {
    return Reloc.getType();
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
