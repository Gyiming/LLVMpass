#include "Skeleton.hpp"
#include "llvm/Support/Path.h"
#include "llvm/Support/FileSystem.h"
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

                for (BasicBlock *block : loop->getBlocks()) {
                    if (loop->isLoopLatch(block) || loop->getHeader() == block)
                        continue;
                   for (Instruction& instr : *block) {
                       instructionDispatchBody(solver, instr);

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
            
            std::error_code ec;
            raw_fd_ostream outputFile("output.ilp", ec, sys::fs::F_None);
            assert(ec.value() == 0);
            std::string str = solver.printILP();
            outputFile << str;
            outputFile.flush();
            outputFile.close();
            return false;
        }

        ILPValue toILPValue(Value *value) {
            
            if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(value)) 
            {
                return ILPValue(CI->getSExtValue());
            }
            else
            {
                return ILPValue(value->getName());
            }
        }
        
        string getValueExpr(Value* v) 
        {
            if (isa<Instruction>(v)) 
            {
                Instruction* inst = cast<Instruction>(v);
            
                switch (inst->getOpcode()) 
                {
                    case Instruction::Add:
                        return "(" + getValueExpr(inst->getOperand(0)) + " + " + getValueExpr(inst->getOperand(1)) + ")";
                        break;
                    case Instruction::Sub:
                        return "(" + getValueExpr(inst->getOperand(0)) + " - " + getValueExpr(inst->getOperand(1)) + ")";;
                        break;
                    case Instruction::Mul:
                        return "(" + getValueExpr(inst->getOperand(0)) + " * " + getValueExpr(inst->getOperand(1)) + ")";;
                        break;
                    case Instruction::Alloca:
                        return inst->getName().str();
                    case Instruction::Load:
                        return cast<LoadInst>(inst)->getPointerOperand()->getName().str();
                        break;
                    case Instruction::SExt:
                        return getValueExpr(inst->getOperand(0));
                    default:
                        return "";
                        break;
                }
            }
            else if (isa<ConstantInt>(v)) 
            {
                return to_string(dyn_cast<ConstantInt>(v)->getValue().getSExtValue());
            }
            else if (v->hasName()) 
            {
                return v->getName().str();
            }
        }


        void instructionDispatchBody(ILPSolver& solver, Instruction &instr) {
            vector <ILPValue> oprands;
            vector <std::string> instrs;
            switch (instr.getOpcode())
            {
                case Instruction::Store: 
                    {
                        errs() << "store" << "\n";
                        instrs.push_back("Store");

                        /*
                        ILPValue lhs = toILPValue(instr.getOperand(1));
                        if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(1)))
                            errs() << CI->getSExtValue() << "****\n";
                        else
                            errs() << *instr.getOperand(1) << "****\n";
                        */
                        int i;
                        for (i=0;i<2;i++)
                        {
                            if (instr.getOperand(i)->hasName())
                                errs() << instr.getOperand(i)->getName() << "****\n";
                            else if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(i)))
                                errs() << CI->getSExtValue() << "****\n";
                            else 
                                {
                                    Value* temp = instr.getOperand(i);
                                    std::string exp = getValueExpr(temp);
                                    errs() << exp << "****\n";
                                }

                        }
                        break;    
                    }
                case Instruction::Load:
                    {
                        instrs.push_back("Load");
                        errs() << "Load " << "\n";
                        int i;
                        for (i=0;i<instr.getNumOperands();i++)
                        {
                            if (instr.getOperand(i)->hasName())
                                errs() << instr.getOperand(i)->getName() << "****\n";
                            else if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(i)))
                                errs() << CI->getSExtValue() << "****\n";
                            else 
                                {
                                    Value* temp = instr.getOperand(i);
                                    std::string exp = getValueExpr(temp);
                                    errs() << exp << "****\n";
                                }
                        }
                        
                        break;
                    }
                case Instruction::Add:
                    {
                        instrs.push_back("Add");
                        int num_opt  = instr.getNumOperands();
                        int i;
                        
                        errs() << "Add " << "\n";
                        for (i=0;i<instr.getNumOperands();i++)
                        {
                            if (instr.getOperand(i)->hasName())
                                errs() << instr.getOperand(i)->getName() << "****\n";
                            else if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(i)))
                                errs() << CI->getSExtValue() << "****\n";
                            else 
                                {
                                    Value* temp = instr.getOperand(i);
                                    std::string exp = getValueExpr(temp);
                                    errs() << exp << "****\n";
                                }
                        }
                        break;
                    }
                case Instruction::Sub:
                    {
                        instrs.push_back("Sub");
                        int num_opt  = instr.getNumOperands();
                        int i;
                        
                        errs() << "Sub " << "\n";
                        for (i=0;i<instr.getNumOperands();i++)
                        {
                            if (instr.getOperand(i)->hasName())
                                errs() << instr.getOperand(i)->getName() << "****\n";
                            else if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(i)))
                                errs() << CI->getSExtValue() << "****\n";
                            else 
                                {
                                    Value* temp = instr.getOperand(i);
                                    std::string exp = getValueExpr(temp);
                                    errs() << exp << "****\n";
                                }
                        }
                        break;
                    }

            
            }
        }
        void instructionDispatch(ILPSolver& solver, Instruction &instr) {
            switch (instr.getOpcode()) {
                // Store(Variable | Constant, Variable) -- Source, Destination
                case Instruction::Store: {
                    ILPValue lhs = toILPValue(instr.getOperand(1));
                    ILPValue rhs = toILPValue(instr.getOperand(0));
                    ILPConstraint constraint = ILPConstraint(ILP_AS, lhs, rhs);
                    solver.add_constraint(constraint);
                    break;
                }
                case Instruction::Load:{
                    ILPValue lhs = toILPValue(instr.getOperand(1));
                    ILPValue rhs = toILPValue(instr.getOperand(0));
                    ILPConstraint constraint = ILPConstraint(ILP_AS,lhs,rhs);
                    solver.add_constraint(constraint);
                    break;
                }
                case Instruction::Sub:{
                    ILPValue lhs = toILPValue(instr.getOperand(0));
                    ILPValue rhs = toILPValue(instr.getOperand(1));
                    ILPConstraint constraint = ILPConstraint(ILP_SB, lhs, rhs, instr.getName());
                    //solver.add_constraint(constraint);
                    break;  
                }
                
                // Add(Variable, Variable | Constant) -- Destination, Source
                case Instruction::Add: {
                    ILPValue lhs = toILPValue(instr.getOperand(0));
                    ILPValue rhs = toILPValue(instr.getOperand(1));
                    ILPConstraint constraint = ILPConstraint(ILP_PL, lhs, rhs, instr.getName());
                    //solver.add_constraint(constraint);
                    break;
                }
                case Instruction::Mul: {
                    ILPValue lhs = toILPValue(instr.getOperand(0));
                    ILPValue rhs = toILPValue(instr.getOperand(1));
                    ILPConstraint constraint = ILPConstraint(ILP_MP, lhs, rhs, instr.getName());
                    solver.add_constraint(constraint);
                    break;
                }
                // ICmp(Variable|Constant, Variable|Constant) -- Variable < UpperBound
                case Instruction::ICmp: {
                    ILPValue cond = toILPValue(instr.getOperand(0));
                    ILPValue bounds = toILPValue(instr.getOperand(1));
                    ILPConstraint constraint = ILPConstraint(ILP_LE, cond, bounds);
                    solver.add_constraint(constraint);
                    break;
                }
                // Assumption: The first PHI operand is lower-bound
                case Instruction::PHI: {
                    ILPValue lhs(instr.getName());
                    ILPValue rhs = toILPValue(instr.getOperand(0));
                    ILPConstraint constraint = ILPConstraint(ILP_GE, lhs, rhs);
                    solver.add_constraint(constraint);
                    errs() << "PHI Node Found! (" << instr.getName() << ",";
                    for (int i = 0; i < instr.getNumOperands(); i++) {
                         errs() << *instr.getOperand(i) << ",";  
                    }
                    errs() << ")\n";
                }
                case Instruction::GetElementPtr: {
                    errs() << "Array Indexing Found! (" << instr.getName() << ",";
                    for (int i = 0; i < instr.getNumOperands(); i++) {
                         errs() << instr.getOperand(i)->getName() << ",";  
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
