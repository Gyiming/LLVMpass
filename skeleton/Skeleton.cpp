#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <set>
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

/* Under Construction!!! */
#define ILP_LE "<="
#define ILP_GE ">="
#define ILP_GT ">"
#define ILP_LT "<"
#define ILP_EQ "=="
#define ILP_AS "="

struct ILPValue {
    ILPValue() : tag(UNINITIALIZED) {}
    ILPValue(int val) : tag(CONSTANT), constant_value(val) {}
    ILPValue(std::string val) : tag(VARIABLE), variable_name(val) {}
    ILPValue(const ILPValue& other) {
        this->tag = other.tag;
        if (this->tag == CONSTANT) {
            this->constant_value = other.constant_value;
        } else {
            this->variable_name = other.variable_name;
        }
    }
    ILPValue& operator=(const ILPValue& other) {
        this->tag = other.tag;
        if (this->tag == CONSTANT) {
            this->constant_value = other.constant_value;
        } else {
            this->variable_name = other.variable_name;
        }
        return *this;
    }
    ~ILPValue() {}
    enum {CONSTANT, VARIABLE, UNINITIALIZED} tag;
    union {
        // Constant
        int constant_value;
        // Variable
        std::string variable_name;
    };
};

struct ILPConstraint {
    ILPConstraint() {}
    ILPConstraint(std::string op, ILPValue& v1, ILPValue& v2) {
        this->op = op;
        this->v1 = v1;
        this->v2 = v2;
    }
    std::string op;
    ILPValue v1;
    ILPValue v2;
};

/*
 *
 * Used to pass ILP expressions.
 *
 */
class ILPSolver {
    ILPSolver() {
        
    }

    void add_constraint(ILPConstraint& constraint) {
        constraints.push_back(constraint);
    }
    
    std::string printILP() {
        // TODO: Iterate over vector of constraints, find all ILPValue with 'VARIABLE' tag, print that out
        // Variables need to be printed first as '-var1 -var2 ... -varN;'
        // Then print out all constraints recursively
        // Constraints need to be printed out as 'var1 op var2;'
        // You can then run lp_solve on this; I'll do this later!
        return nullptr;
    }
     
    std::vector<ILPConstraint> constraints;
};


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
			errs() << instr.getOpcodeName() << ":";
			unsigned int op_cnt = instr.getNumOperands();
			unsigned int i;
			for (i=0; i< op_cnt; i++)
			{
				Value *opnd = instr.getOperand(i);
				if (opnd->hasName())
				{
					errs() << opnd->getName() << ",";
				}
				else errs() << "ptr " << opnd << ",";
			}
			//if it's a store instruction
			if (instr.getOpcode() == Instruction::Store)
			{
				unsigned int i=0;
			}
			//if it's a load instruction 
			if (instr.getOpcode() == Instruction::Load)
			{
				unsigned int i=0;
			}        

			errs() << "\n";
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
