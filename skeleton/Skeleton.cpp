#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include <set>
#include <sstream>
#include <string>
using namespace std;
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
    ILPValue(StringRef val) : tag(VARIABLE), variable_name(val) {}

    enum {CONSTANT, VARIABLE, UNINITIALIZED} tag;
    // Constant
    int constant_value;
    // Variable
    StringRef variable_name;

    friend std::ostream& operator<<(std::ostream& os, const ILPValue val);
};

std::ostream& operator<<(std::ostream& os, const ILPValue val) {
    if (val.tag == ILPValue::CONSTANT) os << val.constant_value;
    else if (val.tag == ILPValue::VARIABLE) os << val.variable_name;
    return os;
}

struct ILPConstraint {
    ILPConstraint() {}
    ILPConstraint(std::string op, ILPValue v1, ILPValue v2) {
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
struct ILPSolver {
    ILPSolver() {
        
    }
    
    void add_constraint(ILPConstraint constraint) {
        constraints.push_back(constraint);
    }
    
    std::string printILP() {
        // TODO: Iterate over vector of constraints, find all ILPValue with 'VARIABLE' tag, print that out
        // Variables need to be printed first as '-var1 -var2 ... -varN;'
        // Then print out all constraints recursively
        // Constraints need to be printed out as 'var1 op var2;'
        // You can then run lp_solve on this; I'll do this later!
        std::set<StringRef> variables;
        std::stringstream str;
        for (ILPConstraint& constraint : constraints) {
            if (constraint.v1.tag == ILPValue::VARIABLE) {
                variables.insert(constraint.v1.variable_name);
            }
            if (constraint.v2.tag == ILPValue::VARIABLE) {
                variables.insert(constraint.v2.variable_name);
            }
        }
        for (auto variable : variables) {
            str << "-" << variable.str() << " ";
        }
        str << ";\n";
        
        // TODO: Need to print out constraints...

        return str.str();
    }
     
    std::vector<ILPConstraint> constraints;
};


struct Instrecord{
	std::string inst_type;
    vector <string> inst_oper;
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
            ILPSolver solver;
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
                            else errs() << *opnd << ",";
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

            errs() << solver.printILP();
            return false;
        }
        
        void instructionDispatch(ILPSolver& solver, Instruction &instr) {
            switch (instr.getOpcode()) {
                case Instruction::Store: {
                    ILPValue lhs(instr.getOperand(1)->getName());
                    ILPValue rhs(instr.getOperand(0)->getName());
                    ILPConstraint constraint = ILPConstraint(ILP_AS, lhs, rhs);
                    solver.add_constraint(constraint);
                }
            }
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
