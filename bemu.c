#include "bemu.h"
#include "cpu.h"
#include "instruction.h"
#include <stdio.h>

typedef enum TRAP_VECTOR_E {
	HALT,
	PUTC,
	PUTS,
	GETS
} TRAP_VECTOR;

typedef enum CONDITION_CODES_E {
	N,
	Z,
	P
} CONDITION_CODES;

int sext(int16_t val, uint16_t length) {
	int n = 16 - length;
	return (int16_t)((val << n)) >> n;
}

void updateCC(int16_t val) {
	cpu.cc[N] = val < 0;
	cpu.cc[Z] = val == 0;
	cpu.cc[P] = val > 0;
}

void fetch() {
	ram.mar = cpu.pc;
	cpu.pc++;
	ram.mdr = ram.memory[ram.mar];
	cpu.ir = ram.mdr;
}

void evalAddress() {
	int opcode = (cpu.ir >> 12) & 0xF;
	if (opcode == LD || opcode == ST || opcode == LEA || opcode == BR) {
		ram.mar = cpu.pc + sext(cpu.ir & 0x1FF, 9);
	} else if (opcode == LDI || opcode == STI) {
		ram.mar = cpu.pc + sext(cpu.ir & 0x1FF, 9);
		ram.mdr = ram.memory[ram.mar];
		ram.mar = ram.mdr;
	} else if (opcode == LDR || opcode == STR) {
		ram.mar = cpu.regFile[(cpu.ir >> 6) & 0x7] + sext(cpu.ir & 0x3F, 6);
	}
}

void fetchOperands() {
	int opcode = (cpu.ir >> 12) & 0xF;
	if (opcode == LD || opcode == LDI || opcode == LDR) {
		ram.mdr = ram.memory[ram.mar];
	}
}

