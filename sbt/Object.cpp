#include "Object.h"

#include "SBTError.h"

#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/FormatVariadic.h>

#include <algorithm>

namespace sbt {

// for now, only ELF 32 LE object files are supported
using ELFObj = llvm::object::ELFObjectFile<
    llvm::object::ELFType<llvm::support::little, false /*is64*/>>;

// symbol flags
class Flags
{
public:
    // symbol flags to string
    std::string str(uint32_t flags) const
    {
        std::string flagsStr;
        for (const auto& f : _allFlags) {
            if (flags & f.first) {
                if (!flagsStr.empty())
                    flagsStr += ",";
                flagsStr += f.second;
            }
        }
        return flagsStr;
    }

private:
    // all flags vector
    using BSR = llvm::object::BasicSymbolRef;
    std::vector<std::pair<uint32_t, llvm::StringRef>> _allFlags = {
        { BSR::SF_Undefined,      "undefined" },      // Symbol is defined in another object file
        { BSR::SF_Global,         "global" },         // Global symbol
        { BSR::SF_Weak,           "weak" },           // Weak symbol
        { BSR::SF_Absolute,       "absolute" },       // Absolute symbol
        { BSR::SF_Common,         "common" },         // Symbol has common linkage
        { BSR::SF_Indirect,       "indirect" },       // Symbol is an alias to another symbol
        { BSR::SF_Exported,       "exported" },       // Symbol is visible to other DSOs
        { BSR::SF_FormatSpecific, "formatSpecific" }, // Specific to the object file format
                                                      // (e.g. section symbols)
        { BSR::SF_Thumb,          "thumb" },          // Thumb symbol in a 32-bit ARM binary
        { BSR::SF_Hidden,         "hidden" },         // Symbol has hidden visibility
        { BSR::SF_Const,          "const" },          // Symbol value is constant
        { BSR::SF_Executable,     "executable" },     // Symbol points to an executable section
                                                      // (IR only)
    };
};


// global Flags instance
static Flags* g_flags;


// symbol type to string
static std::string getTypeStr(Symbol::Type type)
{
    using SR = llvm::object::SymbolRef;
    std::string typeStr = "???";
    switch (type) {
        case SR::ST_Unknown:    typeStr = "unk";    break;
        case SR::ST_Data:       typeStr = "data";   break;
        case SR::ST_Debug:      typeStr = "dbg";    break;
        case SR::ST_File:       typeStr = "file";   break;
        case SR::ST_Function:   typeStr = "func";   break;
        case SR::ST_Other:      typeStr = "oth";    break;
    }
    return typeStr;
}


///
/// Section
///

std::string Section::str() const
{
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss  << "section:"
        << " name=\"" << name() << "\"\n";
    // dump symbols
    for (ConstSymbolPtr sym : symbols())
        ss << sym->str();
    // dump relocations
    for (ConstRelocationPtr rel : relocs())
        ss << rel->str();
    return s;
}


void CommonSection::symbols(ConstSymbolPtrVec&& s)
{
    _symbols = std::move(s);

    for (auto& s : _symbols)
        _size += s->commonSize();
}


LLVMSection::LLVMSection(
    ConstObjectPtr obj,
    llvm::object::SectionRef sec,
    llvm::Error& err)
    :
    Section(obj, ""),
    _sec(sec)
{
    // get section name
    llvm::StringRef nameRef;
    if (_sec.getName(nameRef))
        err = ERROR("failed to get section name");
    else
        _name = nameRef;
}


uint64_t LLVMSection::getELFOffset() const
{
    llvm::object::DataRefImpl Impl = _sec.getRawDataRefImpl();
    auto ei = reinterpret_cast<ELFObj::Elf_Shdr*>(Impl.p);
    return ei->sh_offset;
}


ConstSymbolPtr Section::lookup(uint64_t addr) const
{
    auto it = std::lower_bound(_symbols.begin(), _symbols.end(), addr,
        [](ConstSymbolPtr sym, uint64_t addr) { return sym->address() < addr; });
    if (it != _symbols.end() && (*it)->address() == addr)
        return *it;
    return nullptr;
}


///
/// Symbol
///

Symbol::Symbol(
    ConstObjectPtr obj,
    llvm::object::SymbolRef sym,
    llvm::Error& err)
    :
    _obj(obj),
    _sym(sym)
{
    // get name
    auto expSymName = _sym.getName();
    if (!expSymName) {
        err = ERROR("failed to get symbol name");
        return;
    }
    _name = std::move(expSymName.get());

    // serr
    std::string prefix("symbol(");
    prefix += _name;
    prefix += ")";

    // get section
    auto expSecI = sym.getSection();
    if (!expSecI) {
        err = ERRORF("{0}: could not get section", prefix);
        return;
    }
    llvm::object::section_iterator sit = expSecI.get();
    if (sit != _obj->sectionEnd())
        _sec = _obj->lookupSection(*sit);

    // address
    auto expAddr = _sym.getAddress();
    if (!expAddr) {
        err = ERRORF("{0}: could not get address", prefix);
        return;
    }
    _address = expAddr.get();

    // type
    auto expType = _sym.getType();
    if (!expType) {
        err = ERRORF("{0}: could not get type", prefix);
        return;
    }
    _type = expType.get();
}

std::string Symbol::str() const
{
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss  << "symbol:"
        << " addr=" << _address
        << ", type=" << getTypeStr(type())
        << ", name=\"" << name()
        << "\", flags=[" << g_flags->str(flags())
        << "]\n";
    return s;
}

///
/// Relocation
///

/// LLVMRelocation

LLVMRelocation::LLVMRelocation(
    ConstObjectPtr obj,
    llvm::object::RelocationRef reloc)
    :
    _obj(obj),
    _reloc(reloc)
{
}


ConstSymbolPtr LLVMRelocation::symbol() const
{
    llvm::object::symbol_iterator symit = _reloc.getSymbol();
    xassert(symit != _obj->symbolEnd());
    return _obj->lookupSymbol(*symit);
}


ConstSectionPtr LLVMRelocation::section() const
{
    // DBGF("{0:X+8}", offset());
    ConstSectionPtr sec;

    // get symbol iter
    llvm::object::symbol_iterator symit = _reloc.getSymbol();
    xassert(symit != _obj->symbolEnd());

    // get section iter
    auto expSecIt = symit->getSection();
    xassert(expSecIt);
    auto& it = expSecIt.get();

    // section in relocation: look it up
    if (it != _obj->sectionEnd()) {
        sec = _obj->lookupSection(*it);
        // we must find the section it in this case
        xassert(sec && "relocation section not found");
    // no section in relocation: get from symbol
    } else {
        ConstSymbolPtr sym = _obj->lookupSymbol(*symit);
        // we must find the symbol in this case
        xassert(sym && "no symbol neither section found for relocation!");
        sec = sym->section();
    }

    return sec;
}


uint64_t LLVMRelocation::addend() const
{
    const llvm::object::ObjectFile* obj = _obj->obj();
    xassert(ELFObj::classof(obj));
    const ELFObj* elfobj = reinterpret_cast<const ELFObj*>(obj);

    llvm::object::DataRefImpl impl = _reloc.getRawDataRefImpl();
    const ELFObj::Elf_Rela* rela = elfobj->getRela(impl);
    return rela->r_addend;
}


std::string Relocation::str() const
{
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss  << llvm::formatv(
        "reloc: offs={0:X+8}, type={1}, section=\"{2}\", symbol=\"{3}\""
        ", addend={4:X+8}",
        offset(), typeName(), secName(), symName(), addend());
    ss.flush();
    return s;
}


bool LLVMRelocation::isLocalFunction() const
{
    ConstSymbolPtr sym = symbol();
    ConstSectionPtr sec = section();

    // XXX this may be a bit of cheating
    if (sym && sym->type() == llvm::object::SymbolRef::ST_Function)
        return true;

    // If no symbol info is available, then consider the relocation as
    // referring to a function if it refers to the .text section.
    // (this won't work for programs that use .text to also store data)
    if (sec && sec->isText())
        return true;

    // For external functions sec will be null.
    // They are handled in other part of relocation code

    return false;
}


bool LLVMRelocation::isExternal() const
{
    ConstSymbolPtr sym = symbol();

    return sym && sym->address() == 0 && !section();
}


///
/// Object
///

void Object::init()
{
    g_flags = new Flags;
}


void Object::finish()
{
    delete g_flags;
}


Object::Object(
    const llvm::StringRef& filePath,
    llvm::Error& err)
{
    // atempt to open the binary
    auto expObj = llvm::object::createBinary(filePath);
    if (!expObj) {
        auto serr = SERROR(
            llvm::formatv("failed to open binary file {0}", filePath));
        serr << expObj.takeError();
        err = error(serr);
        return;
    }
    _bin = expObj.get().takeBinary();
    llvm::object::Binary* bin = &*_bin.first;

    if (!ELFObj::classof(bin)) {
        err = ERROR("unsupported file type");
        return;
    }
}


Object::Object(Object&& o) :
    _bin(std::move(o._bin)),
    _obj(nullptr)
{}


llvm::Error Object::readSymbols()
{
    xassert(!_obj && "readSymbols() was called twice or "
        "Object was moved after readSymbols() was called!");

    llvm::object::Binary* bin = &*_bin.first;
    _obj = llvm::dyn_cast<llvm::object::ObjectFile>(bin);
    _fileName = _obj->getFileName();

    SectionPtrVec sections;

    // read sections
    for (const llvm::object::SectionRef& s : _obj->sections()) {
        auto expSec = create<LLVMSection*>(this, s);
        if (!expSec)
            return expSec.takeError();
        Section *sec = expSec.get();
        SectionPtr ptr(sec);
        // add to maps
        _ptrToSection(s.getRawDataRefImpl().p, ConstSectionPtr(ptr));
        // add to sections vector
        sections.push_back(ptr);
    }

    // add common section
    SectionPtr commonSec(new CommonSection(this));
    _ptrToSection(~0ULL, ConstSectionPtr(commonSec));
    sections.push_back(commonSec);
    uint64_t commonOffs = 0;

    // read symbols
    Map<llvm::StringRef, ConstSymbolPtrVec> sectionToSymbols;
    for (const llvm::object::SymbolRef& s : _obj->symbols()) {
        auto expSym = create<Symbol*>(this, s);
        if (!expSym)
            return expSym.takeError();
        Symbol* sym = expSym.get();
        ConstSymbolPtr ptr(sym);
        // skip debug symbols
        if (sym->type() == llvm::object::SymbolRef::ST_Debug)
            continue;
        // add to maps
        _ptrToSymbol(s.getRawDataRefImpl().p, ConstSymbolPtr(ptr));

        // add symbol to corresponding section vector
        ConstSectionPtr sec = nullptr;
        // common section
        if (s.getFlags() & llvm::object::SymbolRef::SF_Common) {
            sec = commonSec;
            sym->address(commonOffs);
            sym->section(sec);
            commonOffs += sym->commonSize();
        // regular section
        } else
            sec = sym->section();

        if (sec) {
            llvm::StringRef sectionName = sec->name();
            ConstSymbolPtrVec *p = sectionToSymbols[sectionName];
            // new section
            if (!p)
                sectionToSymbols(sectionName, { ptr });
            // existing section
            else
                p->push_back(ptr);
        }
    }

    // set symbols in sections
    for (SectionPtr sec : sections) {
        llvm::StringRef sectionName = sec->name();
        ConstSymbolPtrVec *vec = sectionToSymbols[sectionName];
        if (vec) {
            // sort symbols by address
            std::sort(vec->begin(), vec->end(),
                [](const ConstSymbolPtr& s1, const ConstSymbolPtr& s2) {
                    return s1->address() < s2->address();
                });
            sec->symbols(std::move(*vec));
        }
    }

    // build section by name map
    std::map<std::string, SectionPtr> sectionByName;
    for (SectionPtr s : sections)
        sectionByName[s->name()] = s;

    // relocs
    const std::string rela = ".rela";
    for (const llvm::object::SectionRef& s : _obj->sections()) {
        // get section name
        llvm::StringRef sref;
        auto ec = s.getName(sref);
        if (ec)
            continue;
        // strip ".rela"
        std::string name = sref.str();
        if (name.find(rela) != 0)
            continue;
        name = name.substr(rela.size());
        // skip debug sections
        if (name.find(".debug") != std::string::npos)
            continue;

        // get target section
        auto it = sectionByName.find(name);
        xassert(it != sectionByName.end() &&
            "relocation section not found");
        SectionPtr targetSection = it->second;

        // add each relocation
        ConstRelocationPtrVec relocs;
        for (auto rb = s.relocations().begin(),
                 re = s.relocations().end();
                 rb != re; ++rb)
        {
            ConstRelocationPtr ptr(new LLVMRelocation(this, *rb));
            relocs.push_back(ptr);
        }
        targetSection->setRelocs(std::move(relocs));
    }

    return llvm::Error::success();
}


void Object::dump() const
{
    llvm::raw_ostream &os = llvm::outs();

    // basic info
    os << "fileName: " << fileName() << "\n";
    os << "fileFormat: " << fileFormatName() << "\n";

    // sections
    os << "sections:\n";
    for (ConstSectionPtr sec : sections())
        os << sec->str() << "\n";
}

} // sbt
