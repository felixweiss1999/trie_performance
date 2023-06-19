#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "table.c"
#include "util.c"

int countSetBits(uint16_t num) {
    int count = 0;
    while (num != 0) {
        num = num & (num - 1);
        count++;
    }
    return count;
}


int numberOfNodes = 0;
int numberOfNextHopsStored = 0;

typedef struct TreeNode {
    uint16_t child_exists;
    uint16_t prefix_exists;
    char* next_hop_arr;
    struct TreeNode* child_block;
} TreeNode;

void setupNode(TreeNode* setMeUp){
    numberOfNodes++;
    setMeUp->child_exists = 0;
    setMeUp->prefix_exists = 0;
    setMeUp->next_hop_arr = (char*) malloc(sizeof(char) * 15);
    setMeUp->child_block = (TreeNode*) malloc(sizeof(TreeNode) * 16);
}

unsigned char extract_shift4(uint32_t* prefix, int remainingLength) {
    unsigned int mask = 0xFFFFFFFF;  // Mask to extract all bits
    unsigned char ret;
    if (remainingLength < 4) {
        mask = (0xF0000000 << (4 - remainingLength));  // Update the mask based on remaining length
        ret = ((*prefix) & mask) >> (32 - remainingLength);
        *prefix = (*prefix) << remainingLength;
    } else {
        mask = (0xF0000000);
        ret = ((*prefix) & mask) >> 28;
        *prefix = (*prefix) << 4;
    }
    return ret;
}


TreeNode* constructTreeBitmap(TreeNode* root, struct TABLEENTRY* table, int tablelength){
    for(int i = 0; i < tablelength; i++){
        TreeNode* currentNode = root;
        //int depth = 0;
        uint32_t remaining_prefix = table[i].ip;
        int remaining_length = table[i].len;
        unsigned char current_first_four_bits;
        while(remaining_length > 3){ // equivalent to needs another child
            //if(i < 4) currentNode->verify = i*100+depth;
            current_first_four_bits = extract_shift4(&remaining_prefix, remaining_length);
            if(!((currentNode->child_exists >> current_first_four_bits) & 1)){ //child doesn't exist yet?
                currentNode->child_exists |= (1 << current_first_four_bits); //set bit at corresponding position to indicate child now exists
                setupNode(currentNode->child_block + current_first_four_bits);
            }
            //depth++;
            remaining_length -= 4;
            currentNode = currentNode->child_block + current_first_four_bits;
        }
        current_first_four_bits = extract_shift4(&remaining_prefix, remaining_length);
        //if(i < 4) currentNode->verify = i*100+depth;
        //remaining_length = (remaining_length + 4) % 4; //maps 4 to 0!
        //if(remaining_length == 0) current_first_four_bits = 0; // special case where this needs to be 0 because length is 0 anyways
        int pos = (1 << remaining_length) - 1 + current_first_four_bits;
        // printf("saving entry %d at depth %d with nexthop %d at position %d\n", i, depth, table[i].nexthop, pos);
        // if(currentNode->prefix_exists & (1 << pos)){
        //     printf("Warning: the above action will override existing entry %hhu\n", currentNode->next_hop_arr[pos]);
        // }
        //arrived at final node where nexthop info needs to be stored.
        //2^remaining_length - 1 equals the amount of classes of lower length
        currentNode->prefix_exists |= (1 << pos); //set bit at position to indicate prefix stored
        //if(pos >= 15)printf("Warning: saving at %d\n", pos);
        currentNode->next_hop_arr[pos] = table[i].nexthop; //make actual entry
    }
    return root;
}

