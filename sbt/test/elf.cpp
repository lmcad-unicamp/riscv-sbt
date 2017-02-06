#include <cassert>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>


/// str

template <typename T>
std::string str(const T &val)
{
  std::ostringstream ss;
  ss << val;
  return ss.str();
}


/// elf_exception

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


/// elf

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

struct program_header
{
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
};

struct section_header
{
  uint32_t sh_name;
  uint32_t sh_type;
  uint32_t sh_flags;
  uint32_t sh_addr;
  uint32_t sh_offset;
  uint32_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint32_t sh_addralign;
  uint32_t sh_entsize;
};


/// elf ///

class elf
{
public:
  elf(const std::string &filename);

  void dump() const;

private:
  std::string filename;
  elf_header hdr;
  std::unique_ptr<program_header[]> ph;
  std::unique_ptr<section_header[]> sh;
  std::unique_ptr<char[]> shstr;
  size_t shstrsz;


  void dump_ph(const program_header *ph) const;
  void dump_sh(const section_header *sh) const;

  const char *sh_name_str(uint32_t sh_name) const;
};


elf::elf(const std::string &filename) :
  filename(filename)
{
  std::ifstream ifs(filename);
  if (!ifs)
    throw EXCEPTION("failed to open file \"" + filename + "\"");

  // elf header
  ifs.read(reinterpret_cast<char *>(&hdr), sizeof(hdr));
  if (ifs.fail())
    throw EXCEPTION("failed to read elf header");

  // program headers
  ifs.seekg(hdr.e_phoff, std::ios_base::beg);
  if (ifs.fail())
    throw EXCEPTION("seek to e_phoff failed");
  assert(sizeof(program_header) == hdr.e_phentsize);
  ph.reset(new program_header[hdr.e_phnum]);
  for (uint16_t i = 0; i < hdr.e_phnum; i++) {
    ifs.read(reinterpret_cast<char *>(&ph[i]), sizeof(program_header));
    if (ifs.fail())
      throw EXCEPTION("failed to read program headers");
  }

  // section headers
  ifs.seekg(hdr.e_shoff, std::ios_base::beg);
  if (ifs.fail())
    throw EXCEPTION("seek to e_shoff failed");
  assert(sizeof(section_header) == hdr.e_shentsize);
  sh.reset(new section_header[hdr.e_shnum]);
  for (uint16_t i = 0; i < hdr.e_shnum; i++) {
    ifs.read(reinterpret_cast<char *>(&sh[i]), sizeof(section_header));
    if (ifs.fail())
      throw EXCEPTION("failed to read section headers");
  }

  // shstr
  const section_header *p = &this->sh[hdr.e_shstrndx];
  ifs.seekg(p->sh_offset, std::ios_base::beg);
  if (ifs.fail())
    throw EXCEPTION("seek to shstr failed");
  shstrsz = p->sh_size;
  shstr.reset(new char[shstrsz]);
  ifs.read(&shstr[0], shstrsz);
  if (ifs.fail())
    throw EXCEPTION("failed to read shstr");
}


// hex iomanip

struct hex_fmt
{
  hex_fmt(size_t width, bool prefix) :
    width(width),
    prefix(prefix)
  {}

  size_t width;
  bool prefix;
};

static hex_fmt hex(size_t width, bool prefix = true)
{
  return hex_fmt(width, prefix);
}

static std::ostream &operator<<(std::ostream &os, const hex_fmt &fmt)
{
  os  << (fmt.prefix? "0x" : "") << std::hex << std::uppercase
      << std::setw(fmt.width) << std::setfill('0');
  return os;
}

static uint32_t offs = 0;
static std::ostream &poffs(std::ostream &os)
{
  os  << hex(2) << offs << ": ";
  return os;
}


// description formatter

static std::string desc(const std::string &s)
{
    return " (" + s + ")";
}


/// dump

