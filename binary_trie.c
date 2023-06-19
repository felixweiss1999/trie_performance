#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "table.c"
#include "util.c"

int main(){
    int tablelength1;
    uint64_t start, end;
    start = rdtsc();
    struct TABLEENTRY* table = set_table("ipv4/build.txt", &tablelength1);
    end = rdtsc();
    printf("Elapsed clock cycles for building the table: %llu\n", end-start);
}