void compressNode(TreeNode* node){
    //compress next hop info
    // if(node->verify >= 0 && node->verify <= 1000){
    //     printf("gotem on compress %d\n", node->verify);
    // }
    int count = countSetBits(node->prefix_exists);
    //numberOfNextHopsStored += count;
    if(count < 14 && count > 0){
        unsigned char* new_next_hop_arr = (unsigned char*) malloc(sizeof(char) * count);
        int q = 0;
        for (int i = 0; i < 15; i++) {
            if ((node->prefix_exists >> i) & 1) {
                new_next_hop_arr[q++] = node->next_hop_arr[i];
                // if(node->verify >= 0 && node->verify <= 1000){
                //     printf("On compressing %d: copied prefix %d from pos %d into pos %d\n", node->verify, new_next_hop_arr[q-1], i, q-1);
                // }
            }
        }
        //if(count != q)printf("Warning!");
        free(node->next_hop_arr);
        node->next_hop_arr = new_next_hop_arr;
    }
    //pretend we free the useless memory when no nexthop

    //compress child block
    count = countSetBits(node->child_exists);
    if(count < 16 && count > 0){
        TreeNode* new_child_block = (TreeNode*) malloc(sizeof(TreeNode) * count);
        int q = 0;
        for (int i = 0; i < 16; i++) {
            if ((node->child_exists >> i) & 1) {
                new_child_block[q++] = node->child_block[i];
                // if(node->child_block[i].verify >= 0 && node->child_block[i].verify <= 1000){
                //     printf("On compressing %d: copied child block %d from pos %d into pos %d\n", node->verify,new_child_block[q-1].verify, i, q-1);
                // }
            }
        }
        //if(count != q)printf("Warning!");
        free(node->child_block);
        node->child_block = new_child_block;
    }
    //pretend we free the memory when releasing lmao

    //recurse over compressed childblock
    for(int i = 0; i < count; i++){
        compressNode(node->child_block + i);
    }
}

int searchPrefixBitmap(uint32_t prefix_exists, unsigned char byte){
    int pos;
    for(int r = 3; r >= 0; r--){
        pos = (1 << r) - 1 + byte;
        if((prefix_exists >> pos) & 1)
            return pos;
        byte = byte >> 1;
    }
    return -1;
}

int countSetBitsUpToP(uint16_t num, int p) { //not including position p!
    int count = 0;
    uint16_t mask = 1;
    
    for (int i = 0; i < p; i++) {
        if (num & mask) {
            count++;
        }
        mask <<= 1;
    }
    
    return count;
}

unsigned char* lookupIP(TreeNode* node, uint32_t ip, int remaining_len){
    unsigned char* longestMatch = NULL;

    while(1){
        // if(node->verify >= 0 && node->verify <= 1000){
        //     printf("got %d on lookup\n", node->verify);
        // }else{
        //     printf("no: verify = %d", node->verify);
        // }
        unsigned char first_four_bits;
        int pos;
        if(remaining_len > 3){ //needs another child
            first_four_bits = (ip >> 28);
            ip = ip << 4;
            remaining_len -= 4;
            pos = searchPrefixBitmap(node->prefix_exists, first_four_bits >> 1); //expects only last 3 bits to be relevant
        } else { //arrived at child holding next hop
            first_four_bits = (ip >> (32-remaining_len)); //32 -rem
            //ip = ip << remaining_len;
            pos = (1 << remaining_len) - 1 + first_four_bits; //first_four_bits really is first 0 to 3 bits
            if(node->prefix_exists & (1 << pos) == 0){
                pos = -1;
            } else {
                return node->next_hop_arr + countSetBitsUpToP(node->prefix_exists, pos);
            }
        }
        // if(remaining_len < 0){
        //     remaining_len += 4;
        //     first_four_bits = first_four_bits << remaining_len;
        // }
        
        //check internal bitmap


        if(pos != -1){
            longestMatch = node->next_hop_arr + countSetBitsUpToP(node->prefix_exists, pos); //now points to best known next hop
        }
        //check external bitmap
        if(!((node->child_exists >> first_four_bits) & 1)) //there is no valid child potentially storing longer matching prefixes!
            return longestMatch;
        
        node = node->child_block + countSetBitsUpToP(node->child_exists, first_four_bits);
        

        
    }
}


