#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Module.h"
using namespace llvm;

namespace
{
struct SkeletonPass : public ModulePass
{
  static char ID;
  SkeletonPass() : ModulePass(ID) {}

  bool runOnModule(Module &M) override;
};
} // namespace

bool SkeletonPass::runOnModule(Module &M)
{
  LLVMContext &Ctx = M.getContext();
  std::vector<Type *> paramTypes = {Type::getInt32Ty(Ctx)};
  Type *retType = Type::getVoidTy(Ctx);
  FunctionType *logFuncType = FunctionType::get(retType, paramTypes, false);

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

  Constant *logFunc = M.getOrInsertFunction("logop", logFuncType);

  for (auto &F : M)
  {
    std::string func_name = F.getName();
    errs() << "Func name: " << func_name << "\n";
    for (auto &BB : F)
    {
      for (auto &I : BB)
      {
        if (auto *op = dyn_cast<BinaryOperator>(&I))
        {
          // Insert *after* `op`.
          IRBuilder<> builder(op);
          builder.SetInsertPoint(&BB, ++builder.GetInsertPoint());

          // Insert a call to our function.
          Value *args[] = {op};
          builder.CreateCall(logFunc, args);
        }
      }
    }
  }

  return true;
}

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
