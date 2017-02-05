#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

template <typename T>
std::string str(const T &val)
{
  std::ostringstream ss;
  ss << val;
  return ss.str();
}

static const size_t EI_MAG0 =       0;
static const size_t EI_MAG1 =       1;
static const size_t EI_MAG2 =       2;
static const size_t EI_MAG3 =       3;
static const size_t EI_CLASS =      4;
static const size_t EI_DATA =       5;
static const size_t EI_VERSION =    6;
static const size_t EI_OSABI =      7;
static const size_t EI_ABIVERSION = 8;
static const size_t EI_PAD =        9;

struct elf_header
{
  char e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

class elf_exception : public std::exception
{
public:
  elf_exception(const std::string &msg) noexcept :
    msg(msg)
  {}

  ~elf_exception() noexcept override = default;

  const char *what() const noexcept override
  {
    return msg.c_str();
  }

private:
  std::string msg;
};

#define EXCEPTION(msg) \
  elf_exception(str(__FUNCTION__) + ":" + str(__LINE__) + ": " + msg)


/// elf ///

class elf
{
public:
  elf(const std::string &filename);

  void dump() const;

private:
  elf_header hdr;
};


elf::elf(const std::string &filename)
{
  std::ifstream ifs(filename);
  if (!ifs)
    throw EXCEPTION("failed to open file \"" + filename + "\"");

  ifs.read(reinterpret_cast<char *>(&hdr), sizeof(hdr));
  if (ifs.fail())
    throw EXCEPTION("failed to read elf header");
}

struct hex_fmt
{
  hex_fmt(size_t width) : width(width) {}

  size_t width;
};

static hex_fmt hex(size_t width)
{
  return hex_fmt(width);
}

static std::ostream &operator<<(std::ostream &os, const hex_fmt &fmt)
{
  os  << "0x" << std::hex << std::uppercase
      << std::setw(fmt.width) << std::setfill('0');
  return os;
}

static uint32_t offs = 0;
static std::ostream &poffs(std::ostream &os)
{
  os  << hex(2) << offs << ": ";
  return os;
}

void elf::dump() const
{
  using std::cout;

  uint16_t c;
  char nl = '\n';

  // magic
  cout << poffs << "e_ident[EI_MAG0-3] =" << std::hex << std::uppercase;
  for (size_t i = 0; i < 4; i++) {
    c = hdr.e_ident[i];
    cout  << " 0x" << std::setw(2) << std::setfill('0')
          << c
          << "('" << char(isprint(c)? c : '.') << "')";
  }
  cout << std::dec << nl;
  offs += 4;

  auto desc = [](const std::string &s) {
    return " (" + s + ")";
  };

  auto ei_class_str = [](char c)
  {
    switch (c) {
      case 1: return "32-bit";
      case 2: return "64-bit";
      default: return "?";
    }
  };

  c = hdr.e_ident[EI_CLASS];
  cout  << poffs << "e_ident[EI_CLASS] = "
        << hex(2) << c << desc(ei_class_str(c))
        << nl;
  offs++;

  auto ei_data_str = [](char c) {
    switch (c) {
      case 1: return "little endian";
      case 2: return "big endian";
      default: return "?";
    }
  };

  c = hdr.e_ident[EI_DATA];
  cout << poffs << "e_ident[EI_DATA] = "
       << hex(2) << c << desc(ei_data_str(c))
       << nl;
  offs++;

  c = hdr.e_ident[EI_VERSION];
  cout  << poffs << "e_ident[EI_VERSION] = "
        << hex(2) << c << nl;
  offs++;

  c = hdr.e_ident[EI_OSABI];
  cout  << poffs << "e_ident[EI_OSABI] = "
        << hex(2) << c << nl;
  offs++;

  c = hdr.e_ident[EI_ABIVERSION];
  cout  << poffs << "e_ident[EI_ABIVERSION] = "
        << hex(2) << c << nl;
  offs++;

  cout  << poffs << "e_ident[EI_PAD] =";
  for (size_t i = EI_PAD; i < sizeof(hdr.e_ident); i++)
    cout << ' ' << hex(2) << uint16_t(hdr.e_ident[i]);
  cout << nl;
  offs += sizeof(hdr.e_ident) - EI_PAD;

  uint16_t s = hdr.e_type;
  auto e_type_str = [](uint16_t s) {
    switch (s) {
      case 1: return "relocatable";
      case 2: return "executable";
      case 3: return "shared";
      case 4: return "core";
      default: return "?";
    }
  };
  cout  << poffs << "e_type = " << hex(4) << s
        << desc(e_type_str(s)) << nl;
  offs += 2;

  s = hdr.e_machine;
  auto e_machine_str = [](uint16_t s) {
    switch (s) {
      case 0x02: return "SPARC";
      case 0x03: return "x86";
      case 0x08: return "MIPS";
      case 0x14: return "PowerPC";
      case 0x28: return "ARM";
      case 0x2A: return "SuperH";
      case 0x32: return "IA-64";
      case 0x3E: return "x86-64";
      case 0xB7: return "AArch64";
      case 0xF3: return "RISC-V";
      default: return "?";
    }
  };
  cout  << poffs << "e_machine = " << hex(4) << s
        << desc(e_machine_str(s)) << nl;
  offs += 2;

  uint32_t u = hdr.e_version;
  cout  << poffs << "e_version = " << hex(8) << u << nl;
  offs += 4;

  u = hdr.e_entry;
  cout  << poffs << "e_entry = " << hex(8) << u << nl;
  offs += 4;

  u = hdr.e_phoff;
  cout  << poffs << "e_phoff = " << hex(8) << u << nl;
  offs += 4;

  u = hdr.e_shoff;
  cout  << poffs << "e_shoff = " << hex(8) << u << nl;
  offs += 4;

  u = hdr.e_flags;
  cout  << poffs << "e_flags = " << hex(8) << u << nl;
  offs += 4;

  s = hdr.e_ehsize;
  cout  << poffs << "e_ehsize = " << std::dec << s << nl;
  offs += 2;

  s = hdr.e_phentsize;
  cout  << poffs << "e_phentsize = " << s << nl;
  offs += 2;

  s = hdr.e_phnum;
  cout  << poffs << "e_phnum = " << s << nl;
  offs += 2;

  s = hdr.e_shentsize;
  cout  << poffs << "e_shentsize = " << s << nl;
  offs += 2;

  s = hdr.e_shnum;
  cout  << poffs << "e_shnum = " << s << nl;
  offs += 2;

  s = hdr.e_shstrndx;
  cout  << poffs << "e_shstrndx = " << s << nl;
  offs += 2;
}

///

void usage()
{
  std::cout << "usage: elf <filename>" << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    usage();
    return 1;
  }

  elf elf(argv[1]);
  elf.dump();
  return 0;
}

