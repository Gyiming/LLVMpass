# README

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
This will go over the tests we have for now in the /test folder. Or you could build from scrach

3. Add LLVM binaries to your path (optional)
```
# Add in your .bash_profile or other shell config
export PATH=/localdisk/cs255/llvm-project/bin:$PATH
```

4. Add `LLVM_HOME` in your environment:
```
export LLVM_HOME=/localdisk/cs255/llvm-project
```

5. Use CMake and Make to compile your pass to runtime lib
```
mkdir build
cd build
cmake ..
make
```

6. Use your pass
```
clang -Xclang -load -Xclang build/skeleton/libSkeletonPass.so test_swap.c
```

7. Test whether the .ilp file is solvable.
```
glpsol --math test/test_swap.ilp
```

## Reference
https://www.cs.cornell.edu/~asampson/blog/clangpass.html
https://github.com/abenkhadra/llvm-pass-tutorial
