#pragma once
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>
#include <unordered_map>
#include <unordered_set>
#include <set>

const uint64_t DISK_SIZE = 1048576; // Just set it to 1MB for now
const uint32_t BLOCK_SIZE = 512;
const uint32_t MAX_OWNER_SIZE = 10;
const uint32_t MAX_TABLENAME_SIZE = 47;
const uint32_t INODE_MAX_BLOCKS = (DISK_SIZE - 1 - 4 - (MAX_OWNER_SIZE + 1) - (MAX_TABLENAME_SIZE + 1)) / BLOCK_SIZE;
const uint32_t NUM_DISK_BLOCKS = DISK_SIZE / BLOCK_SIZE;

/*
Inode layout size:
1 byte for the type
4 bytes for size
10 + 1 bytes for owner name size
47 + 1 bytes for table name size
448 bytes left for availble disk blocks (MAX 112 blocks per inode)
*/

// If a disk block is empty, then the first character of that disk block read in will be '\0', for both inodes and regular table disk blocks

struct inode {
    char type;
    uint32_t size;
    char owner[MAX_OWNER_SIZE + 1];
    char tbl_name[MAX_TABLENAME_SIZE + 1];
    uint32_t blocks[INODE_MAX_BLOCKS];
};

extern const uint32_t INODE_MAX_BLOCKS;

extern std::unordered_set<int> free_disk_blocks;

extern const std::string VM_DISK;

void read_block_to_inode(inode &curr_inode, int blocknum);
void write_inode_to_block(const inode& curr_inode, int blocknum);
uint32_t to_big_endian(uint32_t x);
std::unordered_set<int> get_free_blocks();