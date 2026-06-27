#include <iostream>
#include <fstream>

#include "globals_disk.h"

const uint64_t DISK_SIZE = 1048576; // Just set it to 1MB for now
const uint32_t INODE_MAX_BLOCKS = DISK_SIZE / BLOCK_SIZE;
const uint32_t BLOCK_SIZE = 512;
const uint32_t MAX_OWNER_SIZE = 11;
const uint32_t MAX_TABLENAME_SIZE = 47;

/*
Inode layout size:
4 bytes for size
11 + 1 bytes for owner name size
47 + 1 bytes for table name size
448 bytes left for availble disk blocks (MAX 112 blocks per inode)
*/

std::unordered_set<int> free_disk_blocks;

const std::string VM_DISK = "vm_disk.bin";

void convert_block_to_inode(inode &curr_inode, const char* block){
    int running_idx = 0;

    uint32_t size = 0;
    std::string attr;
    for(int i = running_idx; i < sizeof(uint32_t) + running_idx; ++i){
        attr += block[i];
        running_idx++;
    }

    size = std::stoi(attr, nullptr, 16);
    curr_inode.size = size;
    attr.clear();

    for(int i = running_idx; i < MAX_OWNER_SIZE + running_idx; ++i){
        attr += block[i];
        running_idx++;
    }
    for(int i = 0; i < attr.size(); ++i){
        curr_inode.owner[i] = attr[i];
    }
    attr.clear();

    for(int i = running_idx; i < MAX_OWNER_SIZE + running_idx; ++i){
        attr += block[i];
        running_idx++;
    }
    for(int i = 0; i < attr.size(); ++i){
        curr_inode.tbl_name[i] = attr[i];
    }
    attr.clear();

    int inode_blocks_idx = 0;
    for(uint32_t i = running_idx; i < size + running_idx; ++i){
        attr += block[i];

        if(attr.size() == sizeof(uint32_t)){
            // We have hit another block num, so read it in
            uint32_t block_num = std::stoi(attr, nullptr, 16);
            attr.clear();
            curr_inode.free_blocks[inode_blocks_idx] = block_num;
            inode_blocks_idx++;
        }
    }
    
    return;
}

std::set<int> get_free_blocks(){
    char block[BLOCK_SIZE];

    std::set<int> curr_free_blocks;
    std::ifstream file(VM_DISK, std::ios::binary);

    for(int i = 0; i < INODE_MAX_BLOCKS; ++i){
        free_disk_blocks.insert(i);
    }

    int curr_block_num = 0;
    file.read(block, BLOCK_SIZE);

    return curr_free_blocks;
}