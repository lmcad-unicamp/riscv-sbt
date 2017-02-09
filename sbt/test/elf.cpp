#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "elf.h"

using std::cout;

/// utilities

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

// print offset

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

// hexdump

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

    char c = p[i];
    cout << hex(2, false) << (uint16_t(c) & 0xFF) << " ";
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


/// elf

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


/// dump

#define ST hdr.
#define ID(x) x

#define P(field, var, fmt, pdesc) \
  var = ID(ST)field; \
  cout << poffs << #field " = " << fmt; \
  if (sizeof(var) == 1) \
    cout << (uint16_t(var) & 0xFF); \
  else \
    cout << var; \
  cout << pdesc << nl; \
  offs += sizeof(var);

#define PX(field, var) \
  P(field, var, hex(sizeof(var)*2), "")

#define PXS(field, var) \
  P(field, var, hex(sizeof(var)*2), desc(field##_str(var)))

#define PD(field, var) \
  P(field, var, std::dec, "")

#define PDS(field, var) \
  P(field, var, std::dec, desc(field##_str(var)))


// elf header

static std::string e_machine_str(uint16_t s)
{
  switch (s) {
    CASE(EM_SPARC)
    CASE(EM_X86)
    CASE(EM_MIPS)
    CASE(EM_POWERPC)
    CASE(EM_ARM)
    CASE(EM_SUPERH)
    CASE(EM_IA64)
    CASE(EM_X86_64)
    CASE(EM_AARCH64)
    CASE(EM_RISCV)
    default: return "?";
  }
}

static std::string ei_class_str(char c)
{
  switch (c) {
    case EIC_32:  return "32-bit";
    case EIC_64:  return "64-bit";
    default:      return "?";
  }
}

static std::string ei_data_str(char c)
{
  switch (c) {
    case EID_LE:  return "little endian";
    case EID_BE:  return "big endian";
    default:      return "?";
  }
}

static std::string e_type_str(uint16_t s)
{
  switch (s) {
    case ET_RELOC:  return "relocatable";
    case ET_EXEC:   return "executable";
    case ET_SHARED: return "shared";
    case ET_CORE:   return "core";
    default:        return "?";
  }
}

std::string elf::e_flags_str(uint32_t f) const
{
  std::string out;
  if (hdr.e_machine == EM_RISCV) {
    FLAG(EFRV_DOUBLE_FLOAT_ABI)
  }
  return out;
}

void elf::dump() const
{
  uint16_t c;

  // e_ident

  // magic
  cout << poffs << "e_ident[EI_MAG0-3] =";
  for (size_t i = 0; i < 4; i++) {
    c = hdr.e_ident[i];
    cout  << " " << hex(2) << c
          << "('" << char(isprint(c)? c : '.') << "')";
  }
  cout << nl;
  offs += 4;

  c = hdr.e_ident[EI_CLASS];
  cout  << poffs << "e_ident[EI_CLASS] = "
        << hex(2) << c << desc(ei_class_str(c))
        << nl;
  offs++;

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

  uint16_t s;
  uint32_t u;

  PXS(e_type, s)
  PXS(e_machine, s)
  PX(e_version, u)
  PX(e_entry, u)
  PX(e_phoff, u)
  PX(e_shoff, u)
  PXS(e_flags, u)
  PD(e_ehsize, s)
  PD(e_phentsize, s)
  PD(e_phnum, s)
  PD(e_shentsize, s)
  PD(e_shnum, s)
  PD(e_shstrndx, s)

  for (uint16_t i = 0; i < hdr.e_phnum; i++) {
    cout << "program_header[" << i << "]" << nl;
    dump_ph(&ph[i]);
  }

  for (uint16_t i = 0; i < hdr.e_shnum; i++) {
    cout << "section_header[" << i << "]" << nl;
    dump_sh(&sh[i]);
  }
}

// program header

static std::string p_type_str(uint32_t u)
{
  switch (u) {
    CASE(PT_NULL)
    CASE(PT_LOAD)
    CASE(PT_DYNAMIC)
    CASE(PT_INTERP)
    CASE(PT_NOTE)
    CASE(PT_SHLIB)
    CASE(PT_PHDR)
    default:
      if (u >= PT_LOOS && u <= PT_HIOS)
        return "PT_OS";
      else if (u >= PT_LOPROC && u <= PT_HIPROC)
        return "PT_PROC";
      else
        return "?";
  }
}

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

#undef ST
#define ST ph->

void elf::dump_ph(const program_header *ph) const
{
  offs = 0;
  uint32_t u;

  PXS(p_type, u)
  PX(p_offset, u)
  PX(p_vaddr, u)
  PX(p_paddr, u)
  PD(p_filesz, u)
  PD(p_memsz, u)
  PXS(p_flags, u)
  PD(p_align, u)
}


const char *elf::sh_name_str(uint32_t sh_name) const
{
  if (sh_name < shstrsz)
    return &shstr[sh_name];
  else
    return "noname";
}


static std::string sh_type_str(uint32_t t)
{
  switch (t) {
    CASE(SHT_NULL)
    CASE(SHT_PROGBITS)
    CASE(SHT_SYMTAB)
    CASE(SHT_STRTAB)
    CASE(SHT_RELA)
    CASE(SHT_HASH)
    CASE(SHT_DYNAMIC)
    CASE(SHT_NOTE)
    CASE(SHT_NOBITS)
    CASE(SHT_REL)
    CASE(SHT_SHLIB)
    CASE(SHT_DYNSYM)
    CASE(SHT_INIT_ARRAY)
    CASE(SHT_FINI_ARRAY)
    CASE(SHT_PREINIT_ARRAY)
    CASE(SHT_GROUP)
    CASE(SHT_SYMTAB_SHNDX)
    CASE(SHT_NUM)
    default:
      if (t >= SHT_LOOS)
        return ">= SHT_LOOS";
      else
        return "?";
  }
}


static std::string sh_flags_str(uint32_t f)
{
  std::string out;
  FLAG(SHF_WRITE)
  FLAG(SHF_ALLOC)
  FLAG(SHF_EXECINSTR)
  FLAG(SHF_MERGE)
  FLAG(SHF_STRINGS)
  FLAG(SHF_INFO_LINK)
  FLAG(SHF_LINK_ORDER)
  FLAG(SHF_OS_NONCONFORMING)
  FLAG(SHF_GROUP)
  FLAG(SHF_TLS)
  FLAG(SHF_ORDERED)
  FLAG(SHF_EXCLUDE)
  return out;
}

std::unique_ptr<char[]> elf::read(uint32_t offs, uint32_t size) const
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
}

#undef ST
#define ST sh->

void elf::dump_sh(const section_header *sh) const
{
  offs = 0;
  uint32_t u;

  // dump fields
  PXS(sh_name, u)
  PXS(sh_type, u)
  PXS(sh_flags, u)
  PX(sh_addr, u)
  PX(sh_offset, u)
  PD(sh_size, u)
  PX(sh_link, u)
  PX(sh_info, u)
  PX(sh_addralign, u)
  PD(sh_entsize, u)

  // dump contents
  uint32_t offs = sh->sh_offset;
  uint32_t size = sh->sh_size;
  std::string name = sh_name_str(sh->sh_name);

  if (name == ".text")
    dump_text(offs, size);
  else if (name == ".data")
    dump_data(offs, size, sh->sh_addr);
  else if (sh->sh_type == SHT_SYMTAB)
    dump_symtab(offs, size);
  else
    dump_data(offs, size);
}


void elf::dump_text(uint32_t offs, uint32_t size) const
{
  cout << std::flush;
  std::string objdump = hdr.e_machine == EM_RISCV?
    "riscv32-unknown-elf-objdump" : "objdump";
  system((objdump + " -d " + filename + " | "
    "awk 'BEGIN { p=0; } { if (/<_start>:/) p=1; if (p) print; }'").c_str());
}

void elf::dump_data(uint32_t offs, uint32_t size, uint32_t doffs) const
{
  auto data = read(offs, size);
  if (doffs == static_cast<uint32_t>(-1))
    doffs = offs;
  hexdump(&data[0], doffs, size);
}


#undef ST
#define ST sym->

const char *elf::st_name_str(uint32_t u) const
{
  if (u < strtabsz)
    return &strtab[u];
  else
    return "noname";

}

static std::string bind_str(uint8_t u)
{
  switch (u) {
    CASE(STB_LOCAL)
    CASE(STB_GLOBAL)
    CASE(STB_WEAK)
    CASE(STB_LOOS)
    CASE(STB_XXOS)
    CASE(STB_HIOS)
    CASE(STB_LOPROC)
    CASE(STB_XXPROC)
    CASE(STB_HIPROC)
    default: return "?";
  }
}

static std::string type_str(uint8_t u)
{
  switch (u) {
    CASE(STT_NOTYPE)
    CASE(STT_OBJECT)
    CASE(STT_FUNC)
    CASE(STT_SECTION)
    CASE(STT_FILE)
    CASE(STT_COMMON)
    CASE(STT_TLS)
    CASE(STT_LOOS)
    CASE(STT_XXOS)
    CASE(STT_HIOS)
    CASE(STT_LOPROC)
    CASE(STT_XXPROC)
    CASE(STT_HIPROC)
    default: return "?";
  }
}

static std::string vis_str(uint8_t u)
{
  switch (u) {
    CASE(STV_DEFAULT)
    CASE(STV_INTERNAL)
    CASE(STV_HIDDEN)
    CASE(STV_PROTECTED)
    CASE(STV_EXPORTED)
    CASE(STV_SINGLETON)
    CASE(STV_ELIMINATE)
    default: return "?";
  }
}

static std::string st_shndx_str(uint16_t u)
{
  switch (u) {
    CASE(SHN_UNDEF)
    CASE(SHN_ABS)
    CASE(SHN_COMMON)
    default: return "";
  }
}

void elf::dump_symtab(uint32_t soffs, uint32_t ssize) const
{
  if (strtabndx == -1) {
    // find .strtab
    for (int i = 0; strtabndx == -1 && i < hdr.e_shnum; i++)
      if (sh_name_str(sh[i].sh_name) == std::string(".strtab"))
        strtabndx = i;

    assert(strtabndx != -1);

    // load .strtab
    const section_header *strtab_sh = &sh[strtabndx];
    strtabsz = strtab_sh->sh_size;
    strtab = read(strtab_sh->sh_offset, strtabsz);
  }

  auto data = read(soffs, ssize);
  const struct sym *syms = reinterpret_cast<const struct sym *>(&data[0]);
  assert(ssize % sizeof(struct sym) == 0);
  size_t n = ssize / sizeof(struct sym);

  cout << "Symbols:" << nl;
  uint8_t  c;
  uint16_t s;
  uint32_t u;
  for (size_t i = 0; i < n; i++) {
    const struct sym *sym = &syms[i];
    offs = 0;

    PXS(st_name, u)
    PX(st_value, u)
    PD(st_size, u)

    // st_info
    c = sym->st_info;
    uint8_t bind = c >> 4;
    uint8_t type = c & 0xF;

    cout << poffs << "st_info = "
         << "bind=" << hex(2) << uint16_t(bind) << desc(bind_str(bind))
         << ", type=" << hex(2) << uint16_t(type) << desc(type_str(type))
         << nl;
    offs++;

    // st_other
    c = sym->st_other;
    uint8_t vis = c & 0x7;
    cout << poffs << "st_other = "
         << "vis=" << hex(2) << uint16_t(vis) << desc(vis_str(vis)) << nl;
    offs++;

    PXS(st_shndx, s)
  }
}


#undef ST
#undef P
#undef PX
#undef PXS
#undef PD
#undef PDS

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

