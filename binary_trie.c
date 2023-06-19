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
        for(int j = 0; j < table->len; j++){
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
    printf("Build Binary Trie: %llu\n", end-start);
    printf("Average build per Insert: %f\n", ((double)end-start)/(double)tablelength1);
    return 0;
}