static const uint16_t EM_SPARC =    0x0002;
static const uint16_t EM_X86 =      0x0003;
static const uint16_t EM_MIPS =     0x0008;
static const uint16_t EM_POWERPC =  0x0014;
static const uint16_t EM_ARM =      0x0028;
static const uint16_t EM_SUPERH =   0x002A;
static const uint16_t EM_IA64 =     0x0032;
static const uint16_t EM_X86_64 =   0x003E;
static const uint16_t EM_AARCH64 =  0x00B7;
static const uint16_t EM_RISCV =    0x00F3;

static std::string e_machine_str(uint16_t s)
{
#define X(x)  case x: return #x;
  switch (s) {
    X(EM_SPARC)
    X(EM_X86)
    X(EM_MIPS)
    X(EM_POWERPC)
    X(EM_ARM)
    X(EM_SUPERH)
    X(EM_IA64)
    X(EM_X86_64)
    X(EM_AARCH64)
    X(EM_RISCV)
    default: return "?";
  }
#undef X
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

  for (uint16_t i = 0; i < hdr.e_phnum; i++) {
    cout << "program_header[" << i << "]" << nl;
    dump_ph(&ph[i]);
  }

  for (uint16_t i = 0; i < hdr.e_shnum; i++) {
    cout << "section_header[" << i << "]" << nl;
    dump_sh(&sh[i]);
  }
}


static const uint32_t PT_NULL =     0x00000000;
static const uint32_t PT_LOAD =     0x00000001;
static const uint32_t PT_DYNAMIC =  0x00000002;
static const uint32_t PT_INTERP =   0x00000003;
static const uint32_t PT_NOTE =     0x00000004;
static const uint32_t PT_SHLIB =    0x00000005;
static const uint32_t PT_PHDR =     0x00000006;
static const uint32_t PT_LOOS =     0x60000000;
static const uint32_t PT_HIOS =     0x6FFFFFFF;
static const uint32_t PT_LOPROC =   0x70000000;
static const uint32_t PT_HIPROC =   0x7FFFFFFF;

static std::string p_type_str(uint32_t u)
{
  switch (u) {
    case PT_NULL:     return "PT_NULL";
    case PT_LOAD:     return "PT_LOAD";
    case PT_DYNAMIC:  return "PT_DYNAMIC";
    case PT_INTERP:   return "PT_INTERP";
    case PT_NOTE:     return "PT_NOTE";
    case PT_SHLIB:    return "PT_SHLIB";
    case PT_PHDR:     return "PT_PHDR";
    default:
      if (u >= PT_LOOS && u <= PT_HIOS)
        return "PT_OS";
      else if (u >= PT_LOPROC && u <= PT_HIPROC)
        return "PT_PROC";
      else
        return "?";
  }
}


static const uint32_t PF_X  = 1;
static const uint32_t PF_W  = 2;
static const uint32_t PF_R  = 4;

static std::string p_flags_str(uint32_t s)
{
  std::string out;

  if (s & PF_R)
    out += 'R';
  if (s & PF_W)
    out += 'W';
  if (s & PF_X)
    out += 'X';

  return out;
}

void elf::dump_ph(const program_header *ph) const
{
  using std::cout;

  const char nl = '\n';
  offs = 0;

  uint32_t u = ph->p_type;
  cout  << poffs << "p_type = " << hex(8) << u
        << desc(p_type_str(u)) << nl;
  offs += 4;

  u = ph->p_offset;
  cout  << poffs << "p_offset = " << hex(8) << u << nl;
  offs += 4;

  u = ph->p_vaddr;
  cout  << poffs << "p_vaddr = " << hex(8) << u << nl;
  offs += 4;

  u = ph->p_paddr;
  cout  << poffs << "p_paddr = " << hex(8) << u << nl;
  offs += 4;

  u = ph->p_filesz;
  cout  << poffs << "p_filesz = " << std::dec << u << nl;
  offs += 4;

  u = ph->p_memsz;
  cout  << poffs << "p_memsz = " << std::dec << u << nl;
  offs += 4;

  u = ph->p_flags;
  cout  << poffs << "p_flags = " << hex(8) << u
        << desc(p_flags_str(u)) << nl;
  offs += 4;

  u = ph->p_align;
  cout  << poffs << "p_align = " << std::dec << u << nl;
  offs += 4;
}


