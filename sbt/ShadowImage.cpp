#include "ShadowImage.h"

#include "Object.h"
#include "Relocation.h"
#include "SBTError.h"

#include <algorithm>
#include <vector>

namespace sbt {

ShadowImage::ShadowImage(Context* ctx, const Object* obj)
    : _ctx(ctx),
      _obj(obj)
{
    build();
}


void ShadowImage::build()
{
    struct Work {
        ConstSectionPtr sec;
        std::vector<uint8_t> vec;
        const ConstRelocationPtrVec& relocs;
        llvm::GlobalVariable* gv = nullptr;

        Work(ConstSectionPtr sec, std::vector<uint8_t>&& vec,
            const ConstRelocationPtrVec& relocs)
            :
            sec(sec),
            vec(std::move(vec)),
            relocs(relocs)
        {}
    };

    std::vector<Work> workVec;
    size_t addr = 0;

    auto gvname = [](const std::string& s) {
        auto s2 = s;
        std::replace(s2.begin(), s2.end(), '.', '_');
        return "ShadowMemory" + s2;
    };

    auto toI32 = [this](llvm::GlobalVariable* gv) {
        return llvm::ConstantExpr::getPointerCast(gv, _ctx->t.i32);
    };

    for (ConstSectionPtr sec : _obj->sections()) {
        // skip non text/data sections
        if (!sec->isText() && !sec->isData() && !sec->isBSS() && !sec->isCommon())
            continue;

        llvm::StringRef bytes;
        std::string z;
        uint64_t align;
        // .bss/.common
        if (sec->isBSS() || sec->isCommon()) {
            z = std::string(sec->size(), 0);
            bytes = z;
            align = 8;
        // others
        } else {
            // read contents
            if (sec->contents(bytes))
                XABORTF("failed to get section [{0}] contents", sec->name());
            align = sec->section().getAlignment();
        }

        // align
        while (addr % align != 0)
            addr++;
        DBGF("{0}@{1:X+8}-{2:X+8}, align={3}",
                sec->name(), addr, addr + bytes.size(), align);
        addr += bytes.size();

        const ConstRelocationPtrVec& relocs = sec->relocs();
        std::vector<uint8_t> vec(bytes.size());
        std::copy(bytes.begin(), bytes.end(), vec.begin());
        llvm::Constant* cda;
        llvm::Type* aty;
        llvm::GlobalVariable* gv;
        Work* work = nullptr;

        // check if section needs to be relocated
        // Note: text sections are relocated during translation
        if (!relocs.empty() && !sec->isText()) {
            xassert(vec.size() % sizeof(uint32_t) == 0);
            uint64_t elems = vec.size() / sizeof(uint32_t);
            cda = nullptr;
            aty = llvm::ArrayType::get(_ctx->t.i32, elems);
            workVec.push_back(Work(sec, std::move(vec), relocs));
            work = &workVec.back();
        } else {
            cda = llvm::ConstantDataArray::get(*_ctx->ctx, vec);
            aty = cda->getType();
        }

        // create the ShadowImage
        gv = new llvm::GlobalVariable(
            *_ctx->module, aty, !CONSTANT,
            llvm::GlobalValue::ExternalLinkage, cda,
            gvname(sec->name()));
        gv->setAlignment(align);
        if (work)
            work->gv = gv;
        _sections[sec->name()] = toI32(gv);
    }

    // now process sections that need relocation,
    // because they may point to other ones
    for (auto work : workVec) {
        // relocate
        SBTRelocation reloc(_ctx,
            work.relocs.begin(), work.relocs.end(), work.sec);
        llvm::GlobalVariable* gv = work.gv;
        llvm::Constant* c = reloc.relocateSection(work.vec, this);
        gv->setInitializer(c);
    }
}

}
