C_SRC = $(wildcard *.c)
CPP_SRC = $(wildcard *.cpp)
C_LLVM_IR = $(C_SRC:.c=.ll)
C_LLVM_BC = $(C_LLVM_IR:.ll=.bc)
CPP_LLVM_IR = $(CPP_SRC:.cpp=.ll)
CPP_LLVM_BC = $(CPP_LLVM_IR:.ll=.bc)
CC = clang

CFLAGS =-O0 -Wall -Wextra -S -emit-llvm -Xclang -disable-O0-optnone -fno-discard-value-names
CPPFLAGS = -O0 -Wall -Wextra -S -emit-llvm -Xclang -disable-O0-optnone -fno-discard-value-names
AS = ./llvm-as-wrapper.py

all: $(C_LLVM_BC) $(CPP_LLVM_BC)


$(C_LLVM_IR): $(C_SRC)
		echo $(C_SRC)
		$(CC) $^ $(CFLAGS)
$(CPP_LLVM_IR): $(CPP_SRC)
		$(CC) $^ $(CPPFLAGS)
$(C_LLVM_BC): $(C_LLVM_IR)
		echo $(C_LLVM_BC)
		$(AS) $^
$(CPP_LLVM_BC): $(CPP_LLVM_IR)
		$(AS) $^

.PHONY: clean
clean:
		rm -f $(wildcard *.ll) $(wildcard *.bc) $(wildcard *.out) $(wildcard *.err) $(wildcard *.ilp)
