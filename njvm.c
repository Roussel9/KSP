#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ninja.h"
#include <stdbool.h>


StackSlot *stack = NULL;
int stack_slots = 0;            
int stack_size_kb = DEFAULT_STACK_SIZE_KB;
int heap_size_kb = DEFAULT_HEAP_SIZE_KB;

char *heap = NULL;
size_t heap_size = 0;
char *heap_ptr = NULL;
size_t heap_half_size = 0;
int current_heap_half = 0;

ObjRef *staticData = NULL;
ObjRef returnValue = NULL;



int main(int argc, char* argv[]) {
    FILE* fp;
    unsigned int instruction_counter;
    unsigned int data_count;
    unsigned int *instruction;
    ObjRef *data;  
    int debug_mode = 0;
    

    if (argc >= 2 && strcmp(argv[1], "--debug") == 0) {
        debug_mode = 1;
        if (argc < 3) {
            printf("Error: no code file specified\n");
            exit(99);
        }
        fp = parse_args(argc - 1, &argv[1]);
    } else {
        fp = parse_args(argc, argv);
    }

    load_program(fp, argv[argc - 1], &instruction, (unsigned int**)&data, &instruction_counter, &data_count);

    if (debug_mode) {
        printf("DEBUG: file '%s' loaded (code size = %u, data size = %u)\n",
            argv[argc - 1], instruction_counter, data_count);
    }

    fclose(fp);

    
    staticData = malloc(data_count * sizeof(ObjRef));
    if (!staticData) {
        fatalError("Out of memory for static data");
    }
    for (unsigned i = 0; i < data_count; i++) {
        staticData[i] = NULL;  
    }

    
heap_size = (size_t)heap_size_kb * 1024;
heap = malloc(heap_size);
if (!heap) fatalError("Out of memory for heap");
heap_half_size = heap_size / 2;
current_heap_half = 0;
heap_ptr = heap;


size_t stack_size_bytes = (size_t)stack_size_kb * 1024;
stack_slots = stack_size_bytes / sizeof(StackSlot);
stack = malloc(stack_slots * sizeof(StackSlot));
if (!stack) fatalError("Out of memory for stack");


for (int i = 0; i < stack_slots; i++) {
     stack[i].isObjRef = 1;  
    stack[i].u.objRef = NULL;  
}


    if (debug_mode) {
        run_debugger(instruction, staticData, instruction_counter, data_count, stack, debug_mode);
    } else {
        run_vm(instruction, staticData, instruction_counter, data_count, stack, debug_mode);
    }

    free(instruction);
    free(staticData);
    
    return 0;
}


void run_vm(unsigned int* instruction, ObjRef data[],
    unsigned int instruction_counter, unsigned int data_count,
    StackSlot stack[], int debug_mode) {

int pc = 0, halt = 0, sp = 0, fp = 0;
unsigned int IR;
ObjRef return_value = NULL; 

printf("Ninja Virtual Machine started\n");
int is_run_command = 1;

while (!halt && pc < instruction_counter) {
IR = instruction[pc++];
execute(IR, stack, &sp, &halt, data, data_count, &fp, &pc,debug_mode,is_run_command,&return_value);
}

printf("Ninja Virtual Machine stopped\n");
}


