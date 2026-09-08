#ifndef BOB16_CPU_H
#define BOB16_CPU_H

#include "common.h"

#define RAM_SIZE 0x10000

typedef int16_t reg_t;

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
	int16_t memory[RAM_SIZE];
	reg_t mar;
	reg_t mdr;
} ram;

#endif
