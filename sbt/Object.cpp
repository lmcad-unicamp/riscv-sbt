#include "Object.h"

#include "SBTError.h"

#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/FormatVariadic.h>

#include <algorithm>
#include <map>

#undef ENABLE_DBGS
#define ENABLE_DBGS 1
#include "Debug.h"

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
    DBGF("name={0}", name());

    std::string s;
    llvm::raw_string_ostream ss(s);
    ss  << "section:"
        << " name=\"" << name() << "\"\n";
    // dump symbols
    for (ConstSymbolPtr sym : symbols())
        ss << sym->str();
    // dump relocations
    for (ConstRelocationPtr rel : relocs())
        ss << rel->str() << nl;
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
    llvm::object::SectionRef sec)
    :
    Section(obj, ""),
    _sec(sec)
{
    // get section name
    llvm::StringRef nameRef;
    std::error_code ec = _sec.getName(nameRef);
    xassert(!ec && "failed to get section name");
    _name = nameRef;
}


uint64_t LLVMSection::getELFOffset() const
{
    llvm::object::DataRefImpl Impl = _sec.getRawDataRefImpl();
    auto ei = reinterpret_cast<ELFObj::Elf_Shdr*>(Impl.p);
    return ei->sh_offset;
}


ConstSymbolPtrVec Section::lookup(uint64_t addr) const
{
    ConstSymbolPtrVec ret;

    auto it = std::lower_bound(_symbols.begin(), _symbols.end(), addr,
        [](ConstSymbolPtr sym, uint64_t addr) { return sym->address() < addr; });

    for (; it != _symbols.end() && (*it)->address() == addr; ++it)
        ret.push_back(*it);
    return ret;
}


///
/// Symbol
///

Symbol::Symbol(
    ConstObjectPtr obj,
    llvm::object::SymbolRef sym)
    :
    _obj(obj),
    _sym(sym)
{
    // get name
    auto expSymName = _sym.getName();
    xassert(expSymName && "failed to get symbol name");
    _name = std::move(expSymName.get());

    // serr
    std::string prefix("symbol(");
    prefix += _name;
    prefix += ")";

    // get section
    auto expSecI = sym.getSection();
    xassert(expSecI && "could not get section");
    llvm::object::section_iterator sit = expSecI.get();
    if (sit != _obj->sectionEnd())
        _sec = _obj->lookupSection(*sit);

    // address
    auto expAddr = _sym.getAddress();
    xassert(expAddr && "could not get address");
    _address = expAddr.get();

    // type
    auto expType = _sym.getType();
    xassert(expType && "could not get type");
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
    // NOTE some relocations, such as RELAX, doesn't have associated
    // symbols/sections in them
    if (symit == _obj->symbolEnd())
        return nullptr;
    return _obj->lookupSymbol(*symit);
}


ConstSectionPtr LLVMRelocation::section() const
{
    // DBGF("{0:X+8}", offset());
    ConstSectionPtr sec;

    // get symbol iter
    llvm::object::symbol_iterator symit = _reloc.getSymbol();
    // NOTE some relocations, such as RELAX, doesn't have associated
    // symbols/sections in them
    if (symit == _obj->symbolEnd())
        return nullptr;

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
    for (const llvm::object::SectionRef s : _obj->sections()) {
        SectionPtr sec(new LLVMSection(this, s));
        ConstSectionPtr ptr(sec);
        // add to maps
        _ptrToSection.upsert(s.getRawDataRefImpl().p, ConstSectionPtr(ptr));
        // add to sections vector
        sections.push_back(sec);
    }

    // add common section
    SectionPtr commonSec(new CommonSection(this));
    _ptrToSection.upsert(~0ULL, ConstSectionPtr(commonSec));
    sections.push_back(commonSec);
    uint64_t commonOffs = 0;

    // read symbols
    std::map<std::string, ConstSymbolPtrVec> sectionToSymbols;
    for (const llvm::object::SymbolRef s : _obj->symbols()) {
        Symbol* sym = new Symbol(this, s);
        ConstSymbolPtr ptr(sym);
        // skip debug symbols
        if (sym->type() == llvm::object::SymbolRef::ST_Debug)
            continue;
        // add to maps
        _ptrToSymbol.upsert(s.getRawDataRefImpl().p, ConstSymbolPtr(ptr));

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
            std::string sectionName = sec->name();
            auto it = sectionToSymbols.find(sectionName);
            // new section
            if (it == sectionToSymbols.end()) {
                ConstSymbolPtrVec vec = { ptr };
                sectionToSymbols.insert(std::make_pair(sectionName, vec));
            // existing section
            } else
                it->second.push_back(ptr);
        }
    }

    // set symbols in sections
    for (SectionPtr sec : sections) {
        std::string sectionName = sec->name();
        auto it = sectionToSymbols.find(sectionName);
        if (it != sectionToSymbols.end()) {
            // sort symbols by address
            std::sort(it->second.begin(), it->second.end(),
                [](const ConstSymbolPtr& s1, const ConstSymbolPtr& s2) {
                    return s1->address() < s2->address();
                });
            sec->symbols(std::move(it->second));
        }
    }

    // build section by name map
    std::map<std::string, SectionPtr> sectionByName;
    for (SectionPtr s : sections)
        sectionByName[s->name()] = s;

    // relocs
    const std::string rela = ".rela";
    DBGF("Looking for relocations...");
    for (const llvm::object::SectionRef s : _obj->sections()) {
        // get section name
        llvm::StringRef sref;
        auto ec = s.getName(sref);
        if (ec) {
            DBGF("failed to get section name");
            continue;
        }
        std::string name = sref.str();
        DBGF("section={0}", name);
        // skip debug sections
        if (name.find(".debug") != std::string::npos) {
            DBGF("skipped: debug section");
            continue;
        }
        // strip ".rela"
        if (name.find(rela) == 0) {
            DBGF(".rela section found");
            name = name.substr(rela.size());
        }
        // get target section
        auto it = sectionByName.find(name);
        if (it == sectionByName.end()) {
            DBGF("target section not found");
            continue;
        }
        SectionPtr targetSection = it->second;

        // add each relocation
        auto rb = s.relocations().begin();
        auto re = s.relocations().end();
        ConstRelocationPtrVec relocs;
        for (; rb != re; ++rb) {
            ConstRelocationPtr ptr(new LLVMRelocation(this, *rb));
            relocs.push_back(ptr);
        }
        DBGF("{0} relocation(s) found", relocs.size());
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
