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
    ConstSymbolPtr lookupFirst(uint64_t addr) const
    {
        ConstSymbolPtrVec r = lookup(addr);
        xassert(r.size() < 2);
        if (r.empty())
            return nullptr;
        else
            return r.front();
    }

    ConstSymbolPtrVec lookup(uint64_t addr) const;

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
    enum RType : uint64_t {
        PROXY_HI        = 0xFFFF0001,
        PROXY_LO        = 0xFFFF0002,
        PROXY           = PROXY_HI,
        PROXY_PCREL_HI  = 0xFFFF0003,
        PROXY_PCREL_LO  = 0xFFFF0004,
        PROXY_PCREL     = PROXY_PCREL_HI,
        PROXY_CALL_HI   = 0xFFFF0005,
        PROXY_CALL_LO   = 0xFFFF0006,
        PROXY_CALL      = PROXY_CALL_HI
    };

    bool hasSym() const
    {
        return !!symbol();
    }

    uint64_t symAddr() const
    {
        xassert(hasSym());
        return symbol()->address();
    }

    std::string symName() const
    {
        ConstSymbolPtr sym = symbol();

        return sym? sym->name() : "<null>";
    }

    virtual ~Relocation() = default;
    virtual uint64_t type() const = 0;
    virtual std::string typeName() const = 0;
    virtual uint64_t offset() const = 0;
    virtual uint64_t addend() const = 0;
    virtual ConstSymbolPtr symbol() const = 0;
    virtual bool hasSec() const = 0;
    virtual std::string secName() const = 0;
    virtual bool isLocalFunction() const = 0;
    virtual bool isExternal() const = 0;
    virtual void validate() const {}
    std::string str() const;
};


class LLVMRelocation : public Relocation
{
public:
    LLVMRelocation(
        ConstObjectPtr obj,
        llvm::object::RelocationRef reloc);

    uint64_t type() const override
    {
        return _reloc.getType();
    }

    std::string typeName() const override
    {
        llvm::SmallVector<char, 128> vec;
        _reloc.getTypeName(vec);
        std::string s;
        llvm::raw_string_ostream ss(s);
        ss << vec;
        return s;
    }

    uint64_t offset() const override
    {
        return _reloc.getOffset();
    }

    uint64_t addend() const override;

    bool hasSec() const override
    {
        return !!section();
    }

    std::string secName() const override
    {
        ConstSectionPtr sec = section();

        return sec? sec->name() : "<null>";
    }

    bool isLocalFunction() const override;
    bool isExternal() const override;

    void validate() const override
    {
        bool isLocalFunc = isLocalFunction();
        bool isExt = isExternal();
        ConstSymbolPtr sym = symbol();
        ConstSectionPtr sec = section();

        // we need the symbol for external references
        xassert(!isExt || sym &&
                "external symbol relocation but symbol not found");

        // bounds check for non-external symbols
        xassert(!(sym && !isExt) ||
                sym->address() < sec->size() &&
                "out of bounds symbol relocation");

        xassert(sec || !isLocalFunc);
    }

    ConstSymbolPtr symbol() const override;

private:
    ConstObjectPtr _obj;
    llvm::object::RelocationRef _reloc;

    ConstSectionPtr section() const;
};


class ProxyRelocation : public Relocation
{
public:
    ProxyRelocation(
        uint64_t type,
        const std::string& typeName,
        uint64_t offset,
        uint64_t addend,
        ConstSymbolPtr sym,
        bool hasSec,
        const std::string& secName,
        bool isLocalFunc,
        bool isExt,
        const std::string& str)
        :
        _type(type),
        _typeName(typeName),
        _offset(offset),
        _addend(addend),
        _sym(sym),
        _hasSec(hasSec),
        _secName(secName),
        _isLocalFunction(isLocalFunc),
        _isExternal(isExt),
        _str(str)
    {}

    ProxyRelocation(const LLVMRelocation& llrel) :
        ProxyRelocation(
            llrel.type(),
            llrel.typeName(),
            llrel.offset(),
            llrel.addend(),
            llrel.symbol(),
            llrel.hasSec(),
            llrel.secName(),
            llrel.isLocalFunction(),
            llrel.isExternal(),
            llrel.str())
    {
        llrel.validate();
    }

    uint64_t type() const override
    {
        return _type;
    }

    std::string typeName() const override
    {
        return _typeName;
    }

    uint64_t offset() const override
    {
        return _offset;
    }

    uint64_t addend() const override
    {
        return _addend;
    }

    ConstSymbolPtr symbol() const override
    {
        return _sym;
    }

    bool hasSec() const override
    {
        return _hasSec;
    }

    std::string secName() const override
    {
        return _secName;
    }

    bool isLocalFunction() const override
    {
        return _isLocalFunction;
    }

    bool isExternal() const override
    {
        return _isExternal;
    }

    void setOffset(uint64_t offs)
    {
        _offset = offs;
    }

    void setType(uint64_t type)
    {
        _type = type;

        switch (type) {
            case PROXY_HI:
                _typeName = "PROXY_HI";
                break;
            case PROXY_LO:
                _typeName = "PROXY_LO";
                break;
            case PROXY_PCREL_HI:
                _typeName = "PROXY_PCREL_HI";
                break;
            case PROXY_PCREL_LO:
                _typeName = "PROXY_PCREL_LO";
                break;
            case PROXY_CALL_HI:
                _typeName = "PROXY_CALL_HI";
                break;
            case PROXY_CALL_LO:
                _typeName = "PROXY_CALL_LO";
                break;
            default:
                xunreachable("Invalid type");
        }
    }

    void setHiPC(uint64_t hipc)
    {
        _hipc = hipc;
    }

    uint64_t hiPC() const
    {
        return _hipc;
    }

private:
    uint64_t _type;
    std::string _typeName;
    uint64_t _offset;
    uint64_t _addend;
    ConstSymbolPtr _sym;
    bool _hasSec;
    std::string _secName;
    bool _isLocalFunction;
    bool _isExternal;
    std::string _str;
    uint64_t _hipc = 0;
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
