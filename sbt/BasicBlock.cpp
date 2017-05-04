llvm::BasicBlock *Translator::splitBB(
  llvm::BasicBlock *BB,
  uint64_t Addr)
{
  auto Res = InstrMap[Addr];
  assert(Res && "Instruction not found!");

  Instruction *TgtInstr = *Res;

  BasicBlock::iterator I, E;
  for (I = BB->begin(), E = BB->end(); I != E; ++I) {
    if (&*I == TgtInstr)
      break;
  }
  assert(I != E);

  BasicBlock *BB2;
  if (BB->getTerminator()) {
    BB2 = BB->splitBasicBlock(I, SBTBasicBlock::getBBName(Addr));
    BBMap(Addr, std::move(BB2));
    return BB2;
  }

  // Insert dummy terminator
  assert(Builder->GetInsertBlock() == BB);
  Instruction *Instr = Builder->CreateRetVoid();
  BB2 = BB->splitBasicBlock(I, SBTBasicBlock::getBBName(Addr));
  BBMap(Addr, std::move(BB2));
  Instr->eraseFromParent();
  Builder->SetInsertPoint(BB2, BB2->end());

  return BB2;
}

