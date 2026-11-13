/*
   IITPSx two-pass assembler      
 
   Name       : Mohd Shuaib  
   Roll No.   : 2502CS09
 
   Declaration of authorship:
     I declare that this source file is my own work, written for the
     CS2102 mini project. 
 
   Usage  : ./asm file.asm
   Output : file.o    binary object file (code starts at address 0)
            file.lst  listing: address, machine word, mnemonic, operand
 
   Design
     Pass 1 reads the source file ONCE. Each line is parsed into a
            SourceLine record (the "internal form") and the symbol table
            (label -> value) is built.
     Pass 2 walks over the stored records, resolves every operand with
            the completed symbol table (so forward references work) and
            writes the object file and the listing together.
     The instruction set lives in one table, not in hard-coded code.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>

#define LINE_SIZE   512     
#define TOKEN_SIZE  128     
#define ERR_SIZE    256     
#define MAX_LINES   10000   
#define MAX_SYMBOLS 1000    

#define OPERAND_MIN (-8388608LL)
#define OPERAND_MAX (8388607LL)

#define OP_DATA (-1)
#define OP_SET  (-2)

#define HEX(x) ((unsigned long)(uint32_t)(x))


typedef enum {
    OPND_NONE,      
    OPND_VALUE,   
    OPND_BRANCH     
} OperandKind;


typedef struct {
    const char *name;       
    int opcode;             
    OperandKind kind;      
} Instruction;

typedef struct {
    int line_no;                
    long long address;          
    char label[TOKEN_SIZE];    
    char operand[TOKEN_SIZE];   
    const Instruction *ins;     
} SourceLine;


typedef struct {
    char name[TOKEN_SIZE];      
    long long value;            
    int is_set;                
    int used;                  
} Symbol;


static const Instruction instructions[] = {
    {"ldc",    0, OPND_VALUE},  {"adc",    1, OPND_VALUE},
    {"ldl",    2, OPND_VALUE},  {"stl",    3, OPND_VALUE},
    {"ldnl",   4, OPND_VALUE},  {"stnl",   5, OPND_VALUE},
    {"add",    6, OPND_NONE},   {"sub",    7, OPND_NONE},
    {"shl",    8, OPND_NONE},   {"shr",    9, OPND_NONE},
    {"adj",   10, OPND_VALUE},  {"a2sp",  11, OPND_NONE},
    {"sp2a",  12, OPND_NONE},   {"call",  13, OPND_BRANCH},
    {"return",14, OPND_NONE},   {"brz",   15, OPND_BRANCH},
    {"brlz",  16, OPND_BRANCH}, {"br",    17, OPND_BRANCH},
    {"halt",  18, OPND_NONE},   {"mul",   19, OPND_NONE},
    {"div",   20, OPND_NONE},
    // pseudo-operations 
    {"data",  OP_DATA, OPND_VALUE},
    {"set",   OP_SET,  OPND_VALUE}
};

#define INSTRUCTION_COUNT (sizeof(instructions) / sizeof(instructions[0]))


static SourceLine source[MAX_LINES];    
static int source_count = 0;

static Symbol symbols[MAX_SYMBOLS];    
static int symbol_count = 0;

static long long program_size = 0;      

static char *skip_space(char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    return s;
}

static int valid_label(const char *s)
{
    size_t i;

    if (!isalpha((unsigned char)s[0]))
        return 0;
    for (i = 1; s[i] != '\0'; i++)
        if (!isalnum((unsigned char)s[i]))
            return 0;
    return 1;
}

static const Instruction *find_instruction(const char *name)
{
    size_t i;

    for (i = 0; i < INSTRUCTION_COUNT; i++)
        if (strcmp(name, instructions[i].name) == 0)
            return &instructions[i];
    return NULL;
}

static int read_token(char **cursor, char *dest)
{
    char *start = *cursor;
    size_t len;

    while (**cursor != '\0' && !isspace((unsigned char)**cursor))
        (*cursor)++;
    len = (size_t)(*cursor - start);
    if (len >= TOKEN_SIZE)
        return 0;
    memcpy(dest, start, len);
    dest[len] = '\0';
    return 1;
}

static int parse_number(const char *text, long long *value)
{
    char *end;

    errno = 0;
    *value = strtoll(text, &end, 0);

    return end != text && *end == '\0' && errno != ERANGE;
}

static void make_filename(const char *input, const char *ext,
                          char *out, size_t size)
{
    const char *slash = strrchr(input, '/');
    const char *bslash = strrchr(input, '\\');
    const char *dot = strrchr(input, '.');
    size_t base_len = strlen(input);

    if (bslash != NULL && (slash == NULL || bslash > slash))
        slash = bslash;
    if (dot != NULL && (slash == NULL || dot > slash))
        base_len = (size_t)(dot - input);
    snprintf(out, size, "%.*s%s", (int)base_len, input, ext);
}


static void write_word(FILE *f, uint32_t word)
{
    int shift;

    for (shift = 0; shift < 32; shift += 8)
        fputc((int)((word >> shift) & 0xFF), f);
}

//  finding the symbol if it exists 
static int find_symbol(const char *name)
{
    int i;

    for (i = 0; i < symbol_count; i++)
        if (strcmp(symbols[i].name, name) == 0)
            return i;
    return -1;
}

// adding a new symbl to teh table
static int define_label(const SourceLine *sl)
{
    long long value = sl->address;
    int is_set = (sl->ins != NULL && sl->ins->opcode == OP_SET);

    // duplicate symbols check 
    if (find_symbol(sl->label) >= 0) {
        printf("Line %d: duplicate label '%s'\n", sl->line_no, sl->label);
        return 1;
    }
    if (symbol_count >= MAX_SYMBOLS) {
        printf("Line %d: too many labels\n", sl->line_no);
        return 1;
    }

    // if is set but the operand is not a no. (as set accepts only no. as operand)
    if (is_set && !parse_number(sl->operand, &value)) {
        printf("Line %d: invalid SET value '%s'\n", sl->line_no, sl->operand);
        return 1;
    }
    strcpy(symbols[symbol_count].name, sl->label); //stored the symbol name
    symbols[symbol_count].value = value; // stored the syml's value(i.e address for all numeric cons fr set)
    symbols[symbol_count].is_set = is_set; // whether is_set or not
    symbols[symbol_count].used = 0; // marked unused for pass 2 
    symbol_count++;
    return 0;
}

// parsing 
static int parse_line(char *text, SourceLine *sl, char *err)
{
    char mnemonic[TOKEN_SIZE];
    char *cursor, *p, *colon;
    size_t len;

    sl->label[0] = '\0';
    sl->operand[0] = '\0';
    sl->ins = NULL;

    p = strchr(text, ';');
    if (p != NULL)
        *p = '\0';
    cursor = skip_space(text);

    colon = strchr(cursor, ':');
    if (colon != NULL) {
        len = (size_t)(colon - cursor);
        if (len >= TOKEN_SIZE) {
            snprintf(err, ERR_SIZE, "label name too long");
            return 0;
        }
        memcpy(sl->label, cursor, len); // chcked len for sl->buffer above (token_size)
        sl->label[len] = '\0';
        if (!valid_label(sl->label)) {
            snprintf(err, ERR_SIZE, "invalid label name '%s'", sl->label);
            return 0;
        }
        cursor = colon + 1;
    }

    cursor = skip_space(cursor);
    if (*cursor == '\0') // just a comment, empty line, or only label:(here instr goes null)
        return 1;

    int ok = read_token(&cursor, mnemonic);
    if (!ok) {
        snprintf(err, ERR_SIZE, "mnemonic too long");
        return 0;
    }

    for (p = mnemonic; *p != '\0'; p++)
        *p = (char)tolower((unsigned char)*p);

    sl->ins = find_instruction(mnemonic);
    if (sl->ins == NULL) {
        snprintf(err, ERR_SIZE, "unknown mnemonic '%s'", mnemonic);
        return 0;
    }

    cursor = skip_space(cursor);
    if (*cursor != '\0') {
        if (!read_token(&cursor, sl->operand)) { // the opernd extracted from here is still text, did the distinction(number or label) in pass2 
            snprintf(err, ERR_SIZE, "operand too long");
            return 0;
        }
        if (*skip_space(cursor) != '\0') {
            snprintf(err, ERR_SIZE, "extra text after operand");
            return 0;
        }
    }

    if (sl->ins->kind != OPND_NONE && sl->operand[0] == '\0') { // eg: ldc (contains no operand but has OPND_VALUE)
        snprintf(err, ERR_SIZE, "missing operand for '%s'", sl->ins->name);
        return 0;
    }

    if (sl->ins->kind == OPND_NONE && sl->operand[0] != '\0') { // eg: add 42 (has OPND_NONE but supplied the opernd)
        snprintf(err, ERR_SIZE, "unexpected operand for '%s'", sl->ins->name);
        return 0;
    }

    if (sl->ins->opcode == OP_SET && sl->label[0] == '\0') {
        snprintf(err, ERR_SIZE, "SET needs a label on the same line"); // valid->limit: set 100, invalid->set 100
        return 0;
    }
    return 1;
}

// pass 1 
static int pass1(FILE *fp)
{
    char text[LINE_SIZE], err[ERR_SIZE];
    int line_no = 0, errors = 0, c;
    long long address = 0;
    SourceLine *sl;
    size_t len;

    while (fgets(text, sizeof text, fp) != NULL) {
        line_no++;

        len = strlen(text);
        if (len == sizeof text - 1 && text[len - 1] != '\n') { // too long line
            printf("Line %d: line too long\n", line_no);
            errors++;
            while ((c = fgetc(fp)) != '\n' && c != EOF)
                ;
            continue;
        }
        if (source_count >= MAX_LINES) { // program's too big 
            printf("Line %d: program too long\n", line_no);
            return errors + 1;
        }

        sl = &source[source_count];
        sl->line_no = line_no;
        sl->address = address;
        if (!parse_line(text, sl, err)) {
            printf("Line %d: %s\n", line_no, err);
            errors++;  
            continue;// didnt do source_count++ as the lien fails to parse. i.e. not included in internal representation
        }

        if (sl->label[0] == '\0' && sl->ins == NULL)
            continue; // skippped empty line 
        source_count++; 

        if (sl->label[0] != '\0') // if line has a label, attempts to add it to the symbol table.
            errors += define_label(sl);

        if (sl->ins != NULL && sl->ins->opcode != OP_SET) // address does not increase for set instruction 
            address++;
    }
    program_size = address;
    return errors;
}

// pass 2
static int resolve_operand(const char *text, long long *value,
                           int *is_label, char *err)
{
    int idx;

    *is_label = 0;
    if (parse_number(text, value)) // first checked if the operaad is number 
        return 1;

   
    if (isalpha((unsigned char)text[0])) { // if the operands first letter is not number 
        idx = find_symbol(text);
        if (idx < 0) { // find_symbol() returns -1 if symbol not found
            snprintf(err, ERR_SIZE, "no such label '%s'", text);
            return 0;
        }
        symbols[idx].used = 1; // bookkeeping to check later if any defined label goes unrefrenced
        *value = symbols[idx].value;
        *is_label = 1;
        return 1;
    }
    snprintf(err, ERR_SIZE, "'%s' is not a valid number or label", text);
    return 0;
}

static int encode(const SourceLine *sl, uint32_t *word, char *err)
{
    const Instruction *ins = sl->ins;
    long long value = 0;
    int is_label = 0;

    if (ins->kind != OPND_NONE &&
        !resolve_operand(sl->operand, &value, &is_label, err))
        return 0;


    if (ins->opcode == OP_DATA) { // handling the data pseudo operation
        if (value < INT32_MIN || value > (long long)UINT32_MAX) { // expected range
            snprintf(err, ERR_SIZE, "data value does not fit in 32 bits");
            return 0;
        }
        *word = (uint32_t)value; // converted the supplied value direct to machine word
        return 1;
    }


    if (ins->kind == OPND_BRANCH && is_label) // branch instruction
        value -= sl->address + 1; // offset = target - (current addr+1)

    if (value < OPERAND_MIN || value > OPERAND_MAX) {
        snprintf(err, ERR_SIZE, "operand %lld does not fit in 24 bits", value);
        return 0;
    }
    *word = (((uint32_t)value & 0xFFFFFFu) << 8) | (uint32_t)ins->opcode;
    return 1;
}


static int pass2(FILE *obj, FILE *lst)
{
    char err[ERR_SIZE];
    uint32_t word;
    int i, idx, errors = 0;
    long long value;

    printf("\nPass 2:\n");
    for (i = 0; i < source_count; i++) {
        const SourceLine *sl = &source[i];
        const Instruction *ins = sl->ins;

        if (sl->label[0] != '\0') {
            idx = find_symbol(sl->label);
            if (idx >= 0)
                value = symbols[idx].value;
            else
                value = sl->address;

            if (ins != NULL && ins->opcode == OP_SET) // the line set directive
                fprintf(lst, "%08lX %s: SET %s\n",
                        HEX(value), sl->label, sl->operand);
            else
                fprintf(lst, "%08lX %s:\n", HEX(value), sl->label);
        }

        if (ins == NULL || ins->opcode == OP_SET)
            continue;

        if (!encode(sl, &word, err)) {
            printf("Line %d: %s\n", sl->line_no, err);
            errors++;
            continue;
        }
        write_word(obj, word); // writing word to obj file by extracting four bytes at a time in little endian order 

        if (sl->operand[0] != '\0')
            fprintf(lst, "%08lX %08lX %s %s\n",
                    HEX(sl->address), HEX(word), ins->name, sl->operand);
        else // Instructions without operands 
            fprintf(lst, "%08lX %08lX %s\n",
                    HEX(sl->address), HEX(word), ins->name);
    }
    return errors;
}


int main(int argc, char *argv[])
{
    char obj_name[LINE_SIZE], lst_name[LINE_SIZE];
    FILE *src, *obj, *lst;
    int errors, i;

    if (argc != 2) {
        printf("Usage: %s source.asm\n", argv[0]);
        return 1;
    }

    src = fopen(argv[1], "r");
    if (src == NULL) {
        printf("Error: cannot open '%s'\n", argv[1]);
        return 1;
    }

    make_filename(argv[1], ".o", obj_name, sizeof obj_name);
    make_filename(argv[1], ".lst", lst_name, sizeof lst_name);
    obj = fopen(obj_name, "wb");
    lst = fopen(lst_name, "w");
    if (obj == NULL || lst == NULL) {
        printf("Error: cannot create '%s' or '%s'\n", obj_name, lst_name);
        fclose(src);
        if (obj != NULL)
            fclose(obj);
        if (lst != NULL)
            fclose(lst);
        return 1;
    }

    errors = pass1(src);
    fclose(src);
    printf("Pass 1 completed: %d error(s), program size %lld word(s)\n",
           errors, program_size);
    printf("Symbol table:\n");
    for (i = 0; i < symbol_count; i++)
        printf("  %-20s %lld%s\n", symbols[i].name, symbols[i].value,
               symbols[i].is_set ? " (SET)" : "");


    errors += pass2(obj, lst);
    fclose(obj);
    fclose(lst);

    if (errors > 0) {
        remove(obj_name);
        remove(lst_name);
        printf("\nAssembly FAILED: %d error(s), no output written.\n", errors);
        return 1;
    }

    for (i = 0; i < symbol_count; i++)
        if (!symbols[i].used)
            printf("Warning: label '%s' is never used\n", symbols[i].name);

    printf("\nAssembly succeeded: %s and %s written.\n", obj_name, lst_name);
    return 0;
}