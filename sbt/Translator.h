#ifndef SBT_TRANSLATOR_H
#define SBT_TRANSLATOR_H

#include "Context.h"
#include "Function.h"

#include <llvm/Support/Error.h>

namespace llvm {
class GlobalVariable;
class MCAsmInfo;
class MCContext;
class MCDisassembler;
class MCInst;
class MCInstPrinter;
class MCInstrInfo;
class MCObjectFileInfo;
class MCRegisterInfo;
class MCSubtargetInfo;
class Target;
}

namespace sbt {

class Syscall;


class Translator
{
public:

    // ctor
    Translator(Context* ctx);

    // move only
    Translator(Translator&&) = default;
    Translator(const Translator&) = delete;

    ~Translator();

    // setup

    void addInputFile(const std::string& file)
    {
        _inputFiles.push_back(file);
    }

    void setOutputFile(const std::string& file)
    {
        _outputFile = file;
    }

    void setOpts(const Options& opts)
    {
        _opts = opts;
    }

    // translate input files
    llvm::Error translate();

    // import external function
    llvm::Expected<std::pair<uint64_t, std::string>>
        import(const std::string& func);

    // counters

    // (this must be called before using any counter function)
    void initCounters();

    const Function& getCycles() const
    {
        return *_getCycles;
    }

    const Function& getTime() const
    {
        return *_getTime;
    }

    const Function& getInstRet() const
    {
        return *_getInstRet;
    }

    // indirect call handler
    const Function& icaller() const
    {
        return _iCaller;
    }

    const Function& isExternal() const
    {
        return _isExternal;
    }

    // syscall handler
    Syscall& syscall();

    static bool isExternalFunc(uint64_t addr)
    {
        return addr >= FIRST_EXT_FUNC_ADDR;
    }

    const Function* sbtabort() const
    {
        return &*_sbtabort;
    }

private:
    // data

    // set by ctor
    Options _opts;
    Context* _ctx;

    // set by sbt
    std::vector<std::string> _inputFiles;
    std::string _outputFile;

    // target info
    const llvm::Target* _target;
    std::unique_ptr<const llvm::MCRegisterInfo> _mri;
    std::unique_ptr<const llvm::MCAsmInfo> _asmInfo;
    std::unique_ptr<const llvm::MCSubtargetInfo> _sti;
    std::unique_ptr<const llvm::MCObjectFileInfo> _mofi;
    std::unique_ptr<llvm::MCContext> _mc;
    std::unique_ptr<const llvm::MCDisassembler> _disAsm;
    std::unique_ptr<const llvm::MCInstrInfo> _mii;
    std::unique_ptr<llvm::MCInstPrinter> _instPrinter;

    // icaller stuff
    Function _iCaller;
    Function _isExternal;
    FunctionPtr _sbtabort;
    Map<std::string, FunctionPtr> _funMap;
    Map<uint64_t, Function*> _funcByAddr;

    // import() stuff
    static const uint64_t FIRST_EXT_FUNC_ADDR = 0xFFFF0000;
    uint64_t _extFuncAddr = FIRST_EXT_FUNC_ADDR;
    std::unique_ptr<llvm::Module> _lcModule;

    // syscall handler
    std::unique_ptr<Syscall> _sc;

    // counters
    bool _initCounters = true;
    FunctionPtr _getCycles;
    FunctionPtr _getTime;
    FunctionPtr _getInstRet;

    //
    std::unique_ptr<AddressToSource> _a2s;

    // methods

    llvm::Error start();
    llvm::Error startTarget();

    llvm::Error finish();

    // gen indirect function caller
    void genICaller();
    void genIsExternal();
};

} // sbt

#endif
