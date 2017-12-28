#include "ShadowImage.h"

#include "Object.h"
#include "Relocation.h"
#include "SBTError.h"

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

        Work(ConstSectionPtr sec, std::vector<uint8_t>&& vec,
             const ConstRelocationPtrVec& relocs) :
            sec(sec),
            vec(std::move(vec)),
            relocs(relocs)
        {}
    };

    std::vector<Work> workVec;
    size_t addr = 0;

    for (ConstSectionPtr sec : _obj->sections()) {
        // skip non text/data sections
        if (!sec->isText() && !sec->isData() && !sec->isBSS() && !sec->isCommon())
            continue;

        llvm::StringRef bytes;
        std::string z;
        // .bss/.common
        if (sec->isBSS() || sec->isCommon()) {
            z = std::string(sec->size(), 0);
            bytes = z;
        // others
        } else {
            // read contents
            if (sec->contents(bytes))
                XABORTF("failed to get section [{0}] contents", sec->name());
        }

        // align
        while (addr % 4 != 0)
            addr++;
        DBGF("{0}@{1:X+8}-{2:X+8}", sec->name(), addr, addr + bytes.size());
        addr += bytes.size();

        const ConstRelocationPtrVec& relocs = sec->relocs();
        llvm::GlobalVariable* gv;
        std::vector<uint8_t> vec(bytes.size());
        std::copy(bytes.begin(), bytes.end(), vec.begin());

        // check if section needs to be relocated
        // Note: text sections are relocated during translation
        if (!relocs.empty() && !sec->isText()) {
            workVec.push_back(Work(sec, std::move(vec), relocs));
            continue;
        }

        // create the ShadowImage
        llvm::Constant* cda = llvm::ConstantDataArray::get(*_ctx->ctx, vec);
        gv = new llvm::GlobalVariable(
            *_ctx->module, cda->getType(), !CONSTANT,
            llvm::GlobalValue::ExternalLinkage, cda,
            "ShadowMemory" + sec->name());
        _sections[sec->name()] = gv;
    }

    // now process sections that need relocation,
    // because they may point to other ones
    for (auto work : workVec) {
        SBTRelocation reloc(_ctx,
            work.relocs.begin(), work.relocs.end(), work.sec);
        _sections[work.sec->name()] = reloc.relocateSection(work.vec, this);
    }
}

}
