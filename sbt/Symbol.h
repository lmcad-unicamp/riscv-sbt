#ifndef SBT_SYMBOL_H
#define SBT_SYMBOL_H

#include "Object.h"

#include <cstdint>
#include <string>

namespace sbt {

class SBTSymbol
{
public:
  uint64_t addr;
  uint64_t val;
  std::string name;
  ConstSectionPtr sec;


  SBTSymbol(
    uint64_t addr,
    uint64_t val,
    const std::string& name,
    ConstSectionPtr sec)
    :
    addr(addr),
    val(val),
    name(name),
    sec(sec)
  {}


  bool isExternal() const
  {
    return addr == 0 && !sec;
  }
};

}

#endif
