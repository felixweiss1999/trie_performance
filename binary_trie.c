#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "table.c"
#include "util.c"


typedef struct binNode{
    int hop;
    struct binNode* left;
    struct binNode* right;
} Node;

int numberOfNodes = 0;

void insert(Node* root, struct TABLEENTRY* table, int tablelength){
    for(int i = 0; i < tablelength; i++){
        Node* currentNode = root;
        for(int j = 0; j < table[i].len; j++){
            //printf("%d", table[i].ip & ((unsigned int)1 << (32-j)));
            if(table[i].ip & ((unsigned int)1 << (31-j))){ //go right
                if(currentNode->right == NULL){ //create right child
                    currentNode->right = (Node*) malloc(sizeof(Node));
                    currentNode->right->hop = 0;
                    currentNode->right->left = NULL;
                    currentNode->right->right = NULL;
                    numberOfNodes++;
                }
                currentNode = currentNode->right;
            } else { //go left
                if(currentNode->left == NULL){ //create right child
                    currentNode->left = (Node*) malloc(sizeof(Node));
                    currentNode->left->hop = 0;
                    currentNode->left->left = NULL;
                    currentNode->left->right = NULL;
                    numberOfNodes++;
                }
                currentNode = currentNode->left;
            }
        }
        currentNode->hop = table[i].nexthop;
    }
}

int lookup(Node* root, int ip, int len){
    int longestMatch = 0;
    Node* currentNode = root;
    int j = 0;
    while(currentNode != NULL && j <= len){
        if(currentNode->hop != 0) longestMatch = currentNode->hop;
        if(ip & (1 << (31 - j++))){
            currentNode = currentNode->right;
        } else {
            currentNode = currentNode->left;
        }
    }
    return longestMatch;
}

int main(){
    int tablelength1, tablelength2;
    uint64_t start, end, total, min, max;
    start = rdtsc();
    struct TABLEENTRY* table = set_table("ipv4_rrc_all_90build.txt", &tablelength1);
    end = rdtsc();
    printf("Build Table: %llu\n", end-start);

    Node root = {0, 0, 0};
    start = rdtsc();
    insert(&root, table, tablelength1);
    end = rdtsc();
    printf("Total build clocks: %llu\n", end-start);
    printf("Average clocks per insert: %f\n", ((double)end-start)/(double)tablelength1);
    printf("Number of Nodes: %d\n", numberOfNodes);
    //verify table
    for(int i = 0; i < tablelength1; i++) if(lookup(&root, table[i].ip, table[i].len) != i+1) printf("Error for i = %d", i);
    
    uint64_t* clocks = (uint64_t*) malloc(sizeof(uint64_t) * tablelength1);
    total = 0;
    min = 999999999;
    max = 0;
    for(int i = 0; i < tablelength1; i++) {
        start = rdtsc();
        lookup(&root, table[i].ip, table[i].len);
        end = rdtsc();
        total += end-start;
        if(end-start < min)
            min = end-start;
        if(end-start > max)
            max = end-start;
        clocks[i] = end-start;
    }
    writeArrayToFile(clocks, tablelength1, "btrie_clocks.txt");
    printf("Total lookup clocks: %llu\n", total);
    printf("Average clocks per lookup: %f\n", (double)(total)/(double)tablelength1);
    printf("Max lookup clock: %d, Min lookup clock: %d\n", max, min);
    //insert 
    struct TABLEENTRY* table2 = set_table("ipv4_rrc_all_90build.txt", &tablelength2);
    start = rdtsc();
    insert(&root, table, tablelength2);
    end = rdtsc();
    printf("Total insert clocks: %llu\n", end-start);
    printf("Average clocks per insert: %f\n", (double)(end-start)/(double)tablelength2);
    printf("Number of Nodes: %d\n", numberOfNodes);
    //verify table
    for(int i = 0; i < tablelength2; i++) if(lookup(&root, table2[i].ip, table2[i].len) != i+1) printf("Error for i = %d", i);
    printf("%d", sizeof(struct binNode*));
    return 0;
}