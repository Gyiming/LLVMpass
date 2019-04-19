# README

Authors: Louis Jenkins & Yiming Gan

We describe our idea of how to build a llvm pass that could effectively transfer the loop dependence problem into a integer linear program problem and solve it using ilp solvers such as glpsol.

## Principles
We have several principles and we listed here:

1. Keep two stacks for both load and store instructions. When ever push something new into the stack, check the stack status to decide whether or not we need to add a constraint.

2. Add a constraint for other instructions such as add, sub, etc. The reason to do so is that we don't need to trace the id of the load and store instructions.

## Logistics
We decribe our code logistics here.

1. All the header file is in Skeleton.hpp. We mainly define three structs ILPValue, ILPConstraint and ILPSolver to connect the llvm ir to ilp solver. We define multiple operators such as add, substraction, equal to, assign, etc.

2. The pass is finished in Skeleton.cpp. For each loop, we handle the loop header and latch using function `instructionDispatch()` and handle the loop body using function `instructionDispatchBody()`.

3. For both header/latch and body, we handle different instructions seperately using `llvm::Instruction::getOpcode`. 


## Reference
https://www.cs.cornell.edu/~asampson/blog/clangpass.html
https://github.com/abenkhadra/llvm-pass-tutorial
http://llvm.org/docs/WritingAnLLVMPass.html