const char *elf::sh_name_str(uint32_t sh_name) const
{
  if (sh_name < shstrsz)
    return &shstr[sh_name];
  else
    return "noname";
}

static const uint32_t SHT_NULL =          0x00000000;
static const uint32_t SHT_PROGBITS =      0x00000001;
static const uint32_t SHT_SYMTAB =        0x00000002;
static const uint32_t SHT_STRTAB =        0x00000003;
static const uint32_t SHT_RELA =          0x00000004;
static const uint32_t SHT_HASH =          0x00000005;
static const uint32_t SHT_DYNAMIC =       0x00000006;
static const uint32_t SHT_NOTE =          0x00000007;
static const uint32_t SHT_NOBITS =        0x00000008;
static const uint32_t SHT_REL =           0x00000009;
static const uint32_t SHT_SHLIB =         0x0000000A;
static const uint32_t SHT_DYNSYM =        0x0000000B;
static const uint32_t SHT_INIT_ARRAY =    0x0000000E;
static const uint32_t SHT_FINI_ARRAY =    0x0000000F;
static const uint32_t SHT_PREINIT_ARRAY = 0x00000010;
static const uint32_t SHT_GROUP =         0x00000011;
static const uint32_t SHT_SYMTAB_SHNDX =  0x00000012;
static const uint32_t SHT_NUM =           0x00000013;
static const uint32_t SHT_LOOS =          0x60000000;

static std::string sh_type_str(uint32_t t)
{
#define X(x) case x:  return #x;
  switch (t) {
    X(SHT_NULL)
    X(SHT_PROGBITS)
    X(SHT_SYMTAB)
    X(SHT_STRTAB)
    X(SHT_RELA)
    X(SHT_HASH)
    X(SHT_DYNAMIC)
    X(SHT_NOTE)
    X(SHT_NOBITS)
    X(SHT_REL)
    X(SHT_SHLIB)
    X(SHT_DYNSYM)
    X(SHT_INIT_ARRAY)
    X(SHT_FINI_ARRAY)
    X(SHT_PREINIT_ARRAY)
    X(SHT_GROUP)
    X(SHT_SYMTAB_SHNDX)
    X(SHT_NUM)
    default:
      if (t >= SHT_LOOS)
        return ">= SHT_LOOS";
      else
        return "?";
  }
#undef X
}


static const uint32_t SHF_WRITE =       0x00000001;
static const uint32_t SHF_ALLOC =       0x00000002;
static const uint32_t SHF_EXECINSTR =   0x00000004;
static const uint32_t SHF_MERGE =       0x00000010;
static const uint32_t SHF_STRINGS =     0x00000020;
static const uint32_t SHF_INFO_LINK =   0x00000040;
static const uint32_t SHF_LINK_ORDER =  0x00000080;
static const uint32_t SHF_OS_NONCONFORMING =  0x00000100;
static const uint32_t SHF_GROUP =       0x00000200;
static const uint32_t SHF_TLS =         0x00000400;
static const uint32_t SHF_MASKOS =      0x0FF00000;
static const uint32_t SHF_MASKPROC =    0xF0000000;
static const uint32_t SHF_ORDERED =     0x40000000;
static const uint32_t SHF_EXCLUDE =     0x80000000;

static std::string sh_flags_str(uint32_t f)
{
#define X(x) \
  if (f & x) { \
    if (!out.empty()) \
      out += ", "; \
    out += #x; \
  }

  std::string out;
  X(SHF_WRITE)
  X(SHF_ALLOC)
  X(SHF_EXECINSTR)
  X(SHF_MERGE)
  X(SHF_STRINGS)
  X(SHF_INFO_LINK)
  X(SHF_LINK_ORDER)
  X(SHF_OS_NONCONFORMING)
  X(SHF_GROUP)
  X(SHF_TLS)
  X(SHF_ORDERED)
  X(SHF_EXCLUDE)
  return out;
#undef X
}

static void hexdump(const char *p, size_t offs, size_t sz)
{
  using std::cout;
  const char nl = '\n';

  std::string line;
  for (size_t i = 0; i < sz; i++) {
    if (i && i % 16 == 0) {
      line = "";
      offs += 16;
    }

    if (i % 16 == 0)
      cout << hex(8, false) << offs << ": ";

    uint16_t c = p[i];
    cout << hex(2, false) << c << " ";
    line += isprint(c)? c : '.';

    if (i % 16 == 15)
      cout << "   " << line << nl;
  }

  if (sz % 16) {
    for (size_t i = 0; i < 16 - sz % 16; i++)
      cout << "   ";
    cout << "   " << line << nl;
  }
}

void elf::dump_sh(const section_header *sh) const
{
  using std::cout;

  const char nl = '\n';
  offs = 0;

  uint32_t u = sh->sh_name;
  std::string name = sh_name_str(u);
  cout  << poffs << "sh_name = " << hex(8) << u
        << desc(name) << nl;
  offs += 4;

  u = sh->sh_type;
  cout  << poffs << "sh_type = " << hex(8) << u
        << desc(sh_type_str(u)) << nl;
  offs += 4;

  u = sh->sh_flags;
  cout  << poffs << "sh_flags = " << hex(8) << u
        << desc(sh_flags_str(u)) << nl;
  offs += 4;

  u = sh->sh_addr;
  cout  << poffs << "sh_addr = " << hex(8) << u << nl;
  offs += 4;

  u = sh->sh_offset;
  cout  << poffs << "sh_offset = " << hex(8) << u << nl;
  offs += 4;

  u = sh->sh_size;
  cout  << poffs << "sh_size = " << hex(8) << u << nl;
  offs += 4;

  u = sh->sh_link;
  cout  << poffs << "sh_link = " << hex(8) << u << nl;
  offs += 4;

  u = sh->sh_info;
  cout  << poffs << "sh_info = " << hex(8) << u << nl;
  offs += 4;

  u = sh->sh_addralign;
  cout  << poffs << "sh_addralign = " << hex(8) << u << nl;
  offs += 4;

  u = sh->sh_entsize;
  cout  << poffs << "sh_entsize = " << hex(8) << u << nl;
  offs += 4;

  // XXX
  auto read = [&](uint32_t offs, uint32_t size)
  {
    std::unique_ptr<char[]> data;
    data.reset(new char[size]);

    std::ifstream ifs(filename);
    if (!ifs)
      throw EXCEPTION("failed to open file \"" + filename + "\"");
    ifs.seekg(offs, std::ios_base::beg);
    if (ifs.fail())
      throw EXCEPTION("seek to section contents failed");
    ifs.read(&data[0], size);
    if (ifs.fail())
      throw EXCEPTION("failed to read section contents");

    return data;
  };

  uint32_t offs = sh->sh_offset;
  uint32_t size = sh->sh_size;

  if (name == ".text") {
    cout << std::flush;
    std::string objdump = hdr.e_machine == EM_RISCV?
      "riscv32-unknown-elf-objdump" : "objdump";
    system((objdump + " -d " + filename + " | "
      "awk 'BEGIN { p=0; } { if (/<_start>:/) p=1; if (p) print; }'").c_str());
  } else if (name == ".data") {
    auto data = read(offs, size);
    offs = sh->sh_addr;
    // cout << "hexdump(data, " << offs << ", " << size << ")" << std::endl;
    hexdump(&data[0], offs, size);
  } else if (name == ".shstrtab") {
    auto data = read(offs, size);
    hexdump(&data[0], 0, size);
  } else if (name == ".strtab") {
    auto data = read(offs, size);
    hexdump(&data[0], 0, size);
  }
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

