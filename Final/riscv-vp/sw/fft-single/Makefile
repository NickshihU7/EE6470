all : main.cpp
	riscv32-unknown-elf-g++ -std=c++14 main.cpp -o main -march=rv32ima -mabi=ilp32
	
sim: all
	../../vp/build/bin/riscv-vp --intercept-syscalls main
	
clean:
	rm -f main