void run_debugger(unsigned int* instruction, ObjRef data[],
    unsigned int instruction_counter, unsigned int data_count,
    StackSlot stack[], int debug_mode) {
    int pc = 0, halt = 0, sp = 0, fp = 0;
    unsigned int IR;
    int breakpoint = -1;
    char input[100];
    ObjRef return_value = NULL; 
    int is_run_command = 0;

    printf("Ninja Virtual Machine started\n");

    while (!halt && pc < instruction_counter) {
        print_instruction(pc, instruction[pc]);
        int done = 0;

        while (!done && !halt) {
            printf("DEBUG: inspect, list, breakpoint, step, run, quit?\n");
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;

            switch(input[0]){
                case'i':
                    printf("DEBUG [inspect]: stack, data, object?\n");
                    fgets(input, sizeof(input), stdin);
                    input[strcspn(input, "\n")] = 0;
                    if (input[0] == 's') {
        if (sp == 0) {
            printf("sp, fp  --->\t%04d:\t(xxxxxx) xxxxxx\n", sp);
            
            printf("\t\t--- bottom of stack ---\n");
            print_instruction(pc, instruction[pc]);
            break;
            
        } else {
            for (int i = sp; i >= 0; i--) {
                bool already_printed = false;

               
                if (i == sp && i == fp) {
                    printf("sp, fp  --->\t%04d:\t", i);
                    already_printed = true;
                } else if (i == sp) {
                    printf("sp      --->\t%04d:\t", i);
                    already_printed = true;
                } else if (i == fp) {
                    printf("fp      --->\t%04d:\t", i);
                } else {
                    printf("\t\t%04d:\t", i);
                }

               
                if (!already_printed) {
                    if (stack[i].isObjRef) {
                        if (stack[i].u.objRef) {
                            printf("(objref) %p\n", (void*)stack[i].u.objRef);
                        } else {
                           printf("(objref) nil\n");  
                        }
                    } else {
                        printf("(number) %d\n", stack[i].u.number);
                    }
                } else {
                   
                    printf("(xxxxxx) xxxxxx\n");
                }
            }
            printf("\t\t--- bottom of stack ---\n");
        }
    } else if (input[0] == 'd') {
                        if (data_count == 0) {
                            printf("\t--- end of data ---\n");
                            print_instruction(pc, instruction[pc]);
                        } else {
                            for (unsigned int i = 0; i < data_count; i++) {
                                printf("\t%04d: (objref) 0x%p\n", i, (void*)staticData[i]);
                            }
                        }
                    }if (input[0] == 'o') {
    printf("object reference?\n");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    
    
    char* ptr = input;
    if (strncmp(ptr, "0x", 2) == 0) {
        ptr += 2;
    }
    
    unsigned long addr;
    if (sscanf(ptr, "%lx", &addr) == 1) {
        ObjRef obj = (ObjRef)addr;
        
        if (obj == NULL) {
            printf("(nil)\n");
        } else if (IS_PRIM(obj)) {
            printf("<primitive object>\n");
            bip.op1 = obj;
            printf("value:\t\t");
            bigPrint(stdout);
            printf("\n");
        } else {
            printf("<compound object>\n");
            ObjRef *refs = GET_REFS(obj);
            for (int i = 0; i < GET_SIZE(obj); i++) {
                printf("field[%04d]:\t", i);  
                if (refs[i]) {
                    printf("(objref) %p\n", (void*)refs[i]);
                } else {
                    printf("(objref) nil\n");
                }
            }
        }
        printf("\t--- end of object ---\n");
    } else {
        printf("Invalid object reference\n");
    }
}
                    print_instruction(pc, instruction[pc]);
                    break;
                    

                    break;

                case 'l':
                    for (unsigned int i = 0; i < instruction_counter; i++) {
                        print_instruction(i, instruction[i]);
                    }

                    printf("\t--- end of code ---\n");
                    print_instruction(pc, instruction[pc]);
                    break;

                case 'b':
                    printf("DEBUG [breakpoint]: cleared\n");
                    printf("DEBUG [breakpoint]: address to set, -1 to clear, <ret> for no change?\n");
                    fgets(input, sizeof(input), stdin);
                    input[strcspn(input, "\n")] = 0;
                    if (strlen(input) > 0) {
                        breakpoint = atoi(input);
                        if (breakpoint == -1){
                            printf("DEBUG [breakpoint]: now cleared\n");
                            print_instruction(pc, instruction[pc]); 
                        }
                    }
                    break;

                case 's':{
                    int is_run_command = 0;
                    IR = instruction[pc++];
                    execute(IR, stack, &sp, &halt, data, data_count, &fp, &pc,debug_mode,is_run_command,&return_value);
                    if (!halt && pc < instruction_counter) {
                        print_instruction(pc, instruction[pc]);  
                    }
                    break;
                }
                case 'r':{
                    int is_run_command = 1;
                    do {
                        IR = instruction[pc++];
                        execute(IR, stack, &sp, &halt, data, data_count, &fp, &pc,debug_mode,is_run_command,&return_value);                    
                    } while (!halt && (breakpoint == -1 || pc != breakpoint));
                    done = 1;
                    break;
                }
                case 'q':
                    halt = 1;
                    done = 1;
                    break;

                default: {
                    int val = atoi(input);
                    if (val != 0 || input[0] == '0') {
                        printf("%d\n", val);
                    }
                }
            }
        }

        if (pc == breakpoint) {
            printf("DEBUG: Breakpoint %d reached.\n", pc);execute(IR, stack, &sp, &halt, data, data_count, &fp, &pc, debug_mode, is_run_command, &returnValue);
        }
    }

    printf("Ninja Virtual Machine stopped\n");
}


BigObjRef createBigIntObject(int value) {
    bigFromInt(value);  
    return bip.res;
}



int getIntFromObject(ObjRef obj) {
    if (!obj || obj->size != sizeof(int)) {
        printf("Error: Invalid integer object\n");
        exit(99);
    }
    return *(int*)obj->data;
}



void switch_heap_half(void) {
    current_heap_half = 1 - current_heap_half;
    heap_ptr = heap + current_heap_half * heap_half_size;
}


