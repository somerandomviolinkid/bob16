/*
	BOB-16
	written by misterbob
	you are free to use and modify this but PLEASE credit me (at least retain the name in some way)

	** read all of the docs before using as they contain some important tidbits **

	**** INSTRUCTION SEMANTICS ****
	(instructions are in order of opcode, nop is 0, add is 1, etc.)
	(instructions with a * change condition codes)
		
		NOP: does nothing other than increment pc

		*ADD (normal normal):		r[dst]  = r[src1] + r[src2]
		ex. add r0 r1 r2

		*ADD (normal immediate): 	r[dst]  = r[src1] + imm[4]
		ex. add r0 r1 4

		*ADD (in place normal): 	r[dst] += r[src1]
		ex. add r0 r1

		*ADD (in place immediate): 	r[dst] += imm[7]
		ex. add r0 4

		*AND (normal normal): 		r[dst]  = r[src1] & r[src2]
		ex. and r0 r1 r2

		*AND (normal immediate): 	r[dst]  = r[src1] & imm[4]
		ex. and r0 r1 4

		*AND (in place normal): 	r[dst] &= r[src1]
		ex. and r0 r1

		*AND (in place immediate): 	r[dst] &= imm[7]
		ex. and r0 4

		*NOT (normal): 		 	r[dst] = !r[src]
		ex. not r0 r1

		*NOT (immediate): 	!r[dst]
		ex. not r1

		*LD:  r[dst] <= memory[pc + imm[9]]
		ex. ld r0 15

		*LDI: r[dst] <= memory[memory[pc + imm[9]]
		ex. ldi r0 15

		*LDR: r[dst] <= memory[r[src] + imm[6]]
		ex. ldr r0 r1 15

		ST:  r[src] => memory[pc + imm[9]]
		ex. st r0 15

		STI: r[src] => memory[memory[pc + imm[9]]
		ex. sti r0 15

		STR: r[src] => memory[r[dst] + imm[6]]
		ex. str r0 r1 15

		BR: if condition codes match any of the nzp flags, pc += imm[9]
		ex. br np 50
		(jumps to incremented pc + 50 if condition codes n or p are 1)

		JMP: pc = r[src]
		ex. jmp r0

		JSR:  r[7] = pc, pc += imm[11]
		ex. jsr 50

		JSRR: r[7] = pc, pc  = r[src]
		ex. jsrr r0

		*LEA: r[src] = pc
		ex. lea r0

		RET: pc = r[7]
		ex. ret

		TRAP (vector 0): halts program
		TRAP (vector 1): prints ascii character in r[0]
		TRAP (vector 2): r[7] = pc, pc = r[0], prints null terminated ascii string
		TRAP (vector 3): r[7] = pc, pc = r[0], gets user inputted string with max length stored in r[1] and stores it sequentially at address in r[0]

		ex. trap 0

	**** NOTES ****
	any immediate values in assembly will clamp to be within bounds, e.g. for ADD normal immediate, if you put 60 as the immediate value it will clamp to 15
	any unused bits in a machine code instruction will be set to 0
	strings are NOT packed!!!
	pc is *always* incremented at the start of execution, so instructions that change or use pc such as jmp will use the incremented pc as the base
	sext stands for sign extend and not anything else
	you MUST include trap 0 to halt the program somewhere or else it'll crash
	condition codes are set after instructions with *, so if the result of an instruction with an * is:
		negative 	-> n bit is set
		zero 		-> z bit is set
		positive	-> p bit is set
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef uint16_t reg_t;

typedef enum INSTRUCTION_E {
	NOP,
	ADD,
	AND,
	NOT,
	LD,
	LDI,
	LDR,
	ST,
	STI,
	STR,
	BR,
	JMP,
	JSR,
	LEA,
	RET,
	TRAP
} INSTRUCTION;

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

typedef struct alu_t {
	reg_t accumulator;
} alu_t;

struct cpu_t {
	reg_t regFile[8];
	reg_t ir;
	reg_t pc;
	bool cc[3];
	alu_t alu;
} cpu;

struct ram_t {
	uint16_t memory[0x10000];
	reg_t mar;
	reg_t mdr;
} ram;

int sext(uint16_t val, uint16_t length) {
	int n = 16 - length;
	return (val << n) >> n;
}

void updateCC(uint16_t val) {
	if (val < 0) {
		cpu.cc[N] = true;
		cpu.cc[Z] = false;
		cpu.cc[P] = false;
	} else if (val == 0) {
		cpu.cc[N] = false;
		cpu.cc[Z] = true;
		cpu.cc[P] = false;
	} else {
		cpu.cc[N] = false;
		cpu.cc[Z] = false;
		cpu.cc[P] = true;
	}
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
			switch ((cpu.ir >> 7) & 0x7) {
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
			switch ((cpu.ir >> 7) & 0x7) {
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

			if (!(cpu.ir & 0x100)) {
				cpu.alu.accumulator = !cpu.regFile[(cpu.ir >> 5) & 0x7];
			} else {
				cpu.alu.accumulator = !cpu.regFile[(cpu.ir >> 9) & 0x7];
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
			if (!(cpu.ir & 0x400)) {
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

int parseReg(const char* s) {
	if (!s) {
		return -1;
	}

	if (strlen(s) != 2) {
		return -2;
	}

	if (s[0] != 'R' && s[0] != 'r') {
		return -3;
	}

	if (s[1] < '0' || s[1] > '7') {
		return -4;
	}

	return s[1] - '0';
}

int assemble() {
	memset(ram.memory, 0, sizeof(ram.memory));

	FILE* file;
	char fileName[256];

	printf("Enter source assembly file name: ");
	fgets(fileName, 256, stdin);
	fileName[strcspn(fileName, "\n")] = '\0';
	file = fopen(fileName, "r");
	if (!file) {
		printf("Source file doesn't exist!\n");
		return -1;
	}

	int instructionCount = 0;
	int lineCount = 0;
	char line[256];
	while (fgets(line, 256, file)) {
		uint16_t instruction = 0;
		size_t n = strspn(line, " \t\r");
		char* lineTrimmed = line + n;

		if (lineTrimmed[0] == '\0' || lineTrimmed[0] == ';') {
			lineCount++;
			continue;
		}

		char* tokens[4] = { NULL, NULL, NULL, NULL };
		int tokenCount = 0;
		char* token = strtok(lineTrimmed, " \t\n");
		while (token != NULL) {
			if (tokenCount == 4) {
				printf("Too many tokens on line %d!\n", lineCount);
				return -2;
			}

			tokens[tokenCount] = token;
			tokenCount++;
			token = strtok(NULL, " \t\n");
		}

		if (strcmp(tokens[0], ".fill") == 0) {
			if (tokenCount != 2) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			if (sscanf(tokens[1], "%hx", &instruction) != 1) {
				printf("Wrong token on line %d!\n", lineCount);
				return -3;
			}

		} else if (strcmp(tokens[0], ".stringz") == 0) {
			if (tokenCount != 2) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			for (int i = 0; i < strlen(tokens[1]); i++) {
				ram.memory[instructionCount] = tokens[1][i] & 0x00FF;
				instructionCount++;
			}
			
			ram.memory[instructionCount] = 0;
			instructionCount++;

		} else if (strcmp(tokens[0], "add") == 0) {
			instruction += (1 << 12);

			if (tokenCount == 4) { //normal
				int dst = parseReg(tokens[1]);
				int src0 = parseReg(tokens[2]);
				int src1 = parseReg(tokens[3]);
				if (dst == -1 || src1 == -1) {
					printf("Wrong tokens on line %d!\n", lineCount);
					return -3;
				}

				instruction += (dst << 9);
				instruction += (src0 << 4);
				if (src0 == -1) { //imm
					instruction += (1 << 7);
					int imm = atoi(tokens[3]);
					if (imm > 7) {
						imm = 7;
					}

					if (imm < -8) {
						imm = -8;
					}

					instruction += imm & 0xF;
				} else { //normal
					instruction += (src1 << 1);
				}

			} else if (tokenCount == 3) { //in place
				int dst = parseReg(tokens[1]);
				int src = parseReg(tokens[2]);
				if (dst == -1) {
					printf("Wrong tokens on line %d!\n", lineCount);
					return -3;
				}

				instruction += (dst << 7);
				if (src == -1) { //imm
					instruction += (3 << 10);
					int imm = atoi(tokens[1]);
					if (imm > 63) {
						imm = 63;
					}

					if (imm < -64) {
						imm = -64;
					}

					instruction += imm & 0x7F;
				} else { //normal
					instruction += (2 << 10);
					instruction += (src << 4);
				}

			} else {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

		} else if (strcmp(tokens[0], "and") == 0) {
			instruction += (2 << 12);

			if (tokenCount == 4) { //normal
				int r0 = parseReg(tokens[1]);
				int r1 = parseReg(tokens[2]);
				int r2 = parseReg(tokens[3]);
				if (r0 == -1 || r1 == -1) {
					printf("Wrong tokens on line %d!\n", lineCount);
					return -3;
				}

				instruction += (r0 << 7);
				instruction += (r1 << 4);
				if (r2 == -1) { //imm
					instruction += (1 << 10);
					int imm = atoi(tokens[3]);
					if (imm > 7) {
						imm = 7;
					}

					if (imm < -8) {
						imm = -8;
					}

					instruction += imm & 0xF;
				} else { //normal
					instruction += (r2 << 1);
				}

			} else if (tokenCount == 3) { //in place
				int r0 = parseReg(tokens[1]);
				int r1 = parseReg(tokens[2]);
				if (r0 == -1) {
					printf("Wrong tokens on line %d!\n", lineCount);
					return -3;
				}

				instruction += (r0 << 7);
				if (r1 == -1) { //imm
					instruction += (3 << 10);
					int imm = atoi(tokens[1]);
					if (imm > 63) {
						imm = 63;
					}

					if (imm < -64) {
						imm = -64;
					}

					instruction += imm & 0x7F;
				} else { //normal
					instruction += (2 << 10);
					instruction += (r1 << 4);
				}

			} else {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

		} else if (strcmp(tokens[0], "not") == 0) {
			instruction += (3 << 12);

			int r0 = parseReg(tokens[1]);
			int r1 = parseReg(tokens[2]);
			if (r0 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 8);
			if (r1 == -1) { //imm
				instruction += (1 << 11);
			} else { //normal
				instruction += (r1 << 5);
			}

		} else if (strcmp(tokens[0], "ld") == 0) {
			instruction += (4 << 12);
			if (tokenCount != 3) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int r0 = parseReg(tokens[1]);
			if (r0 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 9);
			int imm = atoi(tokens[2]);
			if (imm > 255) {
				imm = 255;
			}

			if (imm < -256) {
				imm = -256;
			}

			instruction += (imm & 0x1FF);

		} else if (strcmp(tokens[0], "ldi") == 0) {
			instruction += (5 << 12);

			if (tokenCount != 3) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int r0 = parseReg(tokens[1]);
			if (r0 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 9);
			int imm = atoi(tokens[2]);
			if (imm > 255) {
				imm = 255;
			}

			if (imm < -256) {
				imm = -256;
			}

			instruction += (imm & 0x1FF);
		} else if (strcmp(tokens[0], "ldr") == 0) {
			instruction += (6 << 12);

			if (tokenCount != 4) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int r0 = parseReg(tokens[1]);
			int r1 = parseReg(tokens[2]);
			if (r0 == -1 || r1 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 9);
			instruction += (r1 << 6);
			int imm = atoi(tokens[3]);
			if (imm > 31) {
				imm = 31;
			}

			if (imm < -32) {
				imm = -32;
			}

			instruction += (imm & 0x3F);
		} else if (strcmp(tokens[0], "st") == 0) {
			instruction += (7 << 12);

			if (tokenCount != 3) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int r0 = parseReg(tokens[1]);
			if (r0 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 9);
			int imm = atoi(tokens[2]);
			if (imm > 255) {
				imm = 255;
			}

			if (imm < -256) {
				imm = -256;
			}

			instruction += (imm & 0x1FF);
		} else if (strcmp(tokens[0], "sti") == 0) {
			instruction += (8 << 12);

			if (tokenCount != 3) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int r0 = parseReg(tokens[1]);
			if (r0 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 9);
			int imm = atoi(tokens[2]);
			if (imm > 255) {
				imm = 255;
			}

			if (imm < -256) {
				imm = -256;
			}

			instruction += (imm & 0x1FF);
		} else if (strcmp(tokens[0], "str") == 0) {
			instruction += (9 << 12);

			if (tokenCount != 4) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int r0 = parseReg(tokens[1]);
			int r1 = parseReg(tokens[2]);
			if (r0 == -1 || r1 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 9);
			instruction += (r1 << 6);
			int imm = atoi(tokens[3]);
			if (imm > 31) {
				imm = 31;
			}

			if (imm < -32) {
				imm = -32;
			}

			instruction += (imm & 0x3F);
		} else if (strcmp(tokens[0], "br") == 0) {
			instruction += (10 << 12);

			if (tokenCount != 3) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			size_t l = strlen(tokens[2]);
			if (l > 3 || l == 0) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			int conditionCodes = 0;
			for (int i = 0; i < l; i++) {
				if (tokens[2][i] == 'n') {
					conditionCodes |= 0b100;
				} else if (tokens[2][i] == 'z') {
					conditionCodes |= 0b010;
				} else if (tokens[2][i] == 'p') {
					conditionCodes |= 0b001;
				}
			}

			instruction += ((conditionCodes & 0b111) << 9);

			int imm = atoi(tokens[2]);
			if (imm > 255) {
				imm = 255;
			}

			if (imm < -256) {
				imm = -256;
			}

			instruction += (imm & 0x1FF);

		} else if (strcmp(tokens[0], "jmp") == 0) {
			instruction += (11 << 12);

			if (tokenCount != 2) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int r0 = parseReg(tokens[1]);
			if (r0 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += ((r0 & 0x7) << 6);

		} else if (strcmp(tokens[0], "jsr") == 0) {
			instruction += (12 << 12);
			instruction += (1 << 11);
			
			if (tokenCount != 2) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int imm = atoi(tokens[1]);
			if (imm > 1023) {
				imm = 1023;
			}

			if (imm < -1024) {
				imm = -1024;
			}

			instruction += (imm & 0x7FF);

		} else if (strcmp(tokens[0], "jsrr") == 0) {
			instruction += (12 << 12);

			if (tokenCount != 2) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int r0 = parseReg(tokens[2]);
			if (r0 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 8);

		} else if (strcmp(tokens[0], "lea") == 0) {
			instruction += (13 << 12);

			if (tokenCount != 3) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int r0 = parseReg(tokens[1]);
			if (r0 == -1) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 9);
			int imm = atoi(tokens[2]);
			if (imm > 255) {
				imm = 255;
			}

			if (imm < -256) {
				imm = -256;
			}

			instruction += (imm & 0x1FF);

		} else if (strcmp(tokens[0], "ret") == 0) {
			instruction += (14 << 12);
			
			if (tokenCount != 1) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

		} else if (strcmp(tokens[0], "trap") == 0) {
			instruction += (15 << 12);

			if (tokenCount != 2) {
				printf("Wrong amount of tokens on line %d!\n", lineCount);
				return -4;
			}

			int vector = atoi(tokens[1]);
			if (vector < 0 || vector > 15) {
				printf("Bad trap vector on line %d!\n", lineCount);
				return -5;
			}

			instruction += ((vector & 0xF) << 8);

		} else {
			printf("Unkown opcode on line %d!\n", lineCount);
			return -6;
		}

		ram.memory[instructionCount] = instruction;
		instructionCount++;
		lineCount++;
	}

	printf("Starting execution!\n");
	return 0;
}

int main(int argc, char** argv) {
	int assembleReturn = -1;
	while (assembleReturn < 0) {
		assembleReturn = assemble();
	}

	while (true) {
		cpuCycle();
	}

	return 0;
}