#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include <stdint.h>

typedef int16_t reg_t;

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

typedef enum TRAP_VECTOR_E { HALT, PUTC, PUTS, GETS } TRAP_VECTOR;

typedef enum CONDITION_CODES_E { N, Z, P } CONDITION_CODES;

typedef struct alu_t {
  reg_t accumulator;
} alu_t;

struct cpu_t {
  reg_t regFile[8];
  reg_t ir;
  reg_t pc;
  bool cc[3];
  alu_t alu;
};

struct ram_t {
  int16_t memory[0x10000];
  reg_t mar;
  reg_t mdr;
};

extern struct cpu_t cpu;
extern struct ram_t ram;

int sext(int16_t val, uint16_t length);
void updateCC(int16_t val);
void fetch(void);
void evalAddress(void);
void fetchOperands(void);
void execute(void);
void storeResult(void);
void cpuCycle(void);

#endif