ObjRef newCompoundObject(int numObjRefs) {
    size_t total = sizeof(unsigned int) + numObjRefs * sizeof(ObjRef);
    
   
    size_t alignment_padding = (sizeof(void*) - ((uintptr_t)heap_ptr % sizeof(void*))) % sizeof(void*);
    heap_ptr += alignment_padding;

    
    if (heap_ptr + total > heap + (current_heap_half + 1) * heap_half_size) {
        switch_heap_half();
        
        
        if (heap_ptr + total > heap + (current_heap_half + 1) * heap_half_size) {
            printf("Error: heap overflow\n");
            exit(99);
            //print_heap_status();
            //fatalError("Out of memory in heap");
        }
    }

    ObjRef obj = (ObjRef)heap_ptr;
    heap_ptr += total;
    obj->size = (unsigned int)numObjRefs | MSB;
    memset(obj->data, 0, numObjRefs * sizeof(ObjRef));
    return obj;
}


void *newPrimObject(int dataSize) {
    size_t total = sizeof(unsigned int) + dataSize;
    
  
    size_t alignment_padding = (sizeof(void*) - ((uintptr_t)heap_ptr % sizeof(void*))) % sizeof(void*);
    heap_ptr += alignment_padding;

    if (heap_ptr + total > heap + (current_heap_half + 1) * heap_half_size) {
        switch_heap_half();
        
        if (heap_ptr + total > heap + (current_heap_half + 1) * heap_half_size) {
            printf("heap overflow\n");
            exit(99);
          //  print_heap_status();
           // fatalError("Out of memory in heap");
        }
    }

    ObjRef obj = (ObjRef)heap_ptr;
    heap_ptr += total;
    obj->size = dataSize;
    memset(obj->data, 0, dataSize);
    return obj;
}


void fatalError(char *msg) {
    fprintf(stderr, "Fatal error: %s\n", msg);
    exit(1);
}

void *getPrimObjectDataPointer(void *obj) {
    return ((ObjRef)obj)->data;
}

FILE* parse_args(int argc, char* argv[]) {
    int codefile_index = -1;

    if (argc == 1) {
        printf("Error: no code file specified\n");
        exit(1);
    }

    if (strcmp(argv[1], "--help") == 0) {
        printf("usage: %s [option] <code file>\n", argv[0]);
        printf("Options:\n");
        printf("  --stack <n>      set stack size to n KBytes (default: n = 64)\n");
        printf("  --heap <n>       set heap size to n KBytes (default: n = 8192)\n");
        printf("  --gcpurge        purge old objects after collection\n");
        printf("  --debug          start virtual machine in debug mode\n");
        printf("  --version        show version and exit\n");
        printf("  --help           show this help and exit\n");
        exit(0);
    }

    if (strcmp(argv[1], "--version") == 0) {
        printf("Ninja Virtual Machine version %d (compiled %s, %s)\n", VM_VERSION, __DATE__, __TIME__);
        exit(0);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--stack") == 0 && i + 1 < argc) {
            stack_size_kb = atoi(argv[++i]);
            continue;
        } else if (strcmp(argv[i], "--heap") == 0 && i + 1 < argc) {
            heap_size_kb = atoi(argv[++i]);
            continue;
        } else if (strcmp(argv[i], "--debug") == 0) {
            continue;
        } else if (strcmp(argv[i], "--gcpurge") == 0) {
            continue;  
        } else if (strcmp(argv[i], "--version") == 0) {
            continue;
        } else if (strcmp(argv[i], "--help") == 0) {
            continue;
        }

        if (codefile_index != -1) {
            printf("Error: more than one code file specified\n");
            exit(1);
        }
        codefile_index = i;
    }

    if (codefile_index == -1) {
        printf("Error: no code file specified\n");
        exit(1);
    }

    FILE* fp = fopen(argv[codefile_index], "r");
    if (fp == NULL) {
        printf("Error: cannot open code file '%s'\n", argv[codefile_index]);
        exit(99);
    }

    return fp;
}




