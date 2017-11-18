#include "Object.h"

#include "SBTError.h"

#include <llvm/Object/ELFObjectFile.h>

#include <algorithm>

using namespace llvm;

namespace sbt {

// for now, only ELF 32 LE object files are supported
using ELFObj = object::ELFObjectFile<
    object::ELFType<support::little, false /*is64*/>>;

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
    using BSR = object::BasicSymbolRef;
    std::vector<std::pair<uint32_t, StringRef>> _allFlags = {
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
    std::string typeStr;
    switch (type) {
        default:
        case SR::ST_Unknown:     typeStr = "unk";    break;
        case SR::ST_Data:            typeStr = "data"; break;
        case SR::ST_Debug:         typeStr = "dbg";    break;
        case SR::ST_File:            typeStr = "file"; break;
        case SR::ST_Function:    typeStr = "func"; break;
        case SR::ST_Other:         typeStr = "oth";    break;
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
    ss    << "section{"
            << " name=[" << name() << "]";
    // dump symbols
    for (ConstSymbolPtr sym : symbols())
        ss << "\n        " << sym->str();
    ss << " }";
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
    object::SectionRef sec,
    Error& err)
    :
    Section(obj, ""),
    _sec(sec)
{
    // get section name
    StringRef nameRef;
    if (_sec.getName(nameRef)) {
        SBTError serr;
        serr << "failed to get section name";
        err = error(serr);
        return;
    } else
        _name = nameRef;
}


uint64_t LLVMSection::getELFOffset() const
{
    object::DataRefImpl Impl = _sec.getRawDataRefImpl();
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
    object::SymbolRef sym,
    Error& err)
    :
    _obj(obj),
    _sym(sym)
{
    // get name
    auto expSymName = _sym.getName();
    if (!expSymName) {
        SBTError serr;
        serr << "failed to get symbol name";
        err = error(serr);
        return;
    }
    _name = std::move(expSymName.get());

    // serr
    std::string prefix("symbol(");
    prefix += _name;
    prefix += ")";
    SBTError serr(prefix);

    // get section
    auto expSecI = sym.getSection();
    if (!expSecI) {
        serr << "could not get section";
        err = error(serr);
        return;
    }
    object::section_iterator sit = expSecI.get();
    if (sit != _obj->sectionEnd())
        _sec = _obj->lookupSection(*sit);

    // address
    auto expAddr = _sym.getAddress();
    if (!expAddr) {
        serr << "could not get address";
        err = error(serr);
        return;
    }
    _address = expAddr.get();

    // type
    auto expType = _sym.getType();
    if (!expType) {
        serr << "could not get type";
        err = error(serr);
        return;
    }
    _type = expType.get();
}

std::string Symbol::str() const
{
    std::string s;
    llvm::raw_string_ostream ss(s);
    ss    << "symbol{"
            << " addr=[" << _address
            << "], type=[" << getTypeStr(type())
            << "], name=[" << name()
            << "], flags=[" << g_flags->str(flags())
            << "] }";
    return s;
}

///
/// Relocation
///

Relocation::Relocation(
    ConstObjectPtr obj,
    object::RelocationRef reloc,
    Error& err)
    :
    _obj(obj),
    _reloc(reloc)
{
    // symbol
    object::symbol_iterator it = _reloc.getSymbol();
    if (it != _obj->symbolEnd())
        _sym = _obj->lookupSymbol(*it);
}


std::string Relocation::str() const
{
    std::string s;
    raw_string_ostream ss(s);
    ss    << "reloc{"
            << " offs=[" << offset()
            << "], type=[" << typeName();
    if (section())
        ss << "], section=[" << section()->name();
    if (symbol())
        ss    << "], symbol=[" << symbol()->name();
    ss << "] }";
    return s;
}


uint64_t Relocation::addend() const
{
    const llvm::object::ObjectFile* obj = _obj->obj();
    xassert(ELFObj::classof(obj));
    const ELFObj* elfobj = reinterpret_cast<const ELFObj*>(obj);

    llvm::object::DataRefImpl impl = _reloc.getRawDataRefImpl();
    const ELFObj::Elf_Rela* rela = elfobj->getRela(impl);
    return rela->r_addend;
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
    const StringRef& filePath,
    Error& err)
{
    SBTError serr(filePath);

    // atempt to open the binary
    auto expObj = llvm::object::createBinary(filePath);
    if (!expObj) {
        serr << expObj.takeError() << "failed to open binary file";
        err = error(serr);
        return;
    }
    _bin = expObj.get().takeBinary();
    llvm::object::Binary* bin = &*_bin.first;

    if (!ELFObj::classof(bin)) {
        serr << "unsupported file type";
        err = error(serr);
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

    SBTError serr(_fileName);
    SectionPtrVec sections;

    // read sections
    for (const object::SectionRef& s : _obj->sections()) {
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
    Map<StringRef, ConstSymbolPtrVec> sectionToSymbols;
    for (const object::SymbolRef& s : _obj->symbols()) {
        auto expSym = create<Symbol*>(this, s);
        if (!expSym)
            return expSym.takeError();
        Symbol* sym = expSym.get();
        ConstSymbolPtr ptr(sym);
        // skip debug symbols
        if (sym->type() == object::SymbolRef::ST_Debug)
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
            StringRef sectionName = sec->name();
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
        StringRef sectionName = sec->name();
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

    // relocs
    for (const object::SectionRef& s : _obj->sections()) {
        for (auto rb = s.relocations().begin(),
                 re = s.relocations().end();
                 rb != re; ++rb)
        {
            const object::RelocationRef& r = *rb;
            auto expRel = create<Relocation*>(this, r);
            if (!expRel)
                return expRel.takeError();
            Relocation* rel = expRel.get();
            ConstRelocationPtr ptr(rel);
            _relocs.push_back(ptr);
        }
    }

    return Error::success();
}


void Object::dump() const
{
    raw_ostream &os = outs();

    // basic info
    os << "fileName: " << fileName() << "\n";
    os << "fileFormat: " << fileFormatName() << "\n";

    // sections
    os << "sections:\n";
    for (ConstSectionPtr Sec : sections())
        os << Sec->str() << "\n";

    // relocations
    os << "relocations:\n";
    for (ConstRelocationPtr rel : _relocs)
        os << rel->str() << "\n";
}

} // sbt
