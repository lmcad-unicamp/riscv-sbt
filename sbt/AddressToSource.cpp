#include "AddressToSource.h"

#include "SBTError.h"

#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/MemoryBuffer.h>

namespace sbt {

AddressToSource::AddressToSource(const std::string& path, llvm::Error& err)
{
    if (path.empty()) {
        err = llvm::Error::success();
        return;
    }

    if (!llvm::sys::fs::exists(path)) {
        err = ERRORF("file not found: \"{0}\"", path);
        return;
    }

    auto expMemBuf = llvm::MemoryBuffer::getFile(path);
    if (!expMemBuf) {
        err = llvm::errorCodeToError(expMemBuf.getError());
        return;
    }

    auto& mb = expMemBuf.get();
    llvm::StringRef buf = mb->getBuffer();

    size_t idx = 0;
    size_t pos, n;
    size_t ob, cb, cl;
    uint64_t addr = 0;
    std::string code;
    do {
        pos = buf.find('\n', idx);

        if (pos == llvm::StringRef::npos)
            n = pos;
        else
            n = pos - idx + 1;

        llvm::StringRef ln = buf.substr(idx, n);

        ob = ln.find('[');
        cb = ln.find(']');
        cl = ln.find(':');

        // if ln is address
        if (ob == 0 &&
            cb != llvm::StringRef::npos &&
            cb - ob < 10 &&
            cl == cb + 1 &&
            cl == ln.size() - 2)
        {
            if (idx > 0) {
                DBGS << llvm::formatv("[{0:X+8}]:\n{1}", addr, code);
                _a2s.insert({addr, code});
                code.clear();
            }

            llvm::StringRef addrStr = ln.substr(ob + 1, cb - ob - 1);
            llvm::APInt res;
            if (addrStr.getAsInteger(16, res)) {
                err = ERROR("Invalid code address found on A2S file");
                return;
            }
            addr = res.getZExtValue();

        // else ln is source code
        } else {
            code += ln.str();
        }

        idx += n;
    } while (pos != llvm::StringRef::npos);

    if (!code.empty()) {
        DBGS << llvm::formatv("[{0:X+8}]:\n{1}", addr, code);
        _a2s.insert({addr, code});
    }

    err = llvm::Error::success();
}

}
