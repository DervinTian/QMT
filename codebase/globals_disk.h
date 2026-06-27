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

// Have a root inode that acts as like the "root" directory, that holds all of the tables

struct inode {
    uint32_t size;
    char owner[MAX_OWNER_SIZE];
    char tbl_name[MAX_TABLENAME_SIZE];
    uint32_t free_blocks[INODE_MAX_BLOCKS];
};

extern const uint32_t BLOCK_SIZE;
extern const uint64_t DISK_SIZE; // Just set it to 1MB for now
extern const uint32_t MAX_OWNER_SIZE;
extern const uint32_t MAX_TABLENAME_SIZE;

extern const uint32_t INODE_MAX_BLOCKS;

extern std::unordered_set<int> free_disk_blocks;

extern const std::string VM_DISK;

void convert_block_to_inode(inode &curr_inode);
std::set<int> get_free_blocks();