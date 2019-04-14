#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
using namespace llvm;

// Determine if instruction I holds Induction Variable for loop L
static bool isSimpleIVUser(Instruction *I, const Loop *L, ScalarEvolution *SE) {
		if (!SE->isSCEVable(I->getType()))
				return false;

		// Get the symbolic expression for this instruction.
		const SCEV *S = SE->getSCEV(I);

		// Only consider affine recurrences.
		const SCEVAddRecExpr *AR = dyn_cast<SCEVAddRecExpr>(S);
		if (AR && AR->getLoop() == L)
				return true;

		return false;
}

namespace {
		struct SkeletonPass : public FunctionPass {
				static char ID;
				SkeletonPass() : FunctionPass(ID) {}

				virtual bool runOnFunction(Function &F) {
						errs() << "Processing " << F.getName() << "\n";
						LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
						errs() << "LoopInfo" << "\n";
						ScalarEvolution &SCE = getAnalysis<ScalarEvolutionWrapperPass>().getSE();

						for (Loop *loop : LI) {
								for (BasicBlock *block : loop->getBlocks()) {
										for (Instruction &instr : *block) {
												if (isSimpleIVUser(&instr, loop, &SCE)) {
														errs() << "Found Induction Variable: " << instr << "\n";  
										
												}
										}
								}
						}
						return false;
				}

				void getAnalysisUsage(AnalysisUsage &AU) const {
						AU.setPreservesCFG();
						AU.addRequired<LoopInfoWrapperPass>();
						AU.addRequired<ScalarEvolutionWrapperPass>();
				}
		};
}

char SkeletonPass::ID = 0;

static RegisterPass<SkeletonPass> X("induction-pass", "Induction variable identification pass",
                             false /* Only looks at CFG */,
                             false /* Analysis Pass */);
