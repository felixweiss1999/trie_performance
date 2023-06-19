#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "table.c"
#include "util.c"


typedef struct binNode{
    int hop;
    Node* left;
    Node* right;
} Node;

int main(){
    int tablelength1;
    uint64_t start, end;
    start = rdtsc();
    struct TABLEENTRY* table = set_table("ipv4_rrc_all_90build.txt", &tablelength1);
    end = rdtsc();
    printf("Build Table: %llu\n", end-start);


}