TreeNode* constructIpv6TreeBitmap(TreeNode* root, struct AddressEntry* table, int tablelength){
    unsigned char* address_chunks = (unsigned char*) malloc(sizeof(unsigned char) * 32);
    for(int i = 0; i < tablelength; i++){
        getIPv6AddressChunks(table, i, address_chunks);
        TreeNode* currentNode = root;
        int remaining_length = table[i].subnet_mask;
        int j = 0;
        unsigned char current_first_four_bits;
        while(remaining_length > 3){
            current_first_four_bits = address_chunks[j++];
            if(!((currentNode->child_exists >> current_first_four_bits) & 1)){
                currentNode->child_exists |= (1 << current_first_four_bits);
                setupNode(currentNode->child_block + current_first_four_bits);
            }
            remaining_length -= 4;
            currentNode = currentNode->child_block + current_first_four_bits;
        }
        current_first_four_bits = address_chunks[j] >> (4-remaining_length);
        int pos = (1 << remaining_length) - 1 + current_first_four_bits;

        currentNode->prefix_exists |= (1 << pos);
        currentNode->next_hop_arr[pos] = (unsigned char) i;
    }
    free(address_chunks);
    return root;
}

unsigned char* lookupIPv6(TreeNode* node, unsigned char* addr_chnks, int remaining_len){
    unsigned char* longestMatch = NULL;
    int q = 0;
    while(1){
        unsigned char first_four_bits;
        int pos;
        if(remaining_len > 3){ //needs another child
            first_four_bits = addr_chnks[q++];
            remaining_len -= 4;
            pos = searchPrefixBitmap(node->prefix_exists, first_four_bits >> 1);
        } else {
            first_four_bits = addr_chnks[q] >> (4-remaining_len);
            pos = (1 << remaining_len) - 1 + first_four_bits;
            return node->next_hop_arr + countSetBitsUpToP(node->prefix_exists, pos);

        }

        if(pos != -1){
            longestMatch = node->next_hop_arr + countSetBitsUpToP(node->prefix_exists, pos); //now points to best known next hop
        }
        //check external bitmap
        if(!((node->child_exists >> first_four_bits) & 1))
            return longestMatch;
        //int a = countSetBitsUpToP(node->child_exists, first_four_bits);
        node = node->child_block + countSetBitsUpToP(node->child_exists, first_four_bits);
    }
}

int mainipv6(){
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock.\n");
        return 1;
    }

    int tablelength1;
    uint64_t start, end;
    TreeNode* root = (TreeNode*) malloc(sizeof(TreeNode));
    setupNode(root);
    //build
    start = rdtsc();
    struct AddressEntry* table = read_ipv6_table("ipv6/ipv6_rrc_90build.txt", &tablelength1);
    end = rdtsc();
    printf("%llu clocks: Build Table\n", end-start);

    //construct
    start = rdtsc();
    root = constructIpv6TreeBitmap(root, table, tablelength1);
    end = rdtsc();
    printf("%llu clocks: Build TreeBitmap with %d nodes storing %d entries\n", end-start, numberOfNodes, tablelength1);
    printf("Clocks per node: %f\n", (end-start)/(double)numberOfNodes);
    printf("Clocks per prefix: %f\n", (end-start)/(double)tablelength1);
    
    //compress
    compressNode(root);

    //query
    uint64_t totalclock = 0;
    int maxclock = 0;
    int minclock = 1000000000;
    uint64_t* clocks = (uint64_t*) malloc(sizeof(uint64_t) * tablelength1);
    unsigned char* address_chunks = (unsigned char*) malloc(32);
    for(int i = 0; i < tablelength1; i++){
        start = rdtsc();
        unsigned char* nexthop = lookupIPv6(root, getIPv6AddressChunks(table, i, address_chunks), table[i].subnet_mask);
        end = rdtsc();
        //printf("retrieved %u\n", *nexthop);
        clocks[i] = end-start;
        totalclock += end-start;
        if((end-start) > maxclock){
            //printf("maxclock %d for i=%d nexthop=%d t1=%llu t2=%llu iters=%llu\n", end-start, i, *nexthop, timestamp, timestamp2, iters);
            maxclock = end-start;
        }
        if((end-start) < minclock)
            minclock = end-start;
    }
    printf("Average clocks per lookup: %f\n", totalclock/(double)(tablelength1));
    printf("Maxclock: %d Minclock: %d\n", maxclock, minclock);
    printf("Size of Treenode: %d\n", sizeof(TreeNode));
    printf("Length of table 1: %d\n", tablelength1);
    writeArrayToFile(clocks,tablelength1,"resultsIPv6.txt");


    WSACleanup();
    return 0;
}

























