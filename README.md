# bob16
extremely simple custom 16 bit cpu emulator and assembler as seen on youtube: https://youtu.be/tlIqosU75CQ?is=ZAVWsYRDMPrUKOej \
use at your own risk! code is thoroughly untested so if you find any bugs let me know asap so i can fix them asap

# INSTRUCTION SEMANTICS
(instructions are in order of opcode, nop is 0, add is 1, etc.)\
(instructions with a * change condition codes)

NOP: does nothing other than go through a cpu cycle

*ADD (normal normal):		r[dst]  = r[src1] + r[src2]\
ex. add r0 r1 r2

*ADD (normal immediate): 	r[dst]  = r[src1] + imm[4]\
ex. add r0 r1 4

*ADD (in place normal): 	r[dst] += r[src1]\
ex. add r0 r1

*ADD (in place immediate): 	r[dst] += imm[7]\
ex. add r0 4

*AND (normal normal): 		r[dst]  = r[src1] & r[src2]\
ex. and r0 r1 r2

*AND (normal immediate): 	r[dst]  = r[src1] & imm[4]\
ex. and r0 r1 4

*AND (in place normal): 	r[dst] &= r[src1]\
ex. and r0 r1

*AND (in place immediate): 	r[dst] &= imm[7]\
ex. and r0 4

*NOT (normal): 		 	r[dst] = !r[src]\
ex. not r0 r1

*NOT (immediate): 	!r[dst]\
ex. not r1

*LD:  r[dst] <= memory[pc + imm[9]]\
ex. ld r0 15

*LDI: r[dst] <= memory[memory[pc + imm[9]]\
ex. ldi r0 15

*LDR: r[dst] <= memory[r[src] + imm[6]]\
ex. ldr r0 r1 15

ST:  r[src] => memory[pc + imm[9]]\
ex. st r0 15

STI: r[src] => memory[memory[pc + imm[9]]\
ex. sti r0 15

STR: r[src] => memory[r[dst] + imm[6]]\
ex. str r0 r1 15

BR: if condition codes match any of the nzp flags, pc += imm[9]\
ex. br np 50\
(jumps to incremented pc + 50 if condition codes n or p are 1)

JMP: pc = r[src]\
ex. jmp r0

JSR:  r[7] = pc, pc += imm[11]\
ex. jsr 50

JSRR: r[7] = pc, pc  = r[src]\
ex. jsrr r0

*LEA: r[src] = pc\
ex. lea r0

RET: pc = r[7]\
ex. ret

TRAP (vector 0): halts program\
TRAP (vector 1): prints ascii character in r[0]\
TRAP (vector 2): r[7] = pc, pc = r[0], prints null terminated ascii string\
TRAP (vector 3): r[7] = pc, pc = r[0], gets user inputted string with max length stored in r[1] and stores it sequentially at address in r[0]

ex. trap 0

**** NOTES ****
any immediate values in assembly will clamp to be within bounds, e.g. for ADD normal immediate, if you put 60 as the immediate value it will clamp to 15\
any unused bits in a machine code instruction will be set to 0\
strings are NOT packed!!!\
pc is *always* incremented at the start of execution, so instructions that change or use pc such as jmp will use the incremented pc as the base\
sext stands for sign extend and not anything else\
you MUST include trap 0 to halt the program somewhere or else it'll crash\
condition codes are set after instructions with *, so if the result of an instruction with an * is:\
negative 	-> n bit is set\
zero 		-> z bit is set\
positive	-> p bit is set
