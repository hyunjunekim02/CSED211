/*
CSED211 Cache Lab
Part A. Cache Simulator
--------------------
Name: Hyun June Kim
loginID: hyunjunekim
studentID: 20210643
--------------------
*/

#include "cachelab.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>
#include <limits.h>

//function that uses when we access to the Cache memory
void accessCache(unsigned long address);

//Flags, h:help, v: verbose
bool h_flag, v_flag;

//variables which counts the number of hit, miss, and eviction
int num_hit, num_miss, num_evi;

//variables which store Cache memory's properties
int s, S, b, E;

//variable use when check LRU policy
int LRU;

//Cache memory structure
typedef struct {
    bool valid;
    unsigned int tag;
    int LRU_num;
}cache_block;

//File pointer and two dimensional cache block array
FILE* fp = NULL;
cache_block** cache = NULL;

int main(int argc, char* argv[])
{
    //variable initializing
    num_hit = 0;
    num_miss = 0;
    num_evi = 0;
    LRU = 0;
    v_flag = false;
    h_flag = false;

    //variables about address, size, etc
    unsigned long address;
    char cmd;
    int size;
    int opt;

    //Parsing
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
        case 'h':
            h_flag = true;
            break;
        case 'v':
            v_flag = true;
            break;
        case 's':
            s = atoi(optarg);
            S = 1 << s;
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            fp = fopen(optarg, "r");
            break;
        }
    }

    //error
    if (fp == NULL) {
        fprintf(stderr, "File Error!");
        exit(-1);
    }

    //when help mode
    if (h_flag) {
        printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
        printf("Options:\n");
        printf("-h: Optional help flag that prints usage info\n");
        printf("-v: Optional verbose flag that displays trace info\n");
        printf("-s <s>: Number of set index bits (S=2^s is the number of sets\n");
        printf("-E <E>: Associativity (number of lines per set)\n");
        printf("-b <b>: Number of block bits (B = 2^b is the block size)\n");
        printf("-t <tracefile>: Name of the valgrind trace to replay\n");
        exit(0);
    }

    //two dimensional array allocating
    cache = (cache_block**)malloc(sizeof(cache_block*) * S);
    for (int i = 0; i < S; i++) {
        *(cache + i) = (cache_block*)malloc(sizeof(cache_block) * E);
    }
    //initializing
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < E; j++) {
            cache[i][j].valid = false;
            cache[i][j].tag = -1;
            cache[i][j].LRU_num = -1;
        }
    }

    //file scan
    while (fscanf(fp, "%c %lx, %d", &cmd, &address, &size) != EOF) {
        switch (cmd) {
        //first one is I
        case 'I':
            continue;
        //data load
        case 'L':
            if (v_flag) {
                printf("%c %lx, %d", cmd, address, size);
            }
            accessCache(address);
            printf("\n");
            break;
        //data modivication
        case 'M':
            if (v_flag) {
                printf("%c %lx, %d", cmd, address, size);
            }
            accessCache(address);
            accessCache(address);
            printf("\n");
            break;
        //data storing
        case 'S':
            if (v_flag) {
                printf("%c %lx, %d", cmd, address, size);
            }
            accessCache(address);
            printf("\n");
            break;
        default:
            break;
        }
    }
    //closing file
    fclose(fp);

    //freeing memories
    for (int i = 0; i < E; i++) {
        free(cache[i]);
    }
    free(cache);

    //prints result
    printSummary(num_hit, num_miss, num_evi);
    return 0;
}

//function that called when accessing to Cache memory
void accessCache(unsigned long address) {
    //get an address by parameter, computes index and tag bits
    unsigned index = (address >> b) & (UINT_MAX >> (64 - s));
    unsigned tag = address >> (s + b);
    int min = INT_MAX;
    int evict_line = 0;
    int temp;

    //when the cache hit
    for (int i = 0; i < E; i++) {
        if (cache[index][i].valid) {
            if (cache[index][i].tag != tag) {
                continue;
            }
            cache[index][i].LRU_num = LRU;
            LRU++;
            num_hit++;
            if (v_flag) {
                printf(" hit");
            }
            return;
        }
    }

    //when the cache miss
    num_miss++;
    if (v_flag) {
        printf(" miss");
    }

    //finding that it is a just miss or is an eviction
    for (int i = 0; i < E; i++) {
        temp = cache[index][i].LRU_num;
        if (temp < min) {
            min = temp;
            evict_line = i;
            if(min==-1){
                break;
            }
        }
    }

    //eviction case
    if (cache[index][evict_line].valid) {
        num_evi++;
        if (v_flag) {
            printf(" eviction");
        }
    }

    //updating cache memory information
    cache[index][evict_line].tag = tag;
    cache[index][evict_line].LRU_num = LRU;
    LRU++;
    cache[index][evict_line].valid = true;

    return;
}