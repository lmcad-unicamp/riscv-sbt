#ifndef SBT_DEBUG
#   define SBT_DEBUG 1
#   define ENABLE_DBGS 1
#endif

// "static" part

#ifndef SBT_DEBUG_H
#define SBT_DEBUG_H

// define xassert: like assert, but enabled by SBT_DEBUG instead of NDEBUG,
// in order to make it possible to enable asserts only in SBT, but not in
// LLVM
#if SBT_DEBUG
#   ifdef NDEBUG
#       define DEF_NDEBUG
#       undef NDEBUG
#       include <cassert>
#   endif
#   define xassert(expr) \
        ((expr) \
         ? static_cast<void>(0) \
         : __assert_fail(#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__))
#   ifdef DEF_NDEBUG
#       undef DEF_NDEBUG
#       define NDEBUG
#   endif
#else
#   define xassert(expr) static_cast<void>(expr)
#endif

#   define xunreachable(msg) \
        xassert(false && msg)

namespace sbt {
extern bool g_debug;
}

#endif

// "dynamic" part

// debug stream
#undef DBGS
#undef DBGF
#if ENABLE_DBGS
#     include <llvm/Support/FormatVariadic.h>
#     define DBGS (g_debug? llvm::outs() : llvm::nulls())
#     define DBGF(...) \
        do { \
                if (g_debug) { \
                DBGS << __FUNCTION__ << "(): " << llvm::formatv(__VA_ARGS__) << '\n'; \
                DBGS.flush(); \
                } \
        } while (0)
#else
#     define DBGS llvm::nulls()
#     define DBGF(...)
#endif
