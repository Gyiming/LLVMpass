# README

**NEW INSTRUCTIONS:** Run `source build.sh`, it will build the skeleton, build the tests and obtain their bitcode files,
and then run the LLVM pass over each test and output it to their respective *.err and *.out file. This should significantly
speed up development.

**ORIGINAL INSTRUCTIONS BELOW**

The given code is a skeleton of a llvm pass using llvm-8 on cycle machine.

## Prepration
0. Add LLVM binaries to your path (optional)
```
# Add in your .bash_profile or other shell config
export PATH=/localdisk/cs255/llvm-project/bin:$PATH
```

1. Add `LLVM_HOME` in your environment:
```
export LLVM_HOME=/localdisk/cs255/llvm-project
```

2. Use CMake and Make to compile your pass to runtime lib
```
mkdir build
cd build
cmake ..
make
```

3. Use your pass
```
clang -Xclang -load -Xclang build/skeleton/libSkeletonPass.so test.cpp
```

## Reference
https://www.cs.cornell.edu/~asampson/blog/clangpass.html
https://github.com/abenkhadra/llvm-pass-tutorial
