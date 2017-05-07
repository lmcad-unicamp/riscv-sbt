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
using ObjectPtr = Object *;
using ConstObjectPtr = const Object *;
using RelocationPtr = std::shared_ptr<Relocation>;
using ConstRelocationPtr = std::shared_ptr<const Relocation>;
using SymbolPtr = std::shared_ptr<Symbol>;
using ConstSymbolPtr = std::shared_ptr<const Symbol>;
using SectionPtr = std::shared_ptr<Section>;
using ConstSectionPtr = std::shared_ptr<const Section>;

// vectors
using SectionPtrVec = std::vector<SectionPtr>;
using ConstSymbolPtrVec = std::vector<ConstSymbolPtr>;
using ConstRelocationPtrVec = std::vector<ConstRelocationPtr>;

// maps
using NameToSymbolMap = Map<llvm::StringRef, ConstSymbolPtr>;
using NameToSectionMap = Map<llvm::StringRef, ConstSectionPtr>;
using PtrToSymbolMap = Map<uintptr_t, ConstSymbolPtr>;
using PtrToSectionMap = Map<uintptr_t, ConstSectionPtr>;


/// Section

class Section
{
public:
  Section(
    ConstObjectPtr obj,
    const std::string& name)
    :
    _obj(obj),
    _name(name)
  {}

  virtual ~Section() = default;

  // name
  const std::string& name() const
  {
    return _name;
  }

  // get llvm::object::SectionRef (if any)
  virtual const llvm::object::SectionRef section() const
  {
    return llvm::object::SectionRef();
  }

  // address
  virtual uint64_t address() const
  {
    return 0;
  }

  // size
  virtual uint64_t size() const = 0;

  // type

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

  // contents
  virtual llvm::Error contents(llvm::StringRef& s) const
  {
    s = "";
    return llvm::Error::success();
  }

  // symbols, ordered by address
  const ConstSymbolPtrVec& symbols() const
  {
    return _symbols;
  }

  // set symbols (can't be done at construction time)
  virtual void symbols(ConstSymbolPtrVec&& s)
  {
    _symbols = std::move(s);
  }

  // lookup symbol by address
  ConstSymbolPtr lookup(uint64_t addr) const;

  // ELFOffset
  virtual uint64_t getELFOffset() const
  {
    return 0;
  }

  // offset in Shadow Image
  uint64_t shadowOffs() const
  {
    return _shadowOffs;
  }

  // this needs to be set later...
  void shadowOffs(uint64_t offs) const
  {
    _shadowOffs = offs;
  }

  ConstObjectPtr object() const
  {
    return _obj;
  }

  // get string representation
  std::string str() const;

protected:
  ConstObjectPtr _obj;
  std::string _name;
  ConstSymbolPtrVec _symbols;
  mutable uint64_t _shadowOffs = 0;
};


/// CommonSection

class CommonSection : public Section
{
public:
  // ctor
  CommonSection(ConstObjectPtr obj) :
    Section(obj, ".common")
  {}

  // size
  uint64_t size() const override
  {
    return _size;
  }

  // type
  bool isCommon() const override
  {
    return true;
  }

  // set symbols
  void symbols(ConstSymbolPtrVec&& s) override;

private:
  uint64_t _size = 0;
};


/// LLVMSection

class LLVMSection : public Section
{
public:
  // ctor
  LLVMSection(
    ConstObjectPtr obj,
    llvm::object::SectionRef sec,
    llvm::Error& err);

  // get llvm::object::SectionRef
  const llvm::object::SectionRef section() const override
  {
    return _sec;
  }

  // address
  uint64_t address() const override
  {
    return _sec.getAddress();
  }

  // size
  uint64_t size() const override
  {
    return _sec.getSize();
  }

  // type

  bool isText() const override
  {
    return _sec.isText();
  }

  bool isData() const override
  {
    return _sec.isData();
  }

  bool isBSS() const override
  {
    return _sec.isBSS();
  }

  // contents
  llvm::Error contents(llvm::StringRef& s) const override
  {
    return llvm::errorCodeToError(_sec.getContents(s));
  }

  // ELFOffset
  uint64_t getELFOffset() const override;

private:
  llvm::object::SectionRef _sec;
};


/// Symbol

class Symbol
{
public:
  using Type = llvm::object::SymbolRef::Type;

  // ctor
  Symbol(
    ConstObjectPtr obj,
    llvm::object::SymbolRef sym,
    llvm::Error& err);

  // name
  const llvm::StringRef& name() const
  {
    return _name;
  }

  // type
  Type type() const
  {
    return _type;
  }

