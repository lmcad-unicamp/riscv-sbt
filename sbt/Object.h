#ifndef SBT_OBJECT_H
#define SBT_OBJECT_H

#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/Error.h>

#include <string>

namespace sbt {

class Object
{
public:
  Object(const std::string &File, llvm::Error &E);
  Object(const Object &) = delete;
  Object(Object &&O) = default;
  ~Object() = default;
  Object &operator=(const Object &) = delete;

  static llvm::Expected<Object> create(const std::string &File);
  llvm::Error dump() const;

private:
  llvm::object::OwningBinary<llvm::object::Binary> OB;
  llvm::object::ObjectFile *Obj;
};

} // sbt

#endif
