#include "basm.h"
#include "instruction.h"
#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>

int parseReg(const char* s) {
	if (!s) {
		//string doesn't exist
		return -1;
	}

	if (strlen(s) != 2 || s[0] != 'R' && s[0] != 'r') {
		//incorrect format
		return -2;
	}

	if (s[1] < '0' || s[1] > '7') {
		//out of bounds
		return -3;
	}

	return s[1] - '0';
}

int assemble() {
    int16_t *binaryRom = calloc(RAM_SIZE, sizeof(int16_t));

    if (binaryRom == NULL) {
        fprintf(stderr, "Could not allocate enough memory.\n");
        exit(1);
    }

	FILE* inFile;
	char inFileName[256];

	setbuf(stdout, NULL);
	printf("Enter source assembly file name: \n");
	fgets(inFileName, 256, stdin);
	inFileName[strcspn(inFileName, "\n")] = '\0';
	inFile = fopen(inFileName, "r");
	if (!inFile) {
		printf("Source file doesn't exist!\n");
		return -1;
	}

	int instructionCount = 0;
	int lineCount = 0;
	char line[256];
	while (fgets(line, 256, inFile)) {
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

		if (tokenCount == 0) {
			lineCount++;
			continue;
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
				binaryRom[instructionCount] = tokens[1][i] & 0x00FF;
				instructionCount++;
			}
			
			binaryRom[instructionCount] = 0;
			instructionCount++;
			if (instructionCount > 0xFFFF) {
				printf("Too many instructions!\n");
				return -7;
			}

		} else if (strcmp(tokens[0], "add") == 0) {
			instruction += (1 << 12);

			if (tokenCount == 4) { //normal
				int dst = parseReg(tokens[1]);
				int src0 = parseReg(tokens[2]);
				int src1 = parseReg(tokens[3]);
				if (dst < 0 || src1 < 0) {
					printf("Wrong tokens on line %d!\n", lineCount);
					return -3;
				}

				instruction += (dst << 9);
				instruction += (src0 << 4);
				if (src0 < 0) { //imm
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
				if (dst < 0) {
					printf("Wrong tokens on line %d!\n", lineCount);
					return -3;
				}

				instruction += (dst << 9);
				if (src < 0) { //imm
					instruction += (4 << 7);
					int imm = atoi(tokens[2]);
					if (imm > 63) {
						imm = 63;
					}

					if (imm < -64) {
						imm = -64;
					}

					instruction += imm & 0x7F;
				} else { //normal
					instruction += (3 << 7);
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
				if (r0 < 0 || r1 < 0) {
					printf("Wrong tokens on line %d!\n", lineCount);
					return -3;
				}

				instruction += (r0 << 9);
				instruction += (r1 << 4);
				if (r2 < 0) { //imm
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
					instruction += (r2 << 1);
				}

			} else if (tokenCount == 3) { //in place
				int r0 = parseReg(tokens[1]);
				int r1 = parseReg(tokens[2]);
				if (r0 < 0) {
					printf("Wrong tokens on line %d!\n", lineCount);
					return -3;
				}

				instruction += (r0 << 7);
				if (r1 < 0) { //imm
					instruction += (4 << 10);
					int imm = atoi(tokens[2]);
					if (imm > 63) {
						imm = 63;
					}

					if (imm < -64) {
						imm = -64;
					}

					instruction += imm & 0x7F;
				} else { //normal
					instruction += (3 << 7);
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
			if (r0 < 0) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += (r0 << 9);
			if (r1 < 0) { //imm
				instruction += (1 << 8);
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

			size_t l = strlen(tokens[1]);
			if (l > 3 || l == 0) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			int conditionCodes = 0;
			for (int i = 0; i < l; i++) {
				if (tokens[1][i] == 'n') {
					conditionCodes |= 0b100;
				} else if (tokens[1][i] == 'z') {
					conditionCodes |= 0b010;
				} else if (tokens[1][i] == 'p') {
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
			if (r0 < 0) {
				printf("Wrong tokens on line %d!\n", lineCount);
				return -3;
			}

			instruction += ((r0 & 0x7) << 9);

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
			if (r0 < 0) {
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
			if (r0 < 0) {
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

		binaryRom[instructionCount] = instruction;
		instructionCount++;
		if (instructionCount > 0xFFFF) {
			printf("Too many instructions!\n");
			return -7;
		}

		lineCount++;
	}

    char outFileName[256];
    strcpy(outFileName, inFileName);
    outFileName[strlen(outFileName) - 1] = '\0';
    outFileName[strlen(outFileName) - 1] = 'b';
    outFileName[strlen(outFileName) - 2] = 'o';
    outFileName[strlen(outFileName) - 3] = 'b';

    FILE *outFile = fopen(outFileName, "wb");
    fwrite(binaryRom, RAM_SIZE, sizeof(int16_t), outFile);

	printf("Written to output file: %s\n", outFileName);
	return 0;
}

int main() {
    assemble();
}
