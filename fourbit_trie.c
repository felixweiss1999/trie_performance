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
    return 0;
}   