void execute() {
	switch ((cpu.ir >> 12) & 0xF) {
		case NOP:
			break;
		case ADD:
			switch ((cpu.ir >> 7) & 0x3) {
				case 0:
					if (cpu.ir & 0b1) {
						printf("bad instruction: %X\n at memory address %X\n", cpu.ir, ram.memory[cpu.pc - 1]);
						exit(-1);
					}

					cpu.alu.accumulator = cpu.regFile[(cpu.ir >> 4) & 0x7] + cpu.regFile[(cpu.ir >> 1) & 0x7];
					break;
				case 1:
					cpu.alu.accumulator = cpu.regFile[(cpu.ir >> 4) & 0x7] + sext(cpu.ir & 0xF, 4);
					break;
				case 2:
					if (cpu.ir & 0xF) {
						printf("bad instruction: %X\n at memory address %X\n", cpu.ir, ram.memory[cpu.pc - 1]);
						exit(-1);
					}

					cpu.alu.accumulator = cpu.regFile[(cpu.ir >> 9) & 0x7] + cpu.regFile[(cpu.ir >> 4) & 0x7];
					break;
				case 3:
					cpu.alu.accumulator = cpu.regFile[(cpu.ir >> 9) & 0x7] + sext(cpu.ir & 0x7F, 7);
					break;
			}

			break;
		case AND:
			switch ((cpu.ir >> 7) & 0x3) {
				case 0:
					if (cpu.ir & 0x1) {
						printf("bad instruction: %X\n at memory address %X\n", cpu.ir, ram.memory[cpu.pc - 1]);
						exit(-1);
					}

					cpu.alu.accumulator = cpu.regFile[(cpu.ir >> 4) & 0x7] & cpu.regFile[(cpu.ir >> 1) & 0x7];
					break;
				case 1:
					cpu.alu.accumulator = cpu.regFile[(cpu.ir >> 4) & 0x7] & sext(cpu.ir & 0x1, 4);
					break;
				case 2:
					if (cpu.ir & 0x1F) {
						printf("bad instruction: %X\n at memory address %X\n", cpu.ir, ram.memory[cpu.pc - 1]);
						exit(-1);
					}

					cpu.alu.accumulator = cpu.regFile[(cpu.ir >> 9) & 0x7] & cpu.regFile[(cpu.ir >> 4) & 0x7];
					break;
				case 3:
					cpu.alu.accumulator = cpu.regFile[(cpu.ir >> 9) & 0x7] & sext(cpu.ir & 0x7F, 7);
					break;
			}

			break;
		case NOT:
			if (cpu.ir & 0x1F) {
				printf("bad instruction: %X\n at memory address %X\n", cpu.ir, ram.memory[cpu.pc - 1]);
				exit(-1);
			}

			if (cpu.ir & 0x100) {
				cpu.alu.accumulator = ~cpu.regFile[(cpu.ir >> 9) & 0x7];
			} else {
				cpu.alu.accumulator = ~cpu.regFile[(cpu.ir >> 5) & 0x7];
			}

			break;
		case LD:
			break;
		case LDI:
			break;
		case LDR:
			break;
		case ST:
			break;
		case STI:
			break;
		case STR:
			break;
		case BR:
			if (
				(cpu.cc[N] && ((cpu.ir >> 11) & 1)) ||
				(cpu.cc[Z] && ((cpu.ir >> 10) & 1)) ||
				(cpu.cc[P] && ((cpu.ir >> 9) & 1))
			) {
				cpu.pc = cpu.pc + sext(cpu.ir & 0x1FF, 9);
			}

			break;
		case JMP:
			if (cpu.ir & 0x1FF) {
				printf("bad instruction: %X\n at memory address %X\n", cpu.ir, ram.memory[cpu.pc - 1]);
				exit(-1);
			}

			cpu.pc = cpu.regFile[(cpu.ir >> 9) & 0x7];
			break;
		case JSR:
			cpu.regFile[7] = cpu.pc;
			if (!(cpu.ir & 0x800)) {
				cpu.pc += sext(cpu.ir & 0x7FF, 11);
			} else {
				cpu.pc = cpu.regFile[(cpu.ir >> 8) & 0x7];
			}

			break;
		case LEA:
			break;
		case RET:
			cpu.pc = cpu.regFile[7];
			break;
		case TRAP:
			if (cpu.ir & 0xFF) {
				printf("bad instruction: %X\n at memory address %X\n", cpu.ir, ram.memory[cpu.pc - 1]);
				exit(-1);
			}

			switch ((cpu.ir >> 8) & 0xF) {
				case HALT:
					exit(0);
					break;
				case PUTC:
					putchar(cpu.regFile[0] & 0xFF);
					break;
				case PUTS:
					cpu.regFile[7] = cpu.pc;
					cpu.pc = cpu.regFile[0];
					while (cpu.ir != 0) {
						cpu.ir = ram.memory[cpu.pc];
						cpu.regFile[0] = cpu.ir;
						putchar(cpu.regFile[0] & 0xFF);
						cpu.pc++;					
					}

					putchar('\n');
					
					cpu.pc = cpu.regFile[7];
					break;
				case GETS:
					int n = cpu.regFile[1];
					cpu.regFile[7] = cpu.pc;
					char buf[n];
					fgets(buf, n, stdin);
					for (int i = 0; i < strlen(buf); i++) {
						ram.memory[cpu.regFile[0] + i] = buf[i] & 0xFF;
					}

					cpu.pc = cpu.regFile[7];
					break;
			}

			break;
	}
}

void storeResult() {
	int opcode = (cpu.ir >> 12) & 0xF;
	int dest = (cpu.ir >> 9) & 0x7;
	if (opcode == LD || opcode == LDI || opcode == LDR) {
		cpu.regFile[dest] = ram.mdr;
		updateCC(cpu.regFile[dest]);
	} else if (opcode == ST || opcode == STI || opcode == STR) {
		ram.memory[ram.mar] = cpu.regFile[dest];
	} else if (opcode == ADD || opcode == AND || opcode == NOT) {
		cpu.regFile[dest] = cpu.alu.accumulator;
		updateCC(cpu.regFile[dest]);
	} else if (opcode == LEA) {
		cpu.regFile[dest] = ram.mar;
		updateCC(cpu.regFile[dest]);
	}
}

void cpuCycle() {
	fetch();
	evalAddress();
	fetchOperands();
	execute();
	storeResult();
}

int main() {
	char binFileName[256];
	FILE *binFile;
	setbuf(stdout, NULL);
	printf("Enter source assembly file name: \n");
	fgets(binFileName, 256, stdin);
	binFileName[strcspn(binFileName, "\n")] = '\0';
	binFile = fopen(binFileName, "rb");
	if (!binFile) {
		printf("Source file doesn't exist!\n");
		exit(1);
	}
	size_t bytesRead = fread(&ram.memory, sizeof(int16_t), RAM_SIZE, binFile);
	if (bytesRead < RAM_SIZE) {
		fprintf(stderr, "Didn't read enough bytes\n");
		exit(1);
	}
    while (true) {
		cpuCycle();
	}
}
