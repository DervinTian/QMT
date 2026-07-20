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
const uint32_t MAX_TABLENAME_SIZE = 59;
const uint32_t INODE_MAX_BLOCKS = (DISK_SIZE - 1 - 4 - (MAX_OWNER_SIZE + 1) - (MAX_TABLENAME_SIZE + 1)) / BLOCK_SIZE;
const uint32_t NUM_DISK_BLOCKS = DISK_SIZE / BLOCK_SIZE;
const uint32_t NUM_COL_ENTRIES = BLOCK_SIZE / 64;

// Need a root node forsho lol

/*
Inode layout size:
1 byte for the type
4 bytes for size
10 + 1 bytes for owner name size
59 + 1 bytes for table name size
436 bytes left for availble disk blocks (MAX 112 blocks per inode)
*/

// If a disk block is empty, then the first character of that disk block read in will be '\0', for both inodes and regular table disk blocks

struct inode {
    char type;
    uint32_t size;
    char owner[MAX_OWNER_SIZE + 1];
    char tbl_name[MAX_TABLENAME_SIZE + 1]; // I guess tbl_name represents both the table name or the column name
    uint32_t blocks[INODE_MAX_BLOCKS];
};

// Have a struct of column_entries, that will all fit within a block, make it around 64 bytes, so 8 columns per table as a max for now
struct column_entries{
    char tbl_name[MAX_TABLENAME_SIZE + 1]; // I guess tbl_name represents both the table name or the column name
    uint32_t inode_blocknum;
};

extern const uint32_t INODE_MAX_BLOCKS;

extern std::unordered_set<int> free_disk_blocks;
extern std::unordered_set<int> used_disk_blocks;

extern std::unordered_map<std::string, int> table_inode_to_disk_block;

extern const std::string VM_DISK;

void print_out_disk();
void read_block_to_inode(inode &curr_inode, int blocknum);
void write_inode_to_block(const inode& curr_inode, int blocknum);
void read_block_to_col_entries(std::vector<column_entries>& column_entries, int blocknum);
void write_col_entries_to_block(const std::vector<column_entries>& column_entires, int blocknum);
uint32_t to_big_endian(uint32_t x);
uint16_t to_big_endian_16(uint16_t x);
std::unordered_set<int> get_free_blocks();