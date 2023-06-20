#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "table.c"
#include "util.c"


typedef struct fourbitNode{
    int hop;
    struct fourbitNode* children[16];
} Node;

int numberOfNodes = 0;

Node* setupNode(){
    Node* node = (Node*) malloc(sizeof(struct fourbitNode));
    node->hop = 0;
    for(int i = 0; i<16; i++)
        node->children[i] = NULL;
    numberOfNodes++;
    return node;
}

void insert(Node* root, struct TABLEENTRY* table, int tablelength){
    for(int i = 0; i < tablelength; i++){
        Node* currentNode = root;
        for(int j = 0; j < table[i].len; j+=4){
            //take leftmost four bits of ip, make corresponding child if doesnt exist, update currentNode
            unsigned int curFourBits = table[i].ip >> (28 - j);
            curFourBits = curFourBits & 0xf;
            if(currentNode->children[curFourBits] == NULL){
                currentNode->children[curFourBits] = setupNode();
            }
            currentNode = currentNode->children[curFourBits];
        }
        currentNode->hop = table[i].nexthop;
    }
}

int lookup(Node* root, int ip, int len){
    int longestMatch = 0;
    Node* currentNode = root;
    int j = 0;
    while(currentNode != NULL && j < len){
        if(currentNode->hop != 0) longestMatch = currentNode->hop;
        unsigned int curFourBits = ip >> (28 - j);
        curFourBits = curFourBits & 0xf;
        currentNode = currentNode->children[curFourBits];
        j += 4;
    }
    if(currentNode != NULL)
        if(currentNode->hop != 0) longestMatch = currentNode->hop;
    return longestMatch;
}

int main(){
    int tablelength1, tablelength2;
    uint64_t start, end, total, min, max;
    start = rdtsc();
    struct TABLEENTRY* table = set_table("ipv4_rrc_all_90build.txt", &tablelength1);
    end = rdtsc();
    printf("Build Table: %llu\n", end-start);

    Node* root = setupNode();
    start = rdtsc();
    insert(root, table, tablelength1);
    end = rdtsc();
    printf("Total build clocks: %llu\n", end-start);
    printf("Average clocks per insert: %f\n", ((double)end-start)/(double)tablelength1);
    printf("Number of Nodes: %d\n", numberOfNodes);

    uint64_t* clocks = (uint64_t*) malloc(sizeof(uint64_t) * tablelength1);
    total = 0;
    min = 999999999;
    max = 0;
    for(int i = 0; i < tablelength1; i++) {
        start = rdtsc();
        lookup(root, table[i].ip, table[i].len);
        end = rdtsc();
        total += end-start;
        if(end-start < min)
            min = end-start;
        if(end-start > max)
            max = end-start;
        clocks[i] = end-start;
    }
    writeArrayToFile(clocks, tablelength1, "fourbittrie_clocks.txt");
    printf("Total lookup clocks: %llu\n", total);
    printf("Average clocks per lookup: %f\n", (double)(total)/(double)tablelength1);
    printf("Max lookup clock: %d, Min lookup clock: %d\n", max, min);
    //insert 
    struct TABLEENTRY* table2 = set_table("ipv4_rrc_all_90build.txt", &tablelength2);
    start = rdtsc();
    insert(root, table, tablelength2);
    end = rdtsc();
    printf("Total insert clocks: %llu\n", end-start);
    printf("Average clocks per insert: %f\n", (double)(end-start)/(double)tablelength2);
    printf("Number of Nodes: %d\n", numberOfNodes);
    
    printf("%d", sizeof(struct fourbitNode));

    return 0;
}   