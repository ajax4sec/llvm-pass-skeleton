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

#include <fstream>

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

  void analysisFunction(Function &F, MyModuleContext &MMC);
  void instruMain(Module &M);
  void writeStaticTrace(MyModuleContext &MMC);
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

void SkeletonPass::analysisFunction(Function &F, MyModuleContext &MMC)
{
  auto MF = new MyFunction(F, MMC);
  // for (auto &MBB : MF.getBBs()) {
  //     MBB.getBB().print(errs());
  // }
  // for (auto &BB : F) {
  // 	BB.print(errs());
  // }

  // DFS to tranverse all the loops in a specific function.
  // TODO: Add this part to MyFunction.
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(F).getLoopInfo();
#ifdef MYDBG
  for (auto it = LI.begin(); it != LI.end(); it++)
  {
    Loop *ll = *it;
    errs() << "    ";
    ll->print(dbgs());
  }
#endif
  vector<Loop *> workspace;
  for (auto it = LI.begin(); it != LI.end(); it++)
  {
    workspace.push_back(*it);
  }
  while (!workspace.empty())
  {
    auto L = workspace.back();
    workspace.pop_back();
    MF->addLoop(*L);
    for (auto sl : *L)
    {
      workspace.push_back(sl);
    }
  }

  // Update LoopID for MyBasicBlocks.
  // TODO: Add this part to MyFunction.
  auto &MLs = MF->getLoops();
  for (auto MBB : *MF)
  {
    auto L = LI.getLoopFor(&MBB->getBB());
    if (!L)
      continue;
    for (auto ML : MLs)
    {
      if (L == &ML->getLoop())
      {
        MBB->setLoopID(ML->getID());
        break;
      }
    }
  }

  MF->loopAnalysis();
}

void SkeletonPass::instruMain(Module &M)
{
  auto main = M.getFunction("main");
  if (main)
  {
    {
      IRBuilder<> Builder(&(*main->begin()->begin()));
      vector<Type *> argsType;
      ArrayRef<Type *> argsRef(argsType);
      auto funcType = FunctionType::get(Builder.getVoidTy(), argsRef, false);
      auto initFunc = M.getOrInsertFunction("__init_main", funcType);
      Builder.CreateCall(initFunc);
    }
    for (auto bb = main->begin(), bend = main->end(); bb != bend; ++bb)
    {
      for (auto inst = bb->begin(), iend = bb->end(); inst != iend; ++inst)
      {
        if (auto ri = dyn_cast<ReturnInst>(inst))
        {
          IRBuilder<> Builder(&(*inst));
          vector<Type *> argsType;
          ArrayRef<Type *> argsRef(argsType);
          auto funcType =
              FunctionType::get(Builder.getVoidTy(), argsRef, false);
          auto finiFunc = M.getOrInsertFunction("__fini_main", funcType);
          Builder.CreateCall(finiFunc);
        }
        else if (auto ci = dyn_cast<CallInst>(inst))
        {
          // auto fn = ci->getCalledFunction();
          // if (!fn)
          //   continue;
          // if (fn->getName().equals("exit")) {
          //   IRBuilder<> Builder(&(*inst));
          //   vector<Type *> argsType;
          //   ArrayRef<Type *> argsRef(argsType);
          //   auto funcType =
          //       FunctionType::get(Builder.getVoidTy(), argsRef, false);
          //   auto finiFunc = M.getOrInsertFunction("__fini_main", funcType);
          //   Builder.CreateCall(finiFunc);
          // }
        }
      }
    }
  }

  for (auto &F : M)
  {
    for (auto &BB : F)
    {
      for (auto &inst : BB)
      {
        if (auto ci = dyn_cast<CallInst>(&inst))
        {
          auto fn = ci->getCalledFunction();
          if (!fn)
            continue;
          if (fn->getName().equals("exit"))
          {
            IRBuilder<> Builder(&inst);
            vector<Type *> argsType;
            ArrayRef<Type *> argsRef(argsType);
            auto funcType =
                FunctionType::get(Builder.getVoidTy(), argsRef, false);
            auto finiFunc = M.getOrInsertFunction("__fini_main", funcType);
            Builder.CreateCall(finiFunc);
          }
        }
      }
    }
  }
}

void SkeletonPass::writeStaticTrace(MyModuleContext &MMC)
{
  
  string file = MMC.getModuleName() + ".txt";
  //errs() << file << "\n";
  ofstream out(file);
  if (out.is_open())
  {
    auto &FL = MMC.getFunctionList();
    for (auto MF : FL)
    {
      auto fid = MF->getID();
      out << "Function_"<< fid << " :" << MF->getNumArgs()<<"\n";
    }
    out.close();
  }
  else
    errs() << "open file error\n";
}

//-------------------- MAIN FUNCTION ----------------------
bool SkeletonPass::runOnModule(Module &M)
{
  auto MMC = MyModuleContext(M);
  errs() << "Target: " << M.getModuleIdentifier() << "\n";
  errs() << "--->Function Analysis"
         << "\n";
  auto i = 0;
  for (auto &F : M)
  {
    if (F.size() == 0)
      continue;

    errs() << "Function " << i++ << ": " << F.getName() << "\n";
    // TODO: Add lib function.
    if (F.isDeclaration())
      continue;
    analysisFunction(F, MMC);
  }

  errs() << "--->Detector Instrument"
         << "\n";
  auto SD = SDetector(MMC);

  for (auto MF : MMC)
  {
    MF->applyInstrumentations();
  }

  // Instrument for main function.
  errs() << "--->Instrument Runtime Detector\n";
  instruMain(M);

  // Write trace
  errs() << "--->Write Static Trace\n";
  writeStaticTrace(MMC);

  // TODO: Delete all the new objects.
  errs() << "End\n";
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
