/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

extern int mem_access(int addr, int write_flag, int write_data);
extern int get_num_mem_accesses();

enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty[MAX_BLOCK_SIZE];
    int lru[MAX_BLOCK_SIZE];
    int set;
    int tag[MAX_BLOCK_SIZE];

    int valid[MAX_BLOCK_SIZE];
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;

    int totNumOfWords;
    int blockOffset;
    int setIndex;
    int tagSize;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache();

// Own Variables and functions
// int num_mainMemWordsAccessed = 0;
int num_hits = 0;
int num_misses = 0;
int num_writebacks = 0;
int num_dirtyLeft = 0;

// int totNumWords = 0;
// int blockOffset = 0;
// int setIndex = 0;
// int tagSize = 0;

void setCache();

int getTag(unsigned int addr);
int getSet(unsigned int addr);
int getBlock(unsigned int addr);

int getAddr(int tag, int set, int block);

// int checkInvalid(int set, int block);
// int checkFull(int set);

// 1) WRITE POLICY: write-back, allocate-on-write
// 2) ASSOCIATIVITY: varies according to the blocksPerSet parameter. Associativity ranges from 1 (direct-mapped) to the number 
//    of blocks in the cache (fully associative).
// 3) SIZE: the total number of words in the cache is blockSizeInWords * numberOfSets * blocksPerSet
// 4) BLOCK SIZE: varies according to the blockSizeInWords parameter. All transfers between the cache and memory take place in 
//    units of a single block.
// 5) PLACEMENT/REPLACEMENT POLICY: when looking for a block within a set to replace, the best block to replace is an invalid 
//    (empty) block. If there is none of these, replace the least-recently used block.
// Make sure the units of each parameter are as specified. Note that the smallest data granularity in this project is a word, 
// because this is the data granularity of the LC-2K architecture. blockSizeInWords, numberOfSets, and blocksPerSet should all 
// be powers of two. To simplify your program, you may assume that the maximum number of cache blocks is 256 and the maximum block 
// size is 256 (these small numbers allow you to use a 2-dimensional array for the cache data structure).

