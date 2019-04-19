# README

Authors: Louis Jenkins & Yiming Gan

The given code is a skeleton of a llvm pass using llvm-8 on cycle machine building integer linear program to test the dependence inside loops.


## Prerequisites

0. llvm installed.

1. GLPK solver installed.

## Prepration
0. Clone the github repo
```
# clone the repo into your local directory
git clone https://github.com/Gyiming/LLVMpass
cd LLVMpass
```
1. Create Build folder
```
mkdir build
```

2. Run the script for auto-testing
```
./build.sh
```
This will go over the tests we have for now in the /test folder. 

3. Verify with new test file
We support using your own test code to verify our tools. If you have a code called test_mytest.c, add it into /test folder. If it has dependence, you also need to add a blank file named test_mytest.dep inside /test folder. And run the script with 
```
./build.sh
```

4. Check dependence in your code
If you want to check the dependence of your test file, assuming your code is called test_mycheck.c. Add it into /test folder. Run
```
./build.sh
glpsol --math test/test_mycheck.ilp
```
If the results shows "LP HAS NO PRIMAL FEASIBLE SOLUTION", it means there is no dependence inside the code.
If the results shows "OPTIMAL LP SOLUTION FOUND", it means there is a dependence inside the loop. 
We only support 1D array for now. Using 2D array may result in an wrong answer.

Or you could build from scrach

5. Add LLVM binaries to your path (optional)
```
# Add in your .bash_profile or other shell config
export PATH=/localdisk/cs255/llvm-project/bin:$PATH
```

6. Add `LLVM_HOME` in your environment:
```
export LLVM_HOME=/localdisk/cs255/llvm-project
```

7. Use CMake and Make to compile your pass to runtime lib
```
mkdir build
cd build
cmake ..
make
```

8. Use your pass
```
clang -Xclang -load -Xclang build/skeleton/libSkeletonPass.so test_swap.c
```

9. Test whether the .ilp file is solvable.
```
glpsol --math test/test_swap.ilp
```

## Reference
https://www.cs.cornell.edu/~asampson/blog/clangpass.html
https://github.com/abenkhadra/llvm-pass-tutorial
http://llvm.org/docs/WritingAnLLVMPass.html

