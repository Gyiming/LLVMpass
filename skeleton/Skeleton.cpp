#include "Skeleton.hpp"
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
            for (Loop *loop : LI.getLoopsInPreorder()) {
                errs() << "In loop of depth " << loop->getLoopDepth() << "\n";
                // Get Loop Header - Where the loop condition is tested; where we determine upper-bounds of loop
                // Hint: This is where we get Phi nodes
                for (Instruction& instr : *loop->getHeader()) {
                    instructionDispatch(solver, instr);
                }

                // Get Loop Latchs - A set of basic blocks with a backedge to the the loop head
                // Hint: This is where we can find how much the induction variable gets incremented
                // by in each iteration of the loop.
                SmallVector<BasicBlock *, 1> latches;
                loop->getLoopLatches(latches);
                for (BasicBlock *block : latches) {
                    for (Instruction& instr : *block) {
                        instructionDispatch(solver, instr);
                    }
                }
                
                // No longer necessary to iterate over all blocks...
                /* 
                for (BasicBlock *block : loop->getBlocks()) {
                    std::vector <std::vector<llvm::StringRef>> operand_records;
                    std::vector <llvm::StringRef> instr_records;
                    for (Instruction &instr : *block) {
                        instructionDispatch(solver, instr);
                        errs() << instr.getOpcodeName() << ":";
                        std::vector <llvm::StringRef> operands;
                        llvm::StringRef a = instr.getOpcodeName();
                        instr_records.push_back(a);                                                    
                        unsigned int op_cnt = instr.getNumOperands();
                        unsigned int i;
                        llvm::StringRef temp_name;
                        
                        for (i=0; i< op_cnt; i++)
                        {
                            Value *opnd = instr.getOperand(i);
                            if (opnd->hasName())
                            {
                                errs() << opnd->getName() << ",";
                                temp_name = opnd->getName();
                                operands.push_back(temp_name);
                            }
                            else 
                            {
                                errs() << *opnd << ",";
                                        
                            }
                                
                        }
                        operand_records.push_back(operands); 
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
                    errs() << "Terminator: " << *block->getTerminator() << "\n";
                } */
            }

            errs() << solver.printILP();
            return false;
        }
        
        void instructionDispatch(ILPSolver& solver, Instruction &instr) {
            switch (instr.getOpcode()) {
                // Store(Variable | Constant, Variable) -- Source, Destination
                case Instruction::Store: {
                    ILPValue lhs(instr.getOperand(1)->getName());
                    ILPValue rhs(instr.getOperand(0)->getName());
                    ILPConstraint constraint = ILPConstraint(ILP_AS, lhs, rhs);
                    solver.add_constraint(constraint);
                    break;
                }
                // Add(Variable, Variable | Constant) -- Destination, Source
                case Instruction::Add: {
                    ILPValue lhs(instr.getOperand(0)->getName());
                    ILPValue rhs;
                    if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(1))) {
                        rhs = ILPValue(CI->getSExtValue());
                    } else {
                        rhs = ILPValue(instr.getOperand(1)->getName());
                    }
                    ILPConstraint constraint = ILPConstraint(ILP_PL, lhs, rhs);
                    solver.add_constraint(constraint);
                    break;
                }
                case Instruction::Mul: {
                    ILPValue lhs;
                    ILPValue rhs;
                    if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(0))) {
                        lhs = ILPValue(CI->getSExtValue());
                    } else {
                        lhs = ILPValue(instr.getOperand(0)->getName());
                    }
                    if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(1))) {
                        rhs = ILPValue(CI->getSExtValue());
                    } else {
                        rhs = ILPValue(instr.getOperand(1)->getName());
                    }
                    ILPConstraint constraint = ILPConstraint(ILP_MP, lhs, rhs);
                    solver.add_constraint(constraint);
                    break;
                }
                // ICmp(Variable|Constant, Variable|Constant) -- Variable < UpperBound
                case Instruction::ICmp: {
                    ILPValue cond(instr.getOperand(0)->getName());
                    ILPValue bounds;
                    if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(1))) {
                        bounds = ILPValue(CI->getSExtValue());
                    } else {
                        bounds = ILPValue(instr.getOperand(1)->getName());
                    }
                    ILPConstraint constraint = ILPConstraint(ILP_LT, cond, bounds);
                    solver.add_constraint(constraint);
                    break;
                }
                case Instruction::PHI: {
                    errs() << "PHI Node Found! (";
                    for (int i = 0; i < instr.getNumOperands(); i++) {
                         errs() << *instr.getOperand(i) << ",";  
                    }
                    errs() << ")\n";
                }
                case Instruction::GetElementPtr: {
                    errs() << "Array Indexing Found! (";
                    for (int i = 0; i < instr.getNumOperands(); i++) {
                         errs() << *instr.getOperand(i) << ",";  
                    }
                    errs() << ")\n";
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
