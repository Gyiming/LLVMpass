C_SRC = $(wildcard *.c)
CPP_SRC = $(wildcard *.cpp)
C_LLVM_IR = $(C_SRC:.c=.ll)
C_LLVM_BC = $(C_LLVM_IR:.ll=.bc)
CPP_LLVM_IR = $(CPP_SRC:.cpp=.ll)
CPP_LLVM_BC = $(CPP_LLVM_IR:.ll=.bc)
CC = clang

CFLAGS =-O0 -S -emit-llvm -Xclang -disable-O0-optnone 
CPPFLAGS = $-O0 -S -emit-llvm -Xclang -disable-O0-optnone
AS = llvm-as

all: $(C_LLVM_BC) $(CPP_LLVM_BC)


$(C_LLVM_IR): $(C_SRC)
		$(CC) $^ $(CFLAGS)
$(CPP_LLVM_IR): $(CPP_SRC)
		$(CC) $^ $(CPPFLAGS)
$(C_LLVM_BC): $(C_LLVM_IR)
		$(AS) $^ -o $@
$(CPP_LLVM_BC): $(CPP_LLVM_IR)
		$(AS) $^ -o $@

.PHONY: clean
clean:
		rm -f $(CPP_LLVM_IR) $(CPP_LLVM_BC) $(C_LLVM_BC) $(C_LLVM_IR)
