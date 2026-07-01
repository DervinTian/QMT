#include <iostream>
#include <fstream>

#include "globals_disk.h"

std::unordered_set<int> free_disk_blocks;

const std::string VM_DISK = "vm_disk.bin";

void convert_block_to_inode(inode &curr_inode, const char* block){
    int running_idx = 0;
    std::string attr;

    for(int i = running_idx; i < sizeof(char) + running_idx; ++i){
        attr += block[i];
    }
    running_idx = sizeof(char) + running_idx;
    curr_inode.type = attr[0];

    if(curr_inode.type == '\0'){
        // This means that we do not have an inode and that we should not be looking at this data so return and make sure we don't reference it
        return;
    }
    attr.clear();
    
    // AI helped me read in the bytes into a big endian format, basically since the left are the big bits, we need to bit shift to the right over, and then or everything together to concatenate them into a single size value
    uint32_t size =
        (static_cast<uint8_t>(block[1]) << 24) |
        (static_cast<uint8_t>(block[2]) << 16) |
        (static_cast<uint8_t>(block[3]) << 8)  |
        static_cast<uint8_t>(block[4]);

    running_idx = sizeof(uint32_t) + running_idx;
    curr_inode.size = size;

    for(int i = running_idx; i < MAX_OWNER_SIZE + 1 + running_idx; ++i){
        attr += block[i];
    }
    running_idx = MAX_OWNER_SIZE + 1 + running_idx;
    for(int i = 0; i < attr.size(); ++i){
        curr_inode.owner[i] = attr[i];
    }
    attr.clear();

    for(int i = running_idx; i < MAX_TABLENAME_SIZE + 1 + running_idx; ++i){
        attr += block[i];
    }
    running_idx = MAX_TABLENAME_SIZE + 1 + running_idx;
    for(int i = 0; i < attr.size(); ++i){
        curr_inode.tbl_name[i] = attr[i];
    }
    attr.clear();

    uint32_t curr_inode_blocks_idx = 0;
    for(uint32_t i = running_idx; i < (size * sizeof(uint32_t)) + running_idx;){
        uint32_t inode_block_num =
            (static_cast<uint8_t>(block[i + 0]) << 24) |
            (static_cast<uint8_t>(block[i + 1]) << 16) |
            (static_cast<uint8_t>(block[i + 2]) << 8)  |
            static_cast<uint8_t>(block[i + 3]);
        i += 4;
        curr_inode.blocks[curr_inode_blocks_idx] = inode_block_num;
        curr_inode_blocks_idx++;
    }
    
    return;
}

std::unordered_set<int> get_free_blocks(){
    char block[BLOCK_SIZE];

    std::unordered_set<int> all_free_blocks;
    std::unordered_set<int> curr_used_blocks;
    std::unordered_set<int> free_blocks;
    std::ifstream file(VM_DISK, std::ios::binary);

    for(int i = 0; i < NUM_DISK_BLOCKS; ++i){
        all_free_blocks.insert(i);
    }

    int curr_block_num = 0;
    while(file.read(block, BLOCK_SIZE)){
        inode curr_inode;
        convert_block_to_inode(curr_inode, block);
        if(curr_inode.type != '\0'){
            curr_used_blocks.insert(curr_block_num);
            for(int i = 0; i < curr_inode.size; ++i){
                curr_used_blocks.insert(curr_inode.blocks[i]);
            }
        }

        curr_block_num++;
    }

    for(auto& x : all_free_blocks){
        if(curr_used_blocks.find(x) == curr_used_blocks.end()){
            // means that we didn't find the block number in the curr_used_blocks, it is free
            free_blocks.insert(x);
        }
    }

    return free_blocks;
}