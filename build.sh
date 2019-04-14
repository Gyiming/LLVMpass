#!/bin/bash

echo "Checking hostname..."
if [ "$HOSTNAME" != "cycle1.csug.rochester.edu" ]; then
	echo "Need to be on cycle1.csug.rochester.edu to use this script! Running on $HOSTNAME"
	return -1
fi

echo "Setting \$LLVM_HOME and updating \$PATH (Note: You should be running this script via 'source $(basename $BASH_SOURCE)')"
export PATH=/localdisk/cs255/llvm-project/bin:$PATH
export LLVM_HOME=/localdisk/cs255/llvm-project/llvm/

echo "Building Skeleton..."
cd build
if [ $? -ne 0 ]; then
	echo "No 'build' directory found! Please create one and execute this script again!"
	return -1
fi

echo "Running cmake..."

cmake ..
if [ $? -ne 0 ]; then
	echo "Failed to configure with 'cmake'!"
	cd ..
	return -1
fi

echo "Running make..."
make

if [ $? -ne 0 ]; then
	echo "Failed to build with 'make'!"
	cd ..
	return -1
fi

echo "Done!"