/*
 * Set up the cache with given command line parameters. This is 
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet){
    cache.blockSize = blockSize;
    cache.numSets = numSets;
    cache.blocksPerSet = blocksPerSet;
    cache.totNumOfWords = blockSize * numSets * blocksPerSet;
    cache.blockOffset = log2(blockSize);
    cache.setIndex = log2(numSets);
    cache.tagSize = 32 - cache.setIndex - cache.blockOffset;
    // printf("b_s: %d, n_s: %d\n", blockSize, numSets);
    // printf("tag: %d, set: %d, block: %d\n", cache.tagSize, cache.setIndex, cache.blockOffset);
    setCache();

    printf("Simulating a cache with %d total lines; each line has %d words\n", numSets*blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n", blocksPerSet, numSets);

    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int cache_access(int addr, int write_flag, int write_data) {

    // printf("beginning\n");

    // Get tag, block, and set
    int addrU = (unsigned int)(addr);

    int tag = getTag(addrU);
    int block = getBlock(addrU); 
    int set = getSet(addrU);

    // printf("addr: %d, tag: %d, set: %d, block: %d\n", addrU, tag, set, block);

    // Check for any invlid in set
    int invalid = -1;
    int i;
    for (i = 0; i < cache.blockSize*cache.blocksPerSet; i+=cache.blockSize) {
        if (cache.blocks[set].valid[i] == 0) {
            invalid = i;
            break;
        }
    }

    // Check if already in cache
    int tagFound = -1;
    for (i = 0; i < cache.blockSize*cache.blocksPerSet; i+=cache.blockSize) {
        if (cache.blocks[set].tag[i] == tag) {
            tagFound = i;
            invalid = -1;
            // printf("here?\n");
            break;
        }
    }

    if (invalid != -1) { // if invalid
        int addrTemp = addr;
        int blockTemp = block;

        // Bring block to greatest multiple of blockSize just before current block
        while ((blockTemp % cache.blockSize) != 0) {
            --addrTemp;
            --blockTemp;
        }

        // Fill cache block with data from memory
        int i;
        for (i = 0; i < cache.blockSize; ++i) {
            cache.blocks[set].tag[i+invalid] = tag;
            cache.blocks[set].data[i+invalid] = mem_access(addrTemp+i, 0, 0);
            cache.blocks[set].valid[i+invalid] = 1;
            cache.blocks[set].lru[i+invalid] = 0;
        }

        if (write_flag == 1) {
            // Write new data and set to dirty
            cache.blocks[set].data[invalid+block] = write_data;
            for (i = 0; i < cache.blockSize; ++i) {
                cache.blocks[set].dirty[invalid+i] = 1;
            }
        } 

        // Increment lru
        for (i = 0; i < cache.blockSize*cache.blocksPerSet; ++i) {
            if (cache.blocks[set].lru[i] >= 0) {
                ++cache.blocks[set].lru[i];
            }
        }

        ++num_misses;
        printAction(addrTemp, cache.blockSize, memoryToCache);

        if (write_flag == 1) {
            printAction(addr, 1, processorToCache);
        } else {
            printAction(addr, 1, cacheToProcessor);
        }

        // printCache();

        return cache.blocks[set].data[block+invalid];

    } else { // if valid

        int addrTemp = addr;
        int blockTemp = block;

        // Bring block to greatest multiple of blockSize just before current block
        while ((blockTemp % cache.blockSize) != 0) {
            --blockTemp;
            --addrTemp;
        }

        if (tagFound == -1) { // if tag is not found
            // Find least recently used block in set
            int max = 0;
            int maxInd = 0;
            int i;
            for (i = 0; i < cache.blockSize*cache.blocksPerSet; i+=cache.blockSize) {
                if (cache.blocks[set].lru[i] > max) {
                    max = cache.blocks[set].lru[i];
                    maxInd = i;
                }
            }

            int dirty = 0;

            // Check dirty bits
            for (i = 0; i < cache.blockSize; ++i) {
                if (cache.blocks[set].dirty[i+maxInd] == 1) {
                    dirty = 1;
                    break;
                }
            }

            if (dirty == 1) {
                for (i = 0; i < cache.blockSize; ++i) {
                    if (cache.blocks[set].dirty[maxInd+i] == 1) {
                        mem_access(getAddr(cache.blocks[set].tag[maxInd+i], set, i), 1, cache.blocks[set].data[maxInd+i]);
                    }
                }

                printAction(getAddr(cache.blocks[set].tag[maxInd], set, 0), cache.blockSize, cacheToMemory);
                ++num_writebacks;

            } else {
                // printf("addr: %d, tag: %d, set: %d\n", getAddr(cache.blocks[set].tag[maxInd], set, block), cache.blocks[set].tag[maxInd], set);
                printAction(getAddr(cache.blocks[set].tag[maxInd], set, 0), cache.blockSize, cacheToNowhere);
            }

            // Putting new data in cache
            for (i = 0; i < cache.blockSize; ++i) {
                cache.blocks[set].tag[i+maxInd] = tag;
                cache.blocks[set].data[i+maxInd] = mem_access(addrTemp+i, 0, 0);
                cache.blocks[set].lru[i+maxInd] = 0;
                cache.blocks[set].dirty[i+maxInd] = 0;
            }

            if (write_flag == 1) {
                // Write new data and set to dirty
                cache.blocks[set].data[maxInd+block] = write_data;
                for (i = 0; i < cache.blockSize; ++i) {
                    cache.blocks[set].dirty[maxInd+i] = 1;
                }
            }

            // updating lru
            for (i = 0; i < cache.blockSize*cache.blocksPerSet; ++i) {
                if (cache.blocks[set].lru[i] >= 0) {
                    ++cache.blocks[set].lru[i];
                }
            }

            ++num_misses;
            printAction(addrTemp, cache.blockSize, memoryToCache);
            if (write_flag == 1) {
                // printf("what the heck\n");
                printAction(addr, 1, processorToCache);
            } else {
                printAction(addr, 1, cacheToProcessor);
            }

            // printCache();

            return cache.blocks[set].data[block+maxInd];

        } else { // if tag is found

            if (write_flag == 1) {
                // Write new data and set to dirty
                cache.blocks[set].data[tagFound+block] = write_data;
                for (i = 0; i < cache.blockSize; ++i) {
                    cache.blocks[set].dirty[tagFound+i] = 1;
                }
            }

            // Reset lru
            for (i = 0; i < cache.blockSize; ++i) {
                cache.blocks[set].lru[tagFound+i] = 0;
            }

            // Increment lru
            for (i = 0; i < cache.blockSize*cache.blocksPerSet; ++i) {
                if (cache.blocks[set].lru[i] >= 0) {
                    ++cache.blocks[set].lru[i];
                }
            }

            ++num_hits;
            if (write_flag == 1) {
                printAction(addr, 1, processorToCache);
            } else {
                // printf("addr: %d\n", addr);
                printAction(addr, 1, cacheToProcessor);
            }

            // printCache();

            return cache.blocks[set].data[block+tagFound];

        }
    }
}


/*
 * print end of run statistics like in the spec. This is not required,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(){
    printf("End of run statistics: \n");
    printf("hits %d, misses %d, writebacks %d\n", num_hits, num_misses, num_writebacks);

    int i, j;
    for (i = 0; i < MAX_CACHE_SIZE; ++i) {
        for (j = 0; j < MAX_BLOCK_SIZE; ++j) {
            if (cache.blocks[i].dirty[j] == 1) {
                ++num_dirtyLeft;
            }
        }
    }

    printf("%d dirty cache blocks left\n", num_dirtyLeft);
    return;
}

/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache()
{
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            printf("\t\t[ %i ]: {", block);
            for (int index = 0; index < cache.blockSize; ++index) {
                printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}

// Own functions
void setCache() {
    int i;
    for (i = 0; i < MAX_CACHE_SIZE; ++i) {
        int j;
        for (j = 0; j < MAX_BLOCK_SIZE; ++j) {
            cache.blocks[i].dirty[j] = 0;
            cache.blocks[i].valid[j] = 0;
            cache.blocks[i].lru[j] = -1;
            cache.blocks[i].tag[j] = -1;
        }
    }
}

int getTag(unsigned int addr) {
    return addr >> (cache.setIndex + cache.blockOffset);
}

int getSet(unsigned int addr) {
    if (cache.setIndex == 0) {
        return 0;
    }
    return (addr << cache.tagSize) >> (cache.tagSize + cache.blockOffset);
}

int getBlock(unsigned int addr) {
    if (cache.blockOffset == 0) {
        return 0;
    }
    return (addr << (cache.tagSize + cache.setIndex)) >> (cache.tagSize + cache.setIndex);
}

int getAddr(int tag, int set, int block) {
    // printf("getAddr - tag: %d, set: %d, block: %d\n", tag, set , block);
    int temp = 0;
    temp += (tag << (cache.setIndex + cache.blockOffset));
    // printf("temp: %d\n", temp);
    temp += (set << (cache.blockOffset));
    // printf("temp: %d\n", temp);
    temp += block;
    // printf("temp: %d\n", temp);
    return temp;
} 

// int checkInvalid(int set, int block) {
//     int i;
//     for (i = 0; i < (cache.blockSize * cache.blocksPerSet); i += cache.blockSize) {
//         if (cache.blocks[set].valid[i] == 0) {
//             block = i;
//             cache.blocks[i].valid[i] = 1;
//             return 1;
//         }
//     }
//     return 0;
// }

// int checkFull(int set) {
//     int i;
//     for (i = 0; i < (cache.blockSize * cache.blocksPerSet); i += cache.blockSize) {
//         if (cache.blocks[set].valid[i] != 0) {
//             // right back current stuff to memory
//             // put new stuff in
//         }
//     }
// }