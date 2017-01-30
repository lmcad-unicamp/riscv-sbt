#ifndef SBT_CONSTANTS_H
#define SBT_CONSTANTS_H

#define SBT_DEBUG 1

#include <string>

namespace sbt {

// name of the SBT binary/executable
extern const std::string *BIN_NAME;

// (these are only the make the code easier to read)
static const bool ADD_NULL = true;
static const bool CONSTANT = true;
static const bool ERROR = true;
static const bool SIGNED = true;
static const bool VAR_ARG = true;
static const bool VOLATILE = true;

void initConstants();
void destroyConstants();

} // sbt

#endif