void load_program(FILE* fp, const char* file_name, unsigned int** instruction, 
    unsigned int** data, unsigned int* instruction_counter,
    unsigned int* data_count)
{
    char format[4];
    unsigned int version;

    if (fread(format, sizeof(char), 4, fp) != 4 || strncmp(format, "NJBF", 4) != 0) {
        printf("Error: invalid file format\n");
        exit(99);
    }

    if (fread(&version, sizeof(unsigned int), 1, fp) != 1 || version != VM_VERSION) {
        printf("Error: file '%s' has wrong version number\n", file_name);     
        exit(99);
    }

    if (fread(instruction_counter, sizeof(unsigned int), 1, fp) != 1) {
        printf("Error: cannot read instruction count\n");
        exit(99);
    }

    if (fread(data_count, sizeof(unsigned int), 1, fp) != 1) {
        printf("Error: cannot read data count\n");
        exit(99);
    }

    *instruction = malloc(*instruction_counter * sizeof(unsigned int));
    if (*instruction == NULL) {
        printf("Error: out of memory (instructions)\n");
        exit(99);
    }

    if (fread(*instruction, sizeof(unsigned int), *instruction_counter, fp) != *instruction_counter) {
        printf("Error: could not read all instructions\n");
        free(*instruction);
        exit(99);
    }

    *data = (unsigned int*)malloc(*data_count * sizeof(unsigned int));
    
    if (*data == NULL) {
        printf("Error: out of memory (data)\n");
        free(*instruction);
        exit(99);
    }

    if (fread(*data, sizeof(unsigned int), *data_count, fp) != *data_count) {
        printf("Error: could not read all data\n");
        free(*instruction);
        free(*data);
        exit(99);
    }
}

void print_heap_status(void) {
    printf("Heap status:\n");
    printf("  Total heap size: %zu bytes (%d KiB)\n", heap_size, heap_size_kb);
    printf("  Half size: %zu bytes\n", heap_half_size);
    printf("  Current half: %d\n", current_heap_half);
    printf("  Used in current half: %zu bytes\n", (size_t)(heap_ptr - (heap + current_heap_half * heap_half_size)));
    printf("  Free in current half: %zu bytes\n", 
           (heap + (current_heap_half + 1) * heap_half_size) - heap_ptr);
    printf("  Total used: %zu bytes\n", 
           (current_heap_half * heap_half_size) + (heap_ptr - (heap + current_heap_half * heap_half_size)));
}

size_t get_heap_half_size(void) {
    return heap_half_size;
}






void execute(unsigned int IR, StackSlot stack[], int* sp, int* halt, ObjRef data[], 
    unsigned int data_count, int* frame_pointer, int* pc, 
    int debug_mode, int is_run_command, ObjRef* return_value) {

        
unsigned char opcode = IR >> 24;
unsigned int immediate_value = IMMEDIATE(IR);
int argument = SIGN_EXTEND(immediate_value);


switch (opcode) {
case HALT:
    *halt = 1;
    break;

case PUSH: {
    if (*sp >= stack_slots) {
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    bigFromInt(argument);  
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = bip.res;
    (*sp)++;
    break;
}

case ADD: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    BigObjRef b = stack[--(*sp)].u.objRef;
    BigObjRef a = stack[--(*sp)].u.objRef;
    
    bip.op1 = a;
    bip.op2 = b;
    bigAdd();
    
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = bip.res;
    (*sp)++;
    break;
}

case SUB: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    BigObjRef b = stack[--(*sp)].u.objRef;
    BigObjRef a = stack[--(*sp)].u.objRef;
    
    bip.op1 = a;
    bip.op2 = b;
    bigSub();
    
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = bip.res;
    (*sp)++;
    break;
}

case MUL: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    BigObjRef b = stack[--(*sp)].u.objRef;
    BigObjRef a = stack[--(*sp)].u.objRef;
    
    bip.op1 = a;
    bip.op2 = b;
    bigMul();
    
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = bip.res;
    (*sp)++;
    break;
}

case DIV: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    BigObjRef b = stack[--(*sp)].u.objRef;
    BigObjRef a = stack[--(*sp)].u.objRef;
    
   
    bip.op1 = b;
    if (bigSgn() == 0) {
        printf("Error: Division by zero\n");
        *halt = 1;
        break;
    }
    
    bip.op1 = a;
    bip.op2 = b;
    bigDiv();  
    
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = bip.res;
    (*sp)++;
    break;
}

case MOD: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    BigObjRef b = stack[--(*sp)].u.objRef;
    BigObjRef a = stack[--(*sp)].u.objRef;
    
    
    bip.op1 = b;
    if (bigSgn() == 0) {
        printf("Error: Modulo by zero\n");
        *halt = 1;
        break;
    }
    
    bip.op1 = a;
    bip.op2 = b;
    bigDiv();  
    
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = bip.rem;
    (*sp)++;
    break;
}

