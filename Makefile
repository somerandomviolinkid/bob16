all: bemu.exe basm.exe

bemu.exe: bemu.c
	gcc bemu.c -o bemu.exe

basm.exe: basm.c
	gcc basm.c -o basm.exe
