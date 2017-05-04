llvm::Error Translator::translateSection(ConstSectionPtr Sec)
{
  assert(!inFunc());

  uint64_t SectionAddr = Sec->address();
  uint64_t SectionSize = Sec->size();

  // relocatable object?
  uint64_t ELFOffset = SectionAddr;
  if (SectionAddr == 0)
    ELFOffset = Sec->getELFOffset();

  // skip non code sections
  if (!Sec->isText())
    return Error::success();

  // print section info
  DBGS << formatv("Section {0}: Addr={1:X+4}, ELFOffs={2:X+4}, Size={3:X+4}\n",
      Sec->name(), SectionAddr, ELFOffset, SectionSize);

  State = ST_DFL;
  setCurSection(Sec);

  // get relocations
  const ConstRelocationPtrVec Relocs = CurObj->relocs();
  auto RI = Relocs.cbegin();
  auto RE = Relocs.cend();
  setRelocIters(RI, RE);

  // get section bytes
  StringRef BytesStr;
  if (Sec->contents(BytesStr)) {
    SBTError SE;
    SE << "Failed to get Section Contents";
    return error(SE);
  }
  CurSectionBytes = llvm::ArrayRef<uint8_t>(
    reinterpret_cast<const uint8_t *>(BytesStr.data()), BytesStr.size());

  // for each symbol
  const ConstSymbolPtrVec &Symbols = Sec->symbols();
  size_t SN = Symbols.size();
  for (size_t SI = 0; SI != SN; ++SI)
    if (auto E = translateSymbol(SI))
      return E;

  return Error::success();
}