case LT: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    
    StackSlot bSlot = stack[--(*sp)];
    StackSlot aSlot = stack[--(*sp)];
    
    if (aSlot.isObjRef || bSlot.isObjRef) {
        BigObjRef a = aSlot.isObjRef ? aSlot.u.objRef : NULL;
        BigObjRef b = bSlot.isObjRef ? bSlot.u.objRef : NULL;
        
        if (!a && aSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        if (!b && bSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        
        if (!aSlot.isObjRef && bSlot.isObjRef) {
            bigFromInt(aSlot.u.number);
            a = bip.res;
        }
        else if (aSlot.isObjRef && !bSlot.isObjRef) {
            bigFromInt(bSlot.u.number);
            b = bip.res;
        }
        
        bip.op1 = a;
        bip.op2 = b;
        int cmp = bigCmp();
        
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (cmp < 0) ? 1 : 0;
        (*sp)++;
    }
    else {
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (aSlot.u.number < bSlot.u.number) ? 1 : 0;
        (*sp)++;
    }
    break;
}

case LE: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    
    StackSlot bSlot = stack[--(*sp)];
    StackSlot aSlot = stack[--(*sp)];
    
    if (aSlot.isObjRef || bSlot.isObjRef) {
        BigObjRef a = aSlot.isObjRef ? aSlot.u.objRef : NULL;
        BigObjRef b = bSlot.isObjRef ? bSlot.u.objRef : NULL;
        
        if (!a && aSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        if (!b && bSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        
        if (!aSlot.isObjRef && bSlot.isObjRef) {
            bigFromInt(aSlot.u.number);
            a = bip.res;
        }
        else if (aSlot.isObjRef && !bSlot.isObjRef) {
            bigFromInt(bSlot.u.number);
            b = bip.res;
        }
        
        bip.op1 = a;
        bip.op2 = b;
        int cmp = bigCmp();
        
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (cmp <= 0) ? 1 : 0;
        (*sp)++;
    }
    else {
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (aSlot.u.number <= bSlot.u.number) ? 1 : 0;
        (*sp)++;
    }
    break;
}

case GT: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    
    StackSlot bSlot = stack[--(*sp)];
    StackSlot aSlot = stack[--(*sp)];
    
    if (aSlot.isObjRef || bSlot.isObjRef) {
        BigObjRef a = aSlot.isObjRef ? aSlot.u.objRef : NULL;
        BigObjRef b = bSlot.isObjRef ? bSlot.u.objRef : NULL;
        
        if (!a && aSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        if (!b && bSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        
        if (!aSlot.isObjRef && bSlot.isObjRef) {
            bigFromInt(aSlot.u.number);
            a = bip.res;
        }
        else if (aSlot.isObjRef && !bSlot.isObjRef) {
            bigFromInt(bSlot.u.number);
            b = bip.res;
        }
        
        bip.op1 = a;
        bip.op2 = b;
        int cmp = bigCmp();
        
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (cmp > 0) ? 1 : 0;
        (*sp)++;
    }
    else {
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (aSlot.u.number > bSlot.u.number) ? 1 : 0;
        (*sp)++;
    }
    break;
}

case GE: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    
    StackSlot bSlot = stack[--(*sp)];
    StackSlot aSlot = stack[--(*sp)];
    
    if (aSlot.isObjRef || bSlot.isObjRef) {
        BigObjRef a = aSlot.isObjRef ? aSlot.u.objRef : NULL;
        BigObjRef b = bSlot.isObjRef ? bSlot.u.objRef : NULL;
        
        if (!a && aSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        if (!b && bSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        
        if (!aSlot.isObjRef && bSlot.isObjRef) {
            bigFromInt(aSlot.u.number);
            a = bip.res;
        }
        else if (aSlot.isObjRef && !bSlot.isObjRef) {
            bigFromInt(bSlot.u.number);
            b = bip.res;
        }
        
        bip.op1 = a;
        bip.op2 = b;
        int cmp = bigCmp();
        
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (cmp >= 0) ? 1 : 0;
        (*sp)++;
    }
    else {
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (aSlot.u.number >= bSlot.u.number) ? 1 : 0;
        (*sp)++;
    }
    break;
}

case EQ: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    
    StackSlot bSlot = stack[--(*sp)];
    StackSlot aSlot = stack[--(*sp)];
    
    if (aSlot.isObjRef || bSlot.isObjRef) {
        BigObjRef a = aSlot.isObjRef ? aSlot.u.objRef : NULL;
        BigObjRef b = bSlot.isObjRef ? bSlot.u.objRef : NULL;
        
        if (!a && aSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        if (!b && bSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        
        if (!aSlot.isObjRef && bSlot.isObjRef) {
            bigFromInt(aSlot.u.number);
            a = bip.res;
        }
        else if (aSlot.isObjRef && !bSlot.isObjRef) {
            bigFromInt(bSlot.u.number);
            b = bip.res;
        }
        
        bip.op1 = a;
        bip.op2 = b;
        int cmp = bigCmp();
        
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (cmp == 0) ? 1 : 0;
        (*sp)++;
    }
    else {
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (aSlot.u.number == bSlot.u.number) ? 1 : 0;
        (*sp)++;
    }
    break;
}

case NE: {
    if (*sp < 2) {
        printf("Error: Not enough operands\n");
        *halt = 1;
        break;
    }
    
    StackSlot bSlot = stack[--(*sp)];
    StackSlot aSlot = stack[--(*sp)];
    
    if (aSlot.isObjRef || bSlot.isObjRef) {
        BigObjRef a = aSlot.isObjRef ? aSlot.u.objRef : NULL;
        BigObjRef b = bSlot.isObjRef ? bSlot.u.objRef : NULL;
        
        if (!a && aSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        if (!b && bSlot.isObjRef) {
            printf("Error: Null reference in comparison\n");
            *halt = 1;
            break;
        }
        
        if (!aSlot.isObjRef && bSlot.isObjRef) {
            bigFromInt(aSlot.u.number);
            a = bip.res;
        }
        else if (aSlot.isObjRef && !bSlot.isObjRef) {
            bigFromInt(bSlot.u.number);
            b = bip.res;
        }
        
        bip.op1 = a;
        bip.op2 = b;
        int cmp = bigCmp();
        
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (cmp != 0) ? 1 : 0;
        (*sp)++;
    }
    else {
        stack[*sp].isObjRef = 0;
        stack[*sp].u.number = (aSlot.u.number != bSlot.u.number) ? 1 : 0;
        (*sp)++;
    }
    break;
}

case JMP:
    *pc = argument;
    break;

case BRF:
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    (*sp)--;
    if (!stack[*sp].u.number) {  
        *pc = argument;
    }
    break;

case BRT:
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    (*sp)--;
    if (stack[*sp].u.number) {  
        *pc = argument;
    }
    break;

case RDINT: {
    int value;
    if (scanf("%d", &value) != 1) {
        printf("Error: Invalid integer input\n");
        *halt = 1;
        break;
    }
    if (*sp >= stack_slots) {
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    bigFromInt(value);  
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = bip.res;
    (*sp)++;
    break;
}

case WRINT: {
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    (*sp)--;
    if (stack[*sp].isObjRef) {
        bip.op1 = stack[*sp].u.objRef;
        bigPrint(stdout);  
    } else {
        printf("%d", stack[*sp].u.number);
    }
    break;
}

case RDCHR: {
    int ch;
    do {
        ch = getchar();
    } while (ch == '\n');
    
    if (*sp >= stack_slots) {
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    bigFromInt(ch);  
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = bip.res;
    (*sp)++;
    break;
}

case WRCHR:
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    (*sp)--;
    if (stack[*sp].isObjRef) {
        bip.op1 = stack[*sp].u.objRef;
        int charValue = bigToInt();  
        printf("%c", (char)charValue);
    } else {
        printf("%c", (char)stack[*sp].u.number);
    }
    break;

case PUSHG: {
    if (*sp >= stack_slots) {
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    if (argument >= data_count) {
        printf("Error: Invalid data index\n");
        *halt = 1;
        break;
    }
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = data[argument];
    (*sp)++;
    break;
}

case POPG: {
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    if (argument >= data_count) {
        printf("Error: Invalid data index\n");
        *halt = 1;
        break;
    }
    (*sp)--;
    if (stack[*sp].isObjRef) {
        data[argument] = stack[*sp].u.objRef;
    } else {
        printf("Error: Expected object reference\n");
        *halt = 1;
    }
    break;
}

case ASF:
    if (*sp >= stack_slots) {
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    stack[*sp].isObjRef = 0;
    stack[*sp].u.number = *frame_pointer;
    (*sp)++;
    *frame_pointer = *sp;
    *sp += argument;
    break;

case RSF:
    *sp = *frame_pointer;
    (*sp)--;
    *frame_pointer = stack[*sp].u.number;
    break;

case PUSHL:
    if (*frame_pointer + argument >= stack_slots) {
        printf("Error: Invalid stack access\n");
        *halt = 1;
        break;
    }
    if (*sp >= stack_slots){
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    stack[*sp] = stack[*frame_pointer + argument];
    (*sp)++;
    break;

case POPL:
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    if (*frame_pointer + argument >=stack_slots) {
        printf("Error: Invalid stack access\n");
        *halt = 1;
        break;
    }
    (*sp)--;
    stack[*frame_pointer + argument] = stack[*sp];
    break;

case CALL:
    if (*sp + 1 >= stack_slots) {
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    stack[*sp].isObjRef = 0;
    stack[*sp].u.number = *pc;
    (*sp)++;
    *pc = argument;
    break;

case RET:
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    (*sp)--;
    *pc = stack[*sp].u.number;
    break;

case DROP:
    if (*sp < argument) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    *sp -= argument;
    break;

case DUP:
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    if (*sp >= stack_slots) {
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    stack[*sp] = stack[*sp - 1];
    (*sp)++;
    break;

case PUSHR:
    if (*sp >= stack_slots) {
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = *return_value;
    (*sp)++;
    break;

case POPR:
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    (*sp)--;
    if (stack[*sp].isObjRef) {
        *return_value = stack[*sp].u.objRef;
    } else {
        printf("Error: Expected object reference\n");
        *halt = 1;
    }
    break;


case NEW: {
    if (*sp >= stack_slots){
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    int numFields = argument;
    ObjRef obj = newCompoundObject(numFields);
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = obj;
    (*sp)++;
    break;
}

case GETF: {
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    int fieldOffset = argument;
    ObjRef obj = stack[(*sp)-1].u.objRef;
    
    if (!obj) {
        printf("Error: Null reference access\n");
        *halt = 1;
        break;
    }
    if (IS_PRIM(obj) || fieldOffset < 0 || fieldOffset >= GET_SIZE(obj)) {
        printf("Error: Invalid field access\n");
        *halt = 1;
        break;
    }
    
    stack[(*sp)-1].u.objRef = GET_REFS(obj)[fieldOffset];
    break;
}

case PUTF: {
    if (*sp < 2) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    int fieldOffset = argument;
    ObjRef value = stack[(*sp)-1].u.objRef;
    ObjRef obj = stack[(*sp)-2].u.objRef;
    (*sp) -= 2;
    
    if (!obj) {
        printf("Error: Null reference access\n");
        *halt = 1;
        break;
    }
    if (IS_PRIM(obj) || fieldOffset < 0 || fieldOffset >= GET_SIZE(obj)) {
        printf("Error: Invalid field access\n");
        *halt = 1;
        break;
    }
    
    GET_REFS(obj)[fieldOffset] = value;
    break;
}

case NEWA: {
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    
   
    StackSlot sizeSlot = stack[(*sp)-1];
    (*sp)--; 
    
    
    if (sizeSlot.isObjRef) {
        if (sizeSlot.u.objRef && IS_PRIM(sizeSlot.u.objRef)) {
            
            bip.op1 = sizeSlot.u.objRef;
            sizeSlot.u.number = bigToInt();
            sizeSlot.isObjRef = 0;
        } else {
            printf("Error: Array size must be a number\n");
            *halt = 1;
            break;
        }
    }
    
    int numElements = sizeSlot.u.number;
    
    if (numElements < 0) {
        printf("Error: Negative array size\n");
        *halt = 1;
        break;
    }
    
    ObjRef array = newCompoundObject(numElements);
    
    if (*sp >= stack_slots) {
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = array;
    (*sp)++;
    break;
}
case GETFA: {
    if (*sp < 2) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    
    
    StackSlot indexSlot = stack[(*sp)-1];
    ObjRef array = stack[(*sp)-2].u.objRef;
    (*sp) -= 2;
    
    if (!array) {
        printf("Error: Null reference access\n");
        *halt = 1;
        break;
    }
    
    if (IS_PRIM(array)) {
        printf("Error: Not an array 65(primitive object)\n");
        *halt = 1;
        break;
    }
    
   
    int index;
    if (indexSlot.isObjRef) {
        if (indexSlot.u.objRef && IS_PRIM(indexSlot.u.objRef)) {
            
            bip.op1 = indexSlot.u.objRef;
            index = bigToInt();
        } else {
            printf("Error: Array index must be a number\n");
            *halt = 1;
            break;
        }
    } else {
        index = indexSlot.u.number;
    }
    
    int size = GET_SIZE(array);
    
    if (index < 0 || index >= size) {
        printf("Error: Array index %d out of bounds (size=%d)\n", index, size);
        *halt = 1;
        break;
    }
    
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = GET_REFS(array)[index];
    (*sp)++;
    break;
}

case PUTFA: {
    if (*sp < 3) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    
    
    ObjRef value = stack[(*sp)-1].u.objRef;
    StackSlot indexSlot = stack[(*sp)-2];
    ObjRef array = stack[(*sp)-3].u.objRef;
    (*sp) -= 3;
    
    
    if (!array) {
        printf("Error: Null reference access\n");
        *halt = 1;
        break;
    }
    
    
    if (IS_PRIM(array)) {
        printf("Error: Not an array (primitive object)\n");
        *halt = 1;
        break;
    }
    
    
    int index;
    if (indexSlot.isObjRef) {
        if (indexSlot.u.objRef && IS_PRIM(indexSlot.u.objRef)) {
            
            bip.op1 = indexSlot.u.objRef;
            index = bigToInt();
        } else {
            printf("Error: Array index must be a number\n");
            *halt = 1;
            break;
        }
    } else {
        index = indexSlot.u.number;
    }
    
    int size = GET_SIZE(array);
    
    
    if (index < 0 || index >= size) {
        printf("Error: Array index %d out of bounds (size=%d)\n", index, size);
        *halt = 1;
        break;
    }
    

    GET_REFS(array)[index] = value;
    break;
}

case GETSZ: {
    if (*sp < 1) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    ObjRef obj = stack[(*sp)-1].u.objRef;
    
    if (!obj) {
        printf("Error: Null reference access\n");
        *halt = 1;
        break;
    }
    
    stack[(*sp)-1].isObjRef = 0;
    stack[(*sp)-1].u.number = IS_PRIM(obj) ? -1 : GET_SIZE(obj);
    break;
}

case PUSHN: {
   if (*sp >= stack_slots){
        printf("Error: Stack overflow\n");
        *halt = 1;
        break;
    }
    stack[*sp].isObjRef = 1;
    stack[*sp].u.objRef = NULL;  
    (*sp)++;
    break;
}

case REFEQ: {
    if (*sp < 2) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    ObjRef ref2 = stack[(*sp)-1].u.objRef;
    ObjRef ref1 = stack[(*sp)-2].u.objRef;
    (*sp) -= 2;
    
    stack[*sp].isObjRef = 0;
    stack[*sp].u.number = (ref1 == ref2) ? 1 : 0;
    (*sp)++;
    break;
}

case REFNE: {
    if (*sp < 2) {
        printf("Error: Stack underflow\n");
        *halt = 1;
        break;
    }
    ObjRef ref2 = stack[(*sp)-1].u.objRef;
    ObjRef ref1 = stack[(*sp)-2].u.objRef;
    (*sp) -= 2;
    
    stack[*sp].isObjRef = 0;
    stack[*sp].u.number = (ref1 != ref2) ? 1 : 0;
    (*sp)++;
    break;
}

default:
    printf("Unknown opcode %02x at address %d\n", opcode, *pc - 1);
    *halt = 1;
}
}

void print_instruction(int index, unsigned int instr) {
    unsigned char opcode = instr >> 24;
    unsigned int immediate = IMMEDIATE(instr);
    int argument = SIGN_EXTEND(immediate);


    
    
        printf("%04d:\t", index);  
    
    switch (opcode) {
        case HALT:   printf("halt\n"); break;
        case PUSH:   printf("pushc\t%d\n", argument); break;
        case RDINT:  printf("rdint\n"); break;
        case WRINT:  printf("wrint\n"); break;
        case RDCHR:  printf("rdchr\n"); break;
        case WRCHR:  printf("wrchr\n"); break;
        case ADD:    printf("add\n"); break;
        case SUB:    printf("sub\n"); break;
        case MUL:    printf("mul\n"); break;
        case DIV:    printf("div\n"); break;
        case MOD:    printf("mod\n"); break;
        case PUSHG: printf("pushg\t%d\n", argument); break;
        case POPG: printf("popg\t%d\n", argument); break;
        case ASF: printf("asf\t%d\n", argument); break;
        case RSF: printf("rsf\n"); break;
        case PUSHL: printf("pushl\t%d\n", argument); break;
        case POPL: printf("popl\t%d\n", argument); break;
        case EQ: printf("eq\n"); break;
        case NE: printf("ne\n"); break;
        case LT: printf("lt\n"); break;
        case LE: printf("le\n"); break;
        case GT: printf("gt\n"); break;
        case GE: printf("ge\n"); break;
        case JMP: printf("jmp\t%d\n", argument);break;
        case BRF: printf("brf\t%d\n", argument);break;
        case BRT: printf("brt\t%d\n", argument);break;
        case CALL: printf("call\t%d\n", argument); break;
        case RET: printf("ret\n"); break;
        case DROP: printf("drop\t%d\n", argument); break;
        case PUSHR: printf("pushr\n"); break;
        case POPR: printf("popr\n"); break;
        case DUP: printf("dup\n"); break;
        case NEW:    printf("new\t%d\n", argument); break;
        case GETF:   printf("getf\t%d\n", argument); break;
        case PUTF:   printf("putf\t%d\n", argument); break;
        case NEWA:   printf("newa\n"); break;
        case GETFA:  printf("getfa\n"); break;
        case PUTFA:  printf("putfa\n"); break;
        case GETSZ:  printf("getsz\n"); break;
        case PUSHN:  printf("pushn\n"); break;
        case REFEQ:  printf("refeq\n"); break;
        case REFNE:  printf("refne\n"); break;
        default:     printf("unknown (%02x)\n", opcode); break;
    }
}