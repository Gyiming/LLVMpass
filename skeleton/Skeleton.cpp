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
            // Note, we have one vector for this entire function (I.E this will _only_ work if we have
            // only one loop with up to 1 loop nest!); this is because loop nests are treated as separate
            // loops, and so we need to keep this at the top-level. If time permits, we may clear them on-demand.
            // Vectors of previous loads to create constraints for...
            auto loads = SmallVector<SmallVectorImpl<Value*>*,2>();
            auto stores = SmallVector<SmallVectorImpl<Value*>*,2>();
            
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
                        instructionDispatchBody(solver, instr, loads, stores);

                   } 
                }
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
            else if (value->hasName())
            {
                return ILPValue(value->getName());
            }
            else {
                value->dump();
                errs() << value->getName() << "\n";
                return ILPValue(value->getName());
            }
        }
    
        SmallVectorImpl<Value*> *debugLoadInstr(Value *v) {
            return debugArrayAccess(cast<LoadInst>(v)->getPointerOperand());
        }

        SmallVectorImpl<Value*> *debugStoreInstr(Value *v) {
            return debugArrayAccess(cast<StoreInst>(v)->getPointerOperand());
        }
        // Returns a vector of Value* where the first index is the Array declaration itself,
        // and the others are indices into said index.
        SmallVectorImpl<Value*> *debugArrayAccess(Value *ptrOp) {
            Value *array = nullptr;
            Value *idx1 = nullptr;
            Value *idx2 = nullptr;
            if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(ptrOp)) {
                auto ptrOp2 = GEP->getPointerOperand();
                errs() << "GEP indexing into " << *ptrOp2 << "\n";
                errs() << "GEP Index is " << GEP->getOperand(GEP->getNumIndices())->getName() << "\n";
                idx1 = GEP->getOperand(GEP->getNumIndices());
                if (GetElementPtrInst *GEP2 = dyn_cast<GetElementPtrInst>(ptrOp2)) {
                    errs() << "GEP indexing into " << *GEP2->getPointerOperand() << "\n";
                    errs() << "GEP Index is " << GEP2->getOperand(GEP->getNumIndices())->getName() << "\n";
                    idx2 = GEP2->getOperand(GEP->getNumIndices());
                    array = GEP2->getPointerOperand();
                } else {
                    array = GEP->getPointerOperand(); 
                }
            }

            auto ret = new SmallVector<Value *, 3>();
            ret->push_back(array);
            ret->push_back(idx1);
            if (idx2 != nullptr) {
                ret->push_back(idx2);
            }
            return ret;
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
            else return "";
        }


        void instructionDispatchBody(ILPSolver& solver, Instruction &instr, 
                SmallVectorImpl<SmallVectorImpl<Value *>*>& loads, 
                SmallVectorImpl<SmallVectorImpl<Value *>*>& stores) 
        {
            vector <ILPValue> oprands;
            vector <std::string> instrs;
            switch (instr.getOpcode())
            {
                case Instruction::Store: 
                    {
                        stores.push_back(debugStoreInstr(&instr));
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
                        loads.push_back(debugLoadInstr(&instr));
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
                        ILPValue lhs = toILPValue(instr.getOperand(0));
                        ILPValue rhs = toILPValue(instr.getOperand(1));
                        ILPConstraint constraint = ILPConstraint(ILP_PL, lhs, rhs, instr.getName());
                    /*    //int num_opt  = instr.getNumOperands();
                        int i;
                        
                        errs() << "Add " << "\n";
                        std::vector <StringRef> addexpr;
                        for (i=0;i<2;i++)
                        {
                            if (instr.getOperand(i)->hasName())
                            {
                                errs() << instr.getOperand(i)->getName() << "****\n";
                                addexpr.push_back(instr.getOperand(i)->getName());
                            }
                            else if (llvm::ConstantInt* CI = dyn_cast<llvm::ConstantInt>(instr.getOperand(i)))
                            {
                                errs() << CI->getSExtValue() << "****\n";
                                addexpr.push_back(to_string(CI->getSExtValue()));
                            }
                            else 
                                {
                                    Value* temp = instr.getOperand(i);
                                    std::string exp = getValueExpr(temp);
                                    errs() << exp << "****\n";
                                    addexpr.push_back(exp);
                                }
                        }
                        
                        //ILPValue lhs = toILPValue(final_exp);
                        //ILPValue rhs = toILPValue(final_dest);
                        ILPConstraint constraint = ILPConstraint(ILP_PL,ILPValue(addexpr[0]), ILPValue(addexpr[1]), instr.getName());
                       */ solver.add_constraint(constraint);
                        break;
                    }
                case Instruction::Sub:
                    {
                        instrs.push_back("Sub");
                        //int num_opt  = instr.getNumOperands();
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
                    ILPValue lhs = ILPValue(instr.getName());
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