  // section
  ConstSectionPtr section() const
  {
    return _sec;
  }

  void section(ConstSectionPtr s)
  {
    _sec = s;
  }

  // address
  uint64_t address() const
  {
    return _address;
  }

  void address(uint64_t addr)
  {
    _address = addr;
  }

  // flags
  uint32_t flags() const
  {
    return _sym.getFlags();
  }

  llvm::object::SymbolRef symbol() const
  {
    return _sym;
  }

  // commonSize
  uint64_t commonSize() const
  {
    return _sym.getCommonSize();
  }

  // get string representation
  std::string str() const;

private:
  ConstObjectPtr _obj;
  llvm::object::SymbolRef _sym;
  llvm::StringRef _name;
  Type _type;
  ConstSectionPtr _sec;
  uint64_t _address = 0;
};


/// Relocation

class Relocation
{
public:
  Relocation(
    ConstObjectPtr obj,
    llvm::object::RelocationRef reloc,
    llvm::Error& err);

  // symbol
  ConstSymbolPtr symbol() const
  {
    return _sym;
  }

  // section
  ConstSectionPtr section() const
  {
    if (!symbol())
      return nullptr;
    return _sym->section();
  }

  // offset
  uint64_t offset() const
  {
    return _reloc.getOffset();
  }

  uint64_t type() const
  {
    return _reloc.getType();
  }

  // type name
  using TypeVec = llvm::SmallVector<char, 128>;
  TypeVec typeName() const
  {
    TypeVec vec;
    _reloc.getTypeName(vec);
    return vec;
  }

  // get string representation
  std::string str() const;

private:
  ConstObjectPtr _obj;
  llvm::object::RelocationRef _reloc;
  ConstSymbolPtr _sym;
};


/// Object

class Object
{
public:
  static void init();
  static void finish();

  // ctor
  Object(const llvm::StringRef& filePath, llvm::Error& err);

  // disallow copies
  Object(const Object&) = delete;
  Object& operator=(const Object&) = delete;

  // but allow move construction
  Object(Object&&) = default;

  // get sections
  const NameToSectionMap& sections() const
  {
    return _nameToSection;
  }

  // get symbols
  const NameToSymbolMap& symbols() const
  {
    return _nameToSymbol;
  }

  // get section by Name
  ConstSectionPtr lookupSection(const llvm::StringRef& name) const
  {
    const ConstSectionPtr *p = _nameToSection[name];
    if (!p)
      return ConstSectionPtr(nullptr);
    else
      return *p;
  }

  // get section by SectionRef
  ConstSectionPtr lookupSection(const llvm::object::SectionRef& s) const
  {
    const ConstSectionPtr* p = _ptrToSection[s.getRawDataRefImpl().p];
    if (!p)
      return ConstSectionPtr(nullptr);
    else
      return *p;
  }

  // get symbol by Name
  ConstSymbolPtr lookupSymbol(const llvm::StringRef& name) const
  {
    const ConstSymbolPtr *p = _nameToSymbol[name];
    if (!p)
      return ConstSymbolPtr(nullptr);
    else
      return *p;
  }

  // get symbol by SymbolRef
  ConstSymbolPtr lookupSymbol(const llvm::object::SymbolRef& s) const
  {
    const ConstSymbolPtr *p = _ptrToSymbol[s.getRawDataRefImpl().p];
    if (!p)
      return ConstSymbolPtr(nullptr);
    else
      return *p;
  }

  // sectionEnd iterator
  llvm::object::section_iterator sectionEnd() const
  {
    return _obj->section_end();
  }

  // symbolEnd iterator
  llvm::object::symbol_iterator symbolEnd() const
  {
    return _obj->symbol_end();
  }

  // name of the object file
  const llvm::StringRef &fileName() const
  {
    return _fileName;
  }

  // file format name
  llvm::StringRef fileFormatName() const
  {
    return _obj->getFileFormatName();
  }

  // relocs
  const ConstRelocationPtrVec relocs() const
  {
    return _relocs;
  }

  // dump object contents to stdout
  void dump() const;

private:
  llvm::object::OwningBinary<llvm::object::Binary> _ownBin;
  llvm::object::ObjectFile *_obj;
  llvm::StringRef _fileName;

  // maps
  NameToSymbolMap _nameToSymbol;
  NameToSectionMap _nameToSection;
  PtrToSymbolMap _ptrToSymbol;
  PtrToSectionMap _ptrToSection;

  ConstRelocationPtrVec _relocs;

  // initialize everything
  llvm::Error readSymbols();
};

} // sbt

#endif
