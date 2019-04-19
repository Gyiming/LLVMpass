#!/bin/bash

echo "Checking hostname..."
if [ "$HOSTNAME" != "cycle1.csug.rochester.edu" ]; then
	tput setaf 1 ; echo "Need to be on cycle1.csug.rochester.edu to use this script! Running on $HOSTNAME" ; tput sgr0
	return -1
fi

echo "Setting \$LLVM_HOME and updating \$PATH (Note: You should be running this script via 'source $(basename $BASH_SOURCE)')"
export PATH=/localdisk/cs255/llvm-project/bin:$PATH
export LLVM_HOME=/localdisk/cs255/llvm-project/llvm/

echo "Building Skeleton..."
cd build
if [ $? -ne 0 ]; then
	tput setaf 1 ; echo "No 'build' directory found! Please create one and execute this script again!" ; tput sgr0
	return -1
fi

echo "Running cmake..."

cmake ..
if [ $? -ne 0 ]; then
	tput setaf 1 ; echo "Failed to configure with 'cmake'!" ; tput sgr0
	cd ..
	return -1
fi

echo "Running make..."
make

if [ $? -ne 0 ]; then
	tput setaf 1 ; echo "Failed to build with 'make'!" ; tput sgr0
	cd ..
	return -1
fi

cd ..
echo "Building all test files..."
cd test
if [ $? -ne 0 ]; then
	tput setaf 1 ; echo "Failed to find 'test' directory; please create it and fill with a test and makefile!" ; tput sgr0
	return -1
fi

make
if [ $? -ne 0 ]; then
	tput setaf 1 ; echo "Failed to build test files via 'make'." ; tput sgr0
	cd ..
	return -1
fi

echo "Running Tests..."
for f in *.bc; do
	fname=${f::-3}
	echo "Running $f..."
    # Readable Bitcode after mem2reg...
    opt -instnamer -mem2reg -S < "$fname.bc" > "$fname-mem2reg.ll"
	opt -load ../build/skeleton/libSkeletonPass.so -instnamer -mem2reg -analyze -induction-pass < "$fname.bc" 2> "$fname.err" 1> "$fname.out"
	if [ $? -ne 0 ]; then
		tput setaf 1 ; echo "$f failed, please see $fname.err!" ; tput sgr0
    else
        mv "output.ilp" "$fname.ilp"
    fi
done

for f in *.ilp; do
    fname=${f::-4}
    echo "Running 'glpsol --math $f'"
    if [ $? -ne 0 ]; then
        tput setaf 1 ; echo "$f failed, please see $fname.ilp!" ; tput sgr0
    fi
    if [[ $(glpsol --math $f) =~ (.*NO.*SOLUTION.*) ]]; then
        if [ -f "$fname.dep" ]; then
            tput setaf 1 ; echo "$f: Failed..." ; tput sgr0
        else
            tput setaf 2 ; echo "$f: Success" ; tput sgr0
        fi
    else
        if [ -f "$fname.dep" ]; then
            tput setaf 2 ; echo "$f: Success..." ; tput sgr0
        else
            tput setaf 1 ; echo "$f: Failed" ; tput sgr0
        fi

    fi
done
