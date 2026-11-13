# Two-Pass Assembler and Emulator in C

A two-pass assembler (`asm.c`) and an emulator (`emu.c`) for the IITPSx instruction set, written from scratch in C99. The assembler turns assembly source into a 32-bit object file and a listing file, and the emulator loads that object file and executes it.

Built as a solo project for the DLCO course at IIT Patna.

## Build

Both programs compile with no errors or warnings:
  gcc -std=c99 -pedantic -W -Wall -Wpointer-arith -Wwrite-strings -Wstrict-prototypes asm.c -o asm
  gcc -std=c99 -pedantic -W -Wall -Wpointer-arith -Wwrite-strings -Wstrict-prototypes emu.c -o emu
  
## Usage
./asm file.asm                    # writes file.o (object) and file.lst (listing)
./emu [-trace] [-before] [-after] file.o
sh run_tests.sh                   # assembles and runs every test, producing .log files


| Emulator option | What it does |
|-----------------|--------------|
| `-trace`  | Prints every executed instruction |
| `-before` | Prints a memory dump before execution |
| `-after`  | Prints a memory dump after execution, including non-zero words beyond the program |

## Assembler (`asm.c`)

- **Pass 1** reads the source into an internal form: one `SourceLine` record per statement plus a symbol table.
- **Pass 2** works from that stored form to generate machine code.
- Instruction names, opcodes and expected operand kinds are kept in a single table.
- Supports the `SET` pseudo-instruction (including use before definition) and the `mul` and `div` instructions.
- Branch and call with a label use the displacement `label - (address + 1)`. A numeric operand is used as the raw offset.
- Produces a binary object file: code starts at address 0, one 32-bit word per statement, least significant byte first.
- Produces a listing file with the address, machine word, mnemonic and operand, and labels on their own lines.
- Warns about labels that are never used.

### Errors detected

Duplicate label, no such label, invalid label name, unknown mnemonic, missing operand, unexpected operand, extra text after operand, invalid number, `SET` without a label, invalid `SET` value, and operand not fitting in 24 bits.

All errors in the file are reported in one run. If any error occurs, no output files are written.

## Emulator (`emu.c`)

- Loads the object file produced by the assembler and runs all 21 opcodes.
- Handles signed `mul` and `div`, including negative operands.
- Starts with SP at the top of memory (65536).
- Detects errant programs: PC leaving the program, read or write outside memory, illegal opcode, shift count outside 0 to 31, and runaway loops (limit of 10,000,000 instructions).
- Detects division by zero as a run-time error (exit status 2).

## Example Programs and Tests

Every test is assembled with `./asm`, and the runnable ones are executed with `./emu -before -after`. The outputs are saved as `.log` files.

| Test | What it checks | Result |
|------|----------------|--------|
| `test01` | Valid program with a forward reference | Assembles, unused-label warning |
| `test02` | Error handling | All 9 errors reported, no output files |
| `test03` | `SET` used before it is defined | Assembles |
| `test04` | Binomial coefficients (row 10), recursive call/return | `1 10 45 120 210 252 210 120 45 10`, SP back at 4096 |
| `test05` | Sum 10+9+...+1 with stack locals and `SET` | Assembler-only |
| `test06` | Unused-label warnings | Assembler-only |
| `test07` | Ten deliberate errors in one file | All 10 reported, no output files |
| `test08` | Sum of 1..10 with a loop | A = 55 |
| `test09` | Polynomial 3x² + 2x + 1 at x = 5 (uses `mul`) | A = 86 |
| `test10` | Mean of 5 numbers (uses `div`) | A = 30 |
| `test11` | Factorial of 5 (uses `mul`) | A = 120 |
| `test12` | Division by zero | Run-time error |
| `test13` | Signed `mul` and `div` with negative operands | A = 21 |
| `test14` | Procedure call/return using the stack | A = 42 |
| `test15` | `mul` and `div` sample | A = 21 |
| `bubble` | Bubble sort of 10 numbers | `-7 5 11 12 22 22 25 34 64 90` |

`test01`, `test05`, `test06` are assembler-only. `test01` and `test06` contain infinite loops, and `test05` uses stack locals without setting SP first.

## Sample Error Output

From `test02.asm`:
Line 4: duplicate label 'label'
Line 7: missing operand for 'ldc'
Line 8: unexpected operand for 'add'
Line 9: extra text after operand
Line 10: invalid label name '0def'
Line 11: unknown mnemonic 'fibble'
Line 12: unknown mnemonic '0def'
Line 5: no such label 'nonesuch'
Line 6: '08ge' is not a valid number or label
Assembly FAILED: 9 error(s), no output written.
