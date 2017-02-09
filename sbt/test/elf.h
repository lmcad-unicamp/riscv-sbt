#pragma once

#include <exception>
#include <memory>
#include <sstream>
#include <string>

/// str

template <typename T>
std::string str(const T &val)
{
  std::ostringstream ss;
  ss << val;
  return ss.str();
}

static const char nl = '\n';

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


#define CASE(x)  case x: return #x;

#define FLAG(x) \
  if (f & x) { \
    if (!out.empty()) \
      out += ", "; \
    out += #x; \
  }


/// elf headers

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

// e_ident
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

// ei_class
static const char EIC_32 = 1;
static const char EIC_64 = 2;

// ei_data
static const char EID_LE = 1;
static const char EID_BE = 2;

// e_type
static const uint16_t ET_RELOC  = 1;
static const uint16_t ET_EXEC   = 2;
static const uint16_t ET_SHARED = 3;
static const uint16_t ET_CORE   = 4;

// e_machine
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

// e_flags
static const uint32_t EFRV_DOUBLE_FLOAT_ABI = 0x00000004;

/// program header

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

// p_type
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

// p_flags
static const uint32_t PF_X  = 1;
static const uint32_t PF_W  = 2;
static const uint32_t PF_R  = 4;

/// section header

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

// sh_type
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

// sh_flags
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

/// symtab

struct sym
{
  uint32_t st_name;
  uint32_t st_value;
  uint32_t st_size;
  uint8_t  st_info;
  uint8_t  st_other;
  uint16_t st_shndx;
};

// st_info(bind)
static const uint8_t STB_LOCAL  = 0x0;
static const uint8_t STB_GLOBAL = 0x1;
static const uint8_t STB_WEAK   = 0x2;
static const uint8_t STB_LOOS   = 0xA;
static const uint8_t STB_XXOS   = 0xB;
static const uint8_t STB_HIOS   = 0xC;
static const uint8_t STB_LOPROC = 0xD;
static const uint8_t STB_XXPROC = 0xE;
static const uint8_t STB_HIPROC = 0xF;

// st_info(type)
static const uint8_t STT_NOTYPE   = 0x0;
static const uint8_t STT_OBJECT   = 0x1;
static const uint8_t STT_FUNC     = 0x2;
static const uint8_t STT_SECTION  = 0x3;
static const uint8_t STT_FILE     = 0x4;
static const uint8_t STT_COMMON   = 0x5;
static const uint8_t STT_TLS      = 0x6;
static const uint8_t STT_LOOS     = 0xA;
static const uint8_t STT_XXOS     = 0xB;
static const uint8_t STT_HIOS     = 0xC;
static const uint8_t STT_LOPROC   = 0xD;
static const uint8_t STT_XXPROC   = 0xE;
static const uint8_t STT_HIPROC   = 0xF;

// st_other
static const uint8_t STV_DEFAULT    = 0x0;
static const uint8_t STV_INTERNAL   = 0x1;
static const uint8_t STV_HIDDEN     = 0x2;
static const uint8_t STV_PROTECTED  = 0x3;
static const uint8_t STV_EXPORTED   = 0x4;
static const uint8_t STV_SINGLETON  = 0x5;
static const uint8_t STV_ELIMINATE  = 0x6;

// st_shndx
static const uint16_t SHN_UNDEF   = 0x0000;
static const uint16_t SHN_ABS     = 0xFFF1;
static const uint16_t SHN_COMMON  = 0xFFF2;

/// elf ///

class elf
{
public:
  elf(const std::string &filename);

  void dump() const;

private:
  // data
  std::string filename;
  elf_header hdr;
  std::unique_ptr<program_header[]> ph;
  std::unique_ptr<section_header[]> sh;
  std::unique_ptr<char[]> shstr;
  size_t shstrsz;
  mutable int strtabndx = -1;
  mutable std::unique_ptr<char[]> strtab;
  mutable uint32_t strtabsz = 0;

  // functions
  std::unique_ptr<char[]> read(uint32_t offs, uint32_t size) const;

  void dump_ph(const program_header *ph) const;
  void dump_sh(const section_header *sh) const;

  void dump_text(uint32_t offs, uint32_t size) const;
  void dump_data(uint32_t offs, uint32_t size, uint32_t doffs = -1) const;
  void dump_symtab(uint32_t offs, uint32_t size) const;

  std::string e_flags_str(uint32_t u) const;

  const char *sh_name_str(uint32_t sh_name) const;
  const char *st_name_str(uint32_t u) const;
};

