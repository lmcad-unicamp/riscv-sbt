#ifndef SBT_OBJECT_H
#define SBT_OBJECT_H

/*
 * This module represents an object file, on top of LLVM object classes,
 * but with extra functionality, specially to lookup symbols and 'navigate'
 * through objects, sections, symbols and relocations.
 */

#include "Map.h"
#include "Utils.h"

#include <llvm/Object/ELFObjectFile.h>
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
using ObjectPtr = Object*;
using ConstObjectPtr = const Object*;
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

    // ELF offset
    virtual uint64_t getELFOffset() const
    {
        return 0;
    }

    // get section object
    ConstObjectPtr object() const
    {
        return _obj;
    }

    // relocations
    const ConstRelocationPtrVec& relocs() const
    {
        return _relocs;
    }

    void setRelocs(ConstRelocationPtrVec&& relocs)
    {
        _relocs = std::move(relocs);
    }

    // get string representation
    std::string str() const;

protected:
    ConstObjectPtr _obj;
    std::string _name;
    ConstSymbolPtrVec _symbols;
    ConstRelocationPtrVec _relocs;
};


/// CommonSection
//    This class represents the 'common' ELF section

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

    // ELF offset
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

    // value
    uint64_t value() const
    {
        return _sym.getValue();
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
        llvm::object::RelocationRef reloc);

    ConstSymbolPtr symbol() const;
    ConstSectionPtr section() const;

    uint64_t offset() const
    {
        return _reloc.getOffset();
    }

    uint64_t addend() const;

    uint64_t type() const
    {
        return _reloc.getType();
    }

    std::string typeName() const
    {
        llvm::SmallVector<char, 128> vec;
        _reloc.getTypeName(vec);
        std::string s;
        llvm::raw_string_ostream ss(s);
        ss << vec;
        return s;
    }

    // get string representation
    std::string str() const;

private:
    ConstObjectPtr _obj;
    llvm::object::RelocationRef _reloc;
};


/// Object

class Object
{
public:
    static void init();
    static void finish();

    // ctor
    Object(const llvm::StringRef& filePath, llvm::Error& err);

    // disallow copy
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    // allow move construction only
    Object(Object&&);
    Object& operator=(Object&&) = delete;

    // initialize everything
    llvm::Error readSymbols();

    // get sections
    const PtrToSectionMap& sections() const
    {
        return _ptrToSection;
    }

    // get symbols
    const PtrToSymbolMap& symbols() const
    {
        return _ptrToSymbol;
    }

    // get section by llvm SectionRef
    ConstSectionPtr lookupSection(const llvm::object::SectionRef& s) const
    {
        const ConstSectionPtr* p = _ptrToSection[s.getRawDataRefImpl().p];
        if (!p)
            return ConstSectionPtr(nullptr);
        else
            return *p;
    }

    // get symbol by llvm SymbolRef
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
    const llvm::StringRef& fileName() const
    {
        return _fileName;
    }

    // file format name
    llvm::StringRef fileFormatName() const
    {
        return _obj->getFileFormatName();
    }

    // dump object contents to stdout
    void dump() const;

    const llvm::object::ObjectFile* obj() const
    {
            return _obj;
    }

private:
    std::pair<
        std::unique_ptr<llvm::object::Binary>,
        std::unique_ptr<llvm::MemoryBuffer>>        _bin;
    llvm::object::ObjectFile* _obj = nullptr;
    llvm::StringRef _fileName;

    // maps
    PtrToSymbolMap _ptrToSymbol;
    PtrToSectionMap _ptrToSection;
};

} // sbt

#endif
