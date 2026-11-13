/*
IITPSx emulator (CS2102 Mini Project 2026)

Name       : Mohd Shuaib
Roll No.   : 2502CS09

Declaration of authorship:
    I declare that this source file is my own work, written for the
    CS2102 mini project.

Usage : ./emu [-trace] [-before] [-after] file.o
        -trace   print every executed instruction and the registers
        -before  print a memory dump of the loaded program
         -after   print a memory dump when the program has stopped
             (program words, then any non-zero words beyond it)

The emulator loads the object file written by asm.c (32-bit words,
least significant byte first) at address 0 and runs it from PC = 0
until HALT or until a run-time error is detected.

Registers: A and B (32-bit), PC and SP. SP starts at the top of
memory. Exit status: 0 = HALT reached, 1 = usage or load error,
2 = run-time error in the emulated program.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MEM_WORDS  65536UL     
#define MAX_STEPS  10000000UL   
#define OPCODES    21           

static const char *op_name[OPCODES] = {
    "ldc", "adc", "ldl", "stl", "ldnl", "stnl", "add", "sub", "shl", "shr",
    "adj", "a2sp", "sp2a", "call", "return", "brz", "brlz", "br", "HALT",
    "mul", "div"
};
static const int op_has_operand[OPCODES] = {
    1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
    1, 0, 0, 1, 0, 1, 1, 1, 0,
    0, 0
};


static int32_t mem[MEM_WORDS]; // the memory, one word per address   
static int32_t reg_a, reg_b;// two registers forming a stack  
static int32_t reg_pc, reg_sp; // program counter, stack pointer 

static long program_words = 0; // number of words loaded from file 
static long cur_pc = 0;// address of the instruction running
static unsigned long steps = 0;    // instructions executed

static void show_registers(void)
{
    printf("A=%ld (0x%08lX)  B=%ld (0x%08lX)  PC=%ld  SP=%ld\n",
           (long)reg_a, (unsigned long)(uint32_t)reg_a,
           (long)reg_b, (unsigned long)(uint32_t)reg_b,
           (long)reg_pc, (long)reg_sp);
}

// run-time error 
static void fail(const char *message)
{
    printf("RUNTIME ERROR: %s\n", message);
    printf("  instruction #%lu at address %ld\n", steps, cur_pc);
    printf("  ");
    show_registers();
    exit(2);
}

// Reading the word at address addr
static int32_t mem_read(long addr)
{
    if (addr < 0 || addr >= (long)MEM_WORDS)
        fail("read from an address outside memory");
    return mem[addr];
}

// Writimg a word to address addr
static void mem_write(long addr, int32_t value)
{
    if (addr < 0 || addr >= (long)MEM_WORDS)
        fail("write to an address outside memory"); // the whole program would terminate

    mem[addr] = value;
}

// Print words 0..count-1 of memory, four per line, with the address
static void dump_memory(const char *title, long count)
{
    long i;

    printf("%s (%ld word(s)):\n", title, count);
    for (i = 0; i < count; i++) {
        if (i % 4 == 0)
            printf("%08lX", (unsigned long)i);

        printf(" %08lX", (unsigned long)(uint32_t)mem[i]);

        if (i % 4 == 3 || i == count - 1)
            printf("\n");
    }
}

static void dump_outside_program(void)
{
    long i;
    int found = 0;

    for (i = program_words; i < (long)MEM_WORDS; i++) {
        if (mem[i] == 0)
            continue;

        if (!found)
            printf("Non-zero words outside the loaded program:\n");

        found = 1;
        printf("%08lX %08lX  (%ld)\n", (unsigned long)i,
               (unsigned long)(uint32_t)mem[i], (long)mem[i]);
    }
    if (!found)
        printf("No non-zero words outside the loaded program.\n");
}

// Load the object file into memory from address 0
static long load_program(const char *path)
{
    FILE *fp = fopen(path, "rb");
    long count = 0;
    int shift, c;
    uint32_t word;

    if (fp == NULL) {
        printf("Error: cannot open '%s'\n", path);
        return -1;
    }

    for (;;) {
        word = 0;
        for (shift = 0; shift < 32; shift += 8) {
            c = fgetc(fp);
            if (c == EOF) {
                fclose(fp);
                if (shift != 0) {
                    printf("Error: '%s' is not a whole number of words\n", path);
                    return -1;
                }
                return count;
            }

            word |= (uint32_t)c << shift;   // least significant byte first(little endian)
        }

        if (count >= (long)MEM_WORDS) {
            printf("Error: program does not fit in memory\n");
            fclose(fp);
            return -1;
        }
        mem[count++] = (int32_t)word;
    }
}

// Run the loaded program until HALT
static void run(int trace)
{
    int halted = 0;
    uint32_t word;
    int opcode;
    int32_t operand;

    while (!halted) {
        if (steps >= MAX_STEPS)
            fail("instruction limit reached, probably an infinite loop");

        cur_pc = reg_pc;
        if (reg_pc < 0 || reg_pc >= program_words)
            fail("PC left the program (missing HALT or bad branch?)");

        word = (uint32_t)mem[reg_pc];
        reg_pc++;
        steps++;

        // low 8 bits = opcode, upper 24 bits = signed operand
        opcode = (int)(word & 0xFFu);
        operand = (int32_t)(word >> 8);

        if (operand & 0x800000) // Sign extension
            operand -= 0x1000000;   

        if (opcode >= OPCODES)
            fail("illegal opcode");

        switch (opcode) {
        case 0:  reg_b = reg_a; reg_a = operand; break; // ldc 

        case 1:  reg_a = (int32_t)((uint32_t)reg_a + (uint32_t)operand);// adc  
                 break;                                             

        case 2:  reg_b = reg_a;// ldl  
                 reg_a = mem_read((long)reg_sp + operand); break;

        case 3:  mem_write((long)reg_sp + operand, reg_a);// stl  
                 reg_a = reg_b; break;

        case 4:  reg_a = mem_read((long)reg_a + operand); break; // ldnl 

        case 5:  mem_write((long)reg_a + operand, reg_b); break; // stnl 

        case 6:  reg_a = (int32_t)((uint32_t)reg_b + (uint32_t)reg_a);// add
                 break;                                              

        case 7:  reg_a = (int32_t)((uint32_t)reg_b - (uint32_t)reg_a); // sub 
                 break;                                             

        case 8:  if (reg_a < 0 || reg_a > 31) // shl  
                     fail("shift count outside 0..31");
                 reg_a = (int32_t)((uint32_t)reg_b << reg_a); break;

        case 9:  if (reg_a < 0 || reg_a > 31)// shr 
                     fail("shift count outside 0..31");
                 reg_a = reg_b >> reg_a; break;

        case 10: reg_sp += operand; break; // adj  

        case 11: reg_sp = reg_a; reg_a = reg_b; break;// a2sp 

        case 12: reg_b = reg_a; reg_a = reg_sp; break;// sp2a 

        case 13: reg_b = reg_a; reg_a = reg_pc;// call 
                 reg_pc += operand; break;

        case 14: reg_pc = reg_a; reg_a = reg_b; break; // return 

        case 15: if (reg_a == 0) reg_pc += operand; break; // brz  

        case 16: if (reg_a < 0) reg_pc += operand; break; // brlz 

        case 17: reg_pc += operand; break; // br   

        case 18: halted = 1; break; // HALT 

        case 19: reg_a = (int32_t)((uint32_t)reg_b * (uint32_t)reg_a); // mul
                 break;                                               

        case 20: if (reg_a == 0) // div 
                     fail("division by zero");
                 if (reg_b == INT32_MIN && reg_a == -1)
                     reg_a = INT32_MIN;    
                 else
                     reg_a = reg_b / reg_a;
                 break;
        }

        if (trace) {
            printf("%08lX  %-6s", (unsigned long)cur_pc, op_name[opcode]);
            if (op_has_operand[opcode])
                printf(" %-9ld", (long)operand);
            else
                printf(" %-9s", "");
            printf(" A=%ld B=%ld SP=%ld\n",
                   (long)reg_a, (long)reg_b, (long)reg_sp);
        }
    }
}


int main(int argc, char *argv[])
{
    const char *path = NULL;
    int trace = 0, before = 0, after = 0, i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-trace") == 0)
            trace = 1;

        else if (strcmp(argv[i], "-before") == 0)
            before = 1;

        else if (strcmp(argv[i], "-after") == 0)
            after = 1;

        else if (path == NULL)
            path = argv[i];

        else
            path = NULL, i = argc;         
    }

    if (path == NULL) {
        printf("Usage: %s [-trace] [-before] [-after] file.o\n", argv[0]);
        return 1;
    }

    program_words = load_program(path);

    if (program_words < 0)
        return 1;

    printf("Loaded %ld word(s) from %s\n", program_words, path);
    
    if (before)
        dump_memory("Memory dump before execution", program_words);

    reg_a = reg_b = reg_pc = 0;
    reg_sp = (int32_t)MEM_WORDS;

    run(trace);

    printf("HALT reached after %lu instruction(s)\n", steps);
    printf("Final registers: ");
    show_registers();
    if (after) {
        dump_memory("Memory dump after execution", program_words);
        dump_outside_program();
    }
    return 0;
}