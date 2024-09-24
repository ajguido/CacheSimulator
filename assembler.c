/*
 * Assembler code fragment for LC-2K 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define MAXLINELENGTH 1000
#define MAXNUMWORDS 65536

// Given function prototypes
int readAndParse(FILE *, char *, char *, char *, char *, char *);
int isNumber(char *);

// Main
int
main(int argc, char *argv[])
{
    char *inFileString, *outFileString;
    FILE *inFilePtr, *outFilePtr;
    char label[MAXLINELENGTH], opcode[MAXLINELENGTH], arg0[MAXLINELENGTH],
            arg1[MAXLINELENGTH], arg2[MAXLINELENGTH];

    if (argc != 3) {
        printf("error: usage: %s <assembly-code-file> <machine-code-file>\n",
            argv[0]);
        exit(1);
    }

    // Read from and Write to files
    inFileString = argv[1];
    outFileString = argv[2];

    inFilePtr = fopen(inFileString, "r");
    if (inFilePtr == NULL) {
        printf("error in opening %s\n", inFileString);
        exit(1);
    }
    outFilePtr = fopen(outFileString, "w");
    if (outFilePtr == NULL) {
        printf("error in opening %s\n", outFileString);
        exit(1);
    }

    // Start of my code

    int numLines = 0; // subtract 1 when indexing
    char labelArr[MAXNUMWORDS][7]; // Array of strings used for label tracking - arr[MAX_NUM_OF_WORDS][MAX_LENGTH_OF_LABEL]
    short currLine = 0; // Meant for offset calculation 

    // First pass
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        // fills array with labels, index is line positioning
        if (*label) {
            strcpy(labelArr[numLines], label);
        }

        // Check duplicate label
        int i;
        for (i = 0; i < numLines; ++i) {
            if (strcmp(label, "") && strcmp(labelArr[i], "") && !strcmp(label, labelArr[i])) {
                exit(1);
            }
        }

        ++numLines;
    }
    
    rewind(inFilePtr); // Go back to beginning

    // Second pass
    while (readAndParse(inFilePtr, label, opcode, arg0, arg1, arg2)) {
        uint32_t text = 0;

        // Get regA/srcA
        int regA;
        if (isNumber(arg0)) {
            sscanf(arg0, "%d", &regA);
            regA <<= 19;
        }

        // Get regB/srcB
        int regB;
        if (isNumber(arg1)) {
            sscanf(arg1, "%d", &regB);
            regB <<= 16;
        }
        

        // Do things with specific opcodes
        if (!strcmp(opcode, "add")) { // add
            // Set opcode
            // Opcode already 000

            // Set regA, regB
            text += regA;
            text += regB;

            // Get and set destReg
            int destReg; 
            if (isNumber(arg2)) {
                sscanf(arg2, "%d", &destReg);
            } else {
                int i;
                bool isFound = false;
                for (i = 0; i < numLines; ++i) {
                    if (!strcmp(arg2, labelArr[i])) {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound) { // No label found
                    exit(1);
                }
                destReg = i;
            } 
            text += destReg;

        } else if (!strcmp(opcode, "nor")) { // nor
            // Set opcode
            text = 4194304;

            // Set regA, regB
            text += regA;
            text += regB;
            
            // Get and set destReg
            int destReg; 
            if (isNumber(arg2)) {
                sscanf(arg2, "%d", &destReg);
            } else {
                int i;
                bool isFound = false;
                for (i = 0; i < numLines; ++i) {
                    if (!strcmp(arg2, labelArr[i])) {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound) { // No label found
                    exit(1);
                }
                destReg = i;
            } 
            text += destReg;

        } else if (!strcmp(opcode, "lw")) { // lw
            // Set opcode
            text = 8388608;
            
            // Set regA, regB
            text += regA;
            text += regB;

            // Get offset
            int val = 0;
            short offset16; 
            if (isNumber(arg2)) {
                sscanf(arg2, "%d", &val);
            } else {
                short i;
                bool isFound = false;
                for (i = 0; i < numLines; ++i) {
                    if (!strcmp(arg2, labelArr[i])) {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound) { // No label found
                    exit(1);
                }
                val = i;
            } 

            // Check offset fit in 16-bits
            if (val > 32767 || val < -32768) {
                exit(1);
            }

            offset16 = val;
            
            // Set offset     
            int offset32 = 0;
            offset32 |= offset16;
            offset32 &= 0x0000FFFF;

            text += offset32;

        } else if (!strcmp(opcode, "sw")) { // sw
            // Set opcode
            text = 12582912;

            // Set regA, regB
            text += regA;
            text += regB;

            // Get offset
            int val = 0;
            short offset16;
            if (isNumber(arg2)) {
                sscanf(arg2, "%d", &val);
            } else {
                short i;
                bool isFound = false;
                for (i = 0; i < numLines; ++i) {
                    if (!strcmp(arg2, labelArr[i])) {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound) { // No label found
                    exit(1);
                }
                val = i;
            } 

            // Check offset fit in 16-bits
            if (val > 32767 || val < -32768) {
                exit(1);
            }

            offset16 = val;

            // Set offset     
            int offset32 = 0;
            offset32 |= offset16;
            offset32 &= 0x0000FFFF;

            text += offset32;

        } else if (!strcmp(opcode, "beq")) { // beq
            // Set opcode
            text = 16777216;

            // Set regA, regB
            text += regA;
            text += regB;

            // Get offset
            int val = 0;
            short offset16 = 0;
            if (isNumber(arg2)) {
                sscanf(arg2, "%d", &val);
            } else {
                short i;
                bool isFound = false;
                for (i = 0; i < numLines; ++i) {
                    if (!strcmp(arg2, labelArr[i])) {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound) { // No label found
                    exit(1);
                }
                val = i - currLine - 1;
            } 

            // Check offset fit in 16-bits
            if (val > 32767 || val < -32768) {
                exit(1);
            }

            offset16 = val;

            // Set offset     
            int offset32 = 0;
            offset32 |= offset16;
            offset32 &= 0x0000FFFF;

            text += offset32;

        } else if (!strcmp(opcode, "jalr")) { // jalr
            // Set opcode
            text = 20971520;

            // Set regA, regB
            text += regA;
            text += regB;

            // Do nothing else

        } else if (!strcmp(opcode, "halt")) { // halt
            // Set opcode
            text = 25165824;

            // Do nothing else

        } else if (!strcmp(opcode, "noop")) { // noop
            // Set opcode
            text = 29360128;

            // Do nothing else

        } else if (!strcmp(opcode, ".fill")) { // .fill
            int val = 0;
            if (isNumber(arg0)) {
                val = atoi(arg0);
            } else {
                int i;
                bool isFound = false;
                for (i = 0; i < numLines; ++i) {
                    if (!strcmp(arg0, labelArr[i])) {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound) { // No label found
                    exit(1);
                }
                val = i;
            }
            text += val;

        } else { 
            // Error
            exit(1);
        }

        ++currLine;

        // Print to outFile
        fprintf(outFilePtr, "%d\n", text);
    }

    // Close all open files
    fclose(inFilePtr);
    fclose(outFilePtr);

    exit(0);
}

// Given function implementations

/*
 * Read and parse a line of the assembly-language file.  Fields are returned
 * in label, opcode, arg0, arg1, arg2 (these strings must have memory already
 * allocated to them).
 *
 * Return values:
 *     0 if reached end of file
 *     1 if all went well
 *
 * exit(1) if line is too long.
 */
int
readAndParse(FILE *inFilePtr, char *label, char *opcode, char *arg0,
    char *arg1, char *arg2)
{
    char line[MAXLINELENGTH];
    char *ptr = line;

    /* delete prior values */
    label[0] = opcode[0] = arg0[0] = arg1[0] = arg2[0] = '\0';

    /* read the line from the assembly-language file */
    if (fgets(line, MAXLINELENGTH, inFilePtr) == NULL) {
	/* reached end of file */
        return(0);
    }

    /* check for line too long */
    if (strlen(line) == MAXLINELENGTH-1) {
	printf("error: line too long\n");
	exit(1);
    }

    /* is there a label? */
    ptr = line;
    if (sscanf(ptr, "%[^\t\n ]", label)) {
	/* successfully read label; advance pointer over the label */
        ptr += strlen(label);
    }

    /*
     * Parse the rest of the line.  Would be nice to have real regular
     * expressions, but scanf will suffice.
     */
    sscanf(ptr, "%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]%*[\t\n\r ]%[^\t\n\r ]",
        opcode, arg0, arg1, arg2);
    return(1);
}

int
isNumber(char *string)
{
    /* return 1 if string is a number */
    int i;
    return( (sscanf(string, "%d", &i)) == 1);
}

