#include "MTS.h"
#include "Detector.h"

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/LoopPass.h"

using namespace llvm;

namespace
{
struct SkeletonPass : public ModulePass
{
  static char ID;
  SkeletonPass() : ModulePass(ID) {}

  virtual void getAnalysisUsage(AnalysisUsage &AU) const
  {
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
  }

  bool runOnModule(Module &M) override;
};
} // namespace

std::string bb_getname(BasicBlock *bb)
{
  std::string bb_name;
  bb_name = bb->getName();

  if (bb_name.empty())
  {
    std::string Str;
    raw_string_ostream OS(Str);
    bb->printAsOperand(OS, false);
    std::string func_name = bb->getParent()->getName();
    bb_name = func_name + OS.str();
    bb->setName(bb_name);
    //errs() << OS.str() << "\n";
  }

  return bb_name;
}

bool SkeletonPass::runOnModule(Module &M)
{
  //output module information
  errs() << "Begin: " << M.getModuleIdentifier() << "\n";
  /*
  std::string module_name = M.getName();
  char *oldname, *p, *tempp;
  oldname = new char[module_name.size() + 1];
  strcpy(oldname, module_name.c_str());
  p = strtok(oldname, "/");
  while (p != NULL)
  {
    tempp = p;
    p = strtok(NULL, "/");
  }
  module_name = tempp;
  delete[] oldname;
  errs() << "Module name: " << module_name << "\n";
  */
  auto MMC = MyModuleContext(M);
  auto i = 0;
  for (auto &F : M)
  {
    if (F.size() == 0)
      continue;
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
    vector<Loop *> workspace;
    for (auto it = LI.begin(); it != LI.end(); it++)
    {
      Loop *ll = *it;
      ll->print(dbgs());
      errs() << "Loop: "
             << "\n";
      workspace.push_back(*it);
      for (BasicBlock *BB : (*it)->getBlocks())
      {
        std::string bb_name = bb_getname(BB);
        errs() << "BB name: " << bb_name << "\n";
      }
    }
  }

  return false;
}

//register llvm pass
char SkeletonPass::ID = 0;

static RegisterPass<SkeletonPass> X("test", "Test Pass");
static void registerPass(const PassManagerBuilder &,
                         legacy::PassManagerBase &PM)
{
  PM.add(new SkeletonPass());
}

static RegisterStandardPasses RegisterAFLPass(
    PassManagerBuilder::EP_OptimizerLast, registerPass);

static RegisterStandardPasses RegisterAFLPass0(
    PassManagerBuilder::EP_EnabledOnOptLevel0, registerPass);

/*
// Automatically enable the pass.
// http://adriansampson.net/blog/clangpass.html
static void registerSkeletonPass(const PassManagerBuilder &,
                                 legacy::PassManagerBase &PM)
{
  PM.add(new SkeletonPass());
}
static RegisterStandardPasses
    RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible,
                   registerSkeletonPass);
*/
