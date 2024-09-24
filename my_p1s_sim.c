/*
 * Project 1
 * EECS 370 LC-2K Instruction-level simulator
 *
 * Make sure *not* to modify printState or any of the associated functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Machine Definitions
#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */

// File
#define MAXLINELENGTH 1000 /* MAXLINELENGTH is the max number of characters we read */

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

// Function protoyptes
void printState(stateType *);

int convertNum(int);

// Own function prototypes
int getReg(int, char);

int getOffset(int);

// P4 Stuff
extern void cache_init(int blockSize, int numSets, int blocksPerSet);
extern int cache_access(int addr, int write_flag, int write_data);
extern void printStats();
static stateType state;
static int num_mem_accesses = 0;
int mem_access(int addr, int write_flag, int write_data) {
    ++num_mem_accesses;
    if (write_flag) {
        state.mem[addr] = write_data;
    }
    return state.mem[addr];
}
int get_num_mem_accesses(){
	return num_mem_accesses;
}

// main
int main(int argc, char *argv[])
{
    char line[MAXLINELENGTH];
    FILE *filePtr;

    if (argc != 5) {
        printf("error: usage: %s <machine-code file> <blockSizeInWords> <numberOfSets> <blocksPerSet>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    int blockSize;
    sscanf(argv[2], "%d", &blockSize);
    int numSets;
    sscanf(argv[3], "%d", &numSets);
    int blocksSet;
    sscanf(argv[4], "%d", &blocksSet);

    // Initialize cache
    cache_init(blockSize, numSets, blocksSet);

    /* read the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL; state.numMemory++) {
        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        // printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }

    //Your code starts here

    // Max sw
    int sw_max = 0;

    // Counter
    int counter = 0;

    // Initialize program counter and registers to 0
    state.pc = 0;
    int i;
    for (i = 0; i < 8; ++i) {
        state.reg[i] = 0;
    }

    // Read opcodes and do operations
    while ((state.mem[state.pc] >> 22) != 6) { // while not "halt"

        // Print state before operation
        // printState(&state);

        // Initialize state variable
        // int st = state.mem[state.pc]; 
        // printf("bruh?\n");
        int st = cache_access(state.pc, 0, 0);

        if ((st >> 22) == 0) { // add

            // Get regA, regB, and dest
            int regA = getReg(st, 'A');
            int regB = getReg(st, 'B');
            int dest = getReg(st, 'D');

            // Do add operation
            state.reg[dest] = state.reg[regA] + state.reg[regB];

            // Increment program counter
            ++state.pc;

        } else if ((st >> 22) == 1) { // nor

            // Get regA, regB, and dest
            int regA = getReg(st, 'A');
            int regB = getReg(st, 'B');
            int dest = getReg(st, 'D');

            // Do or operation
            state.reg[dest] = ~(state.reg[regA] | state.reg[regB]);

            // Increment program counter
            ++state.pc;

        } else if ((st >> 22) == 2) { // lw

            // Get regA, regB, and offset from st
            int regA = getReg(st, 'A');
            int regB = getReg(st, 'B');
            int offset16 = getOffset(st);

            // Do load operation
            // state.reg[regB] = state.mem[state.reg[regA] + offset16]; 
            // printf("loc lw: %d\n", state.reg[regA] + offset16);
            state.reg[regB] = cache_access(state.reg[regA] + offset16, 0, 0);

            // Increment program counter
            ++state.pc;

        } else if ((st >> 22) == 3) { // sw

            // Get regA, regB, and offset from st
            int regA = getReg(st, 'A');
            int regB = getReg(st, 'B');
            int offset16 = getOffset(st);

            // Do store operation
            int loc = state.reg[regA] + offset16;
            // state.mem[loc] = state.reg[regB];
            // printf("loc sw: %d\n", loc);
            cache_access(loc, 1, state.reg[regB]);

            // Increment program counter
            ++state.pc;

            if (offset16 > sw_max) {
                sw_max = offset16;
            }

        } else if ((st >> 22) == 4) { // beq

            // Get regA, regB, and offset from st
            int regA = getReg(st, 'A');
            int regB = getReg(st, 'B');
            int offset16 = getOffset(st);

            // Do beq operation and update program counter
            if (state.reg[regA] == state.reg[regB]) {
                state.pc = state.pc + 1 + offset16; 
            } else {
                ++state.pc;
            }

        } else if ((st >> 22) == 5) { // jalr

            // Get regA and regB from st
            int regA = getReg(st, 'A');
            int regB = getReg(st, 'B');

            // Do jalr operation and update program counter
            state.reg[regB] = state.pc + 1;
            state.pc = state.reg[regA];

        } else if ((st >> 22) == 7) { // noop

            // noop does nothing
            
            // Increment program counter
            ++state.pc;
        }

        // Increment counter
        ++counter;
    }

    // Halt goes in cache too (for some reason)
    cache_access(state.pc, 0, 0);

    // Print state after operation completes
    // printState(&state);

    // Halt attributes to program counter and counter
    ++state.pc;
    ++counter;    

    // Set new numMemory
    state.numMemory = sw_max + 1;

    // Print out ending stuff
    printf("machine halted\n");
    printf("total of %d instructions executed\n", counter);
    printf("final state of machine:\n");
    printState(&state);

    // Print num_memory_accessed
    printf("$$$ Main memory words accessed: %d\n", num_mem_accesses); 

    // Prints stats
    printStats();

    return(0);
}

// Function definitions
void printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
              printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
              printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

int convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Linux integer */
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return(num);
}

// Own function definitions
int getReg(int st, char c) {
    if (c == 'A') {
        return((st & 0x00380000) >> 19);
    } else if (c == 'B') {
        return((st & 0x00070000) >> 16);
    } else {
        return(st & 0x00000007);
    }
}

int getOffset(int st) {
    int offset = st & 0x0000FFFF;
    if (offset > 32767) { // if 16-bit is neg, pad 1's to make 32-bit neg
        offset |= 0xFFFF0000;
    }
    return(offset);
}