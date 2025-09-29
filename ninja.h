#ifndef NINJA_H
#define NINJA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>   
#include "bigint.h"

#define VM_VERSION 8


#define DEFAULT_STACK_SIZE_KB 64
#define DEFAULT_HEAP_SIZE_KB  8192   

#define IMMEDIATE(x) ((x) & 0x00FFFFFF)
#define SIGN_EXTEND(i) ((i) & 0x00800000 ? (i) | 0xFF000000 : (i))


#define MSB (1 << (8 * sizeof(unsigned int) - 1))
#define IS_PRIM(obj) (((obj)->size & MSB) == 0)
#define GET_SIZE(obj) ((obj)->size & ~MSB)
#define GET_REFS(obj) ((ObjRef *)(obj)->data)


#define HALT    0
#define PUSH    1
#define ADD     2
#define SUB     3
#define MUL     4
#define DIV     5
#define MOD     6
#define RDINT   7
#define WRINT   8
#define RDCHR   9
#define WRCHR   10
#define PUSHG   11
#define POPG    12
#define ASF     13
#define RSF     14 
#define PUSHL   15
#define POPL    16
#define EQ      17
#define NE      18
#define LT      19
#define LE      20
#define GT      21
#define GE      22
#define JMP     23
#define BRF     24
#define BRT     25
#define CALL    26
#define RET     27
#define DROP    28
#define PUSHR   29
#define POPR    30
#define DUP     31
#define NEW     32
#define GETF    33
#define PUTF    34
#define NEWA    35
#define GETFA   36
#define PUTFA   37
#define GETSZ   38
#define PUSHN   39
#define REFEQ   40
#define REFNE   41

typedef struct {
    unsigned int size;  
    unsigned char data[];
} *ObjRef;

typedef struct {
    int isObjRef;
    union {
        ObjRef objRef;
        int number;
    } u;
} StackSlot;


extern StackSlot *stack;         
extern int stack_slots;          

extern ObjRef *staticData;
extern ObjRef returnValue;
extern BIP bip;


extern char *heap;               
extern size_t heap_size;         
extern char *heap_ptr;           
extern size_t heap_half_size;    
extern int current_heap_half;    


FILE* parse_args(int argc, char* argv[]);
void load_program(FILE* fp, const char* file_name, unsigned int** instruction,
                 unsigned int** data, unsigned int* instruction_counter,
                 unsigned int* data_count);
void execute(unsigned int IR, StackSlot stack[], int* sp, int* halt, ObjRef data[],
            unsigned int data_count, int* frame_pointer, int* pc,
            int debug_mode, int is_run_command, ObjRef* return_value);
void print_instruction(int index, unsigned int instr);
void run_vm(unsigned int* instruction, ObjRef data[],
           unsigned int instruction_counter, unsigned int data_count,
           StackSlot stack[], int debug_mode);
void run_debugger(unsigned int* instruction, ObjRef data[],
                 unsigned int instruction_counter, unsigned int data_count,
                 StackSlot stack[], int debug_mode);


void * newPrimObject(int dataSize);
ObjRef newCompoundObject(int numObjRefs);


void print_heap_status(void);
size_t get_heap_half_size(void);
void switch_heap_half(void);

void fatalError(char *msg);



#endif
