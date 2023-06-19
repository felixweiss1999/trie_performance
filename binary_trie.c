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
                }
                currentNode = currentNode->right;
            } else { //go left
                if(currentNode->left == NULL){ //create right child
                    currentNode->left = (Node*) malloc(sizeof(Node));
                    currentNode->left->hop = 0;
                    currentNode->left->left = NULL;
                    currentNode->left->right = NULL;
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
    int tablelength1;
    uint64_t start, end;
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
    
    //verify table
    for(int i = 0; i < tablelength1; i++) if(lookup(&root, table[i].ip, table[i].len) != i+1) printf("Error for i = %d", i);
    
    start = rdtsc();
    for(int i = 0; i < tablelength1; i++) lookup(&root, table[i].ip, table[i].len);
    end = rdtsc();
    printf("Total lookup clocks: %llu\n", end-start);
    printf("Average clocks per lookup: %f", (double)(end-start)/(double)tablelength1);

    
    return 0;
}