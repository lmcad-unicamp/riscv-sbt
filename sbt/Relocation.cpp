  using ConstRelocIter = ConstRelocationPtrVec::const_iterator;


  llvm::Expected<llvm::Value*> handleRelocation(llvm::raw_ostream &SS);

llvm::Expected<llvm::Value *>
Translator::handleRelocation(llvm::raw_ostream &SS)
{
  LastImm.IsSym = false;

  // No more relocations exist
  if (RI == RE)
    return nullptr;

  // Check if there is a relocation for current address
  auto CurReloc = *RI;
  if (CurReloc->offset() != CurAddr)
    return nullptr;

  ConstSymbolPtr Sym = CurReloc->symbol();
  uint64_t Type = CurReloc->type();
  bool IsLO = false;
  uint64_t Mask;
  ConstSymbolPtr RealSym = Sym;

  switch (Type) {
    case llvm::ELF::R_RISCV_PCREL_HI20:
    case llvm::ELF::R_RISCV_HI20:
      break;

    // This rellocation has PC info only
    // Symbol info is present on the PCREL_HI20 Reloc
    case llvm::ELF::R_RISCV_PCREL_LO12_I:
      IsLO = true;
      RealSym = (**RLast).symbol();
      break;

    case llvm::ELF::R_RISCV_LO12_I:
      IsLO = true;
      break;

    default:
      DBGS << "Relocation Type: " << Type << '\n';
      llvm_unreachable("unknown relocation");
  }

  // Set Symbol Relocation info
  SymbolReloc SR;
  SR.IsValid = true;
  SR.Name = RealSym->name();
  SR.Addr = RealSym->address();
  SR.Val = SR.Addr;
  SR.Sec = RealSym->section();

  // Note: !SR.Sec && SR.Addr == External Symbol
  assert((SR.Sec || !SR.Addr) && "No section found for relocation");
  if (SR.Sec) {
    assert(SR.Addr < SR.Sec->size() && "Out of bounds relocation");
    SR.Val += SR.Sec->shadowOffs();
  }

  // Increment relocation iterator
  RLast = RI;
  do {
    ++RI;
  } while (RI != RE && (**RI).offset() == CurAddr);

  // Write relocation string
  if (IsLO) {
    Mask = 0xFFF;
    SS << "%lo(";
  } else {
    Mask = 0xFFFFF000;
    SS << "%hi(";
  }
  SS << SR.Name << ") = " << SR.Addr;

  Value *V = nullptr;
  bool IsFunction =
    SR.Sec && SR.Sec->isText() &&
    RealSym->type() == llvm::object::SymbolRef::ST_Function;

  // special case: external functions: return our handler instead
  if (SR.isExternal()) {
    auto ExpF = import(SR.Name);
    if (!ExpF)
      return ExpF.takeError();
    uint64_t Addr = ExpF.get();
    Addr &= Mask;
    V = ConstantInt::get(I32, Addr);

  // special case: internal function
  } else if (IsFunction) {
    uint64_t Addr = SR.Addr;
    Addr &= Mask;
    V = ConstantInt::get(I32, Addr);

  // add the relocation offset to ShadowImage to get the final address
  } else if (SR.Sec) {
    // get char * to memory
    std::vector<llvm::Value *> Idx = { ZERO,
      llvm::ConstantInt::get(I32, SR.Val) };
    V = Builder->CreateGEP(ShadowImage, Idx);
    updateFirst(V);

    V = i8PtrToI32(V);

    // Finally, get only the upper or lower part of the result
    V = Builder->CreateAnd(V, llvm::ConstantInt::get(I32, Mask));
    updateFirst(V);

  } else
    llvm_unreachable("Failed to resolve relocation");

  LastImm.IsSym = true;
  LastImm.SymRel = std::move(SR);
  return V;
}