int main(){
    int tablelength1;
    uint64_t start, end;
    TreeNode* root = (TreeNode*) malloc(sizeof(TreeNode));
    setupNode(root);
    //build
    start = rdtsc();
    struct TABLEENTRY* table = set_table("ipv4/build.txt", &tablelength1);
    end = rdtsc();
    printf("Elapsed clock cycles for building the table: %llu\n", end-start);
    
    

    //tablelength1 = 10000;
    start = rdtsc();
    root = constructTreeBitmap(root, table, 100000);
    end = rdtsc();
    printf("Elapsed clock cycles for building the (uncompressed) TreeBitmap: %llu with %d nodes.\n", end-start, numberOfNodes);
    printf("Clock cycles per node: %f\n", (end-start)/(double)numberOfNodes);
    printf("Clock cycles per prefix stored: %f\n", (end-start)/(double)100000);
    // compressNode(root);
    // printf("retrieved: %d", *lookupIP(root, table[0].ip));
    // return 0;
   
    // //insert
    // int tablelength2;
    // struct TABLEENTRY* table2 = set_table("ipv4/ipv4_rrc_all_10insert.txt", &tablelength2);
    // start = rdtsc();
    // root = constructTreeBitmap(root, table2, tablelength2);
    // end = rdtsc();
    // printf("Elapsed clock cycles for inserting: %d with %d nodes.\n", end-start, numberOfNodes);
    // printf("Clock cycles per insert: %f\n", (end-start)/(double)tablelength2);
    int tablelength2 = 0;
    
    //compress
    start = rdtsc();
    compressNode(root);
    end = rdtsc();
    printf("Elapsed clock cycles for table compression: %d\n", end-start);
    printf("Clock cycles per prefix stored: %f\n", (end-start)/(double)(100000+tablelength2));
    printf("Total nexthops stored: %d\n", numberOfNextHopsStored);
    
    
    //query
    uint64_t totalclock = 0;
    int maxclock = 0;
    int minclock = 1000000000;
    //uint32_t* query_table = set_query_table("ipv4/build.txt", &tablelength2);
    uint64_t* clocks = (uint64_t*) malloc(sizeof(uint64_t) * tablelength1);
    for(int i = 0; i < 100000; i++){
        start = rdtsc();
        unsigned char* nexthop = lookupIP(root, table[i].ip, table[i].len);
        //printf("retrieved %d\n", *nexthop);
        end = rdtsc();
        clocks[i] = end-start;
        totalclock += end-start;
        if((end-start) > maxclock){
            //printf("maxclock %d for i=%d nexthop=%d t1=%llu t2=%llu iters=%llu\n", end-start, i, *nexthop, timestamp, timestamp2, iters);
            maxclock = end-start;
        }
        if((end-start) < minclock)
            minclock = end-start;
    }
    printf("Average clocks per lookup: %f\n", totalclock/(double)(100000));
    printf("Maxclock: %d Minclock: %d\n", maxclock, minclock);
    printf("Size of Treenode: %d", sizeof(TreeNode));
    printf("Length of table 1: %d", 100000);
    //writeArrayToFile(clocks,tablelength1,"results.txt");
    return 0;
}
