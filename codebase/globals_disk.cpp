#include <iostream>
#include <fstream>

#include "globals_disk.h"

std::unordered_set<int> free_disk_blocks;

const std::string VM_DISK = "vm_disk.bin";

uint32_t to_big_endian(uint32_t x) {
    return htonl(x);
}

void read_block_to_inode(inode &curr_inode, int blocknum){
    // go to the start of the block (AI helped with the syntax here as well)
    int block_byte_offset = blocknum * BLOCK_SIZE;
    std::ifstream disk(VM_DISK, std::ios::binary);
    disk.seekg(block_byte_offset);

    char block[BLOCK_SIZE];
    disk.read(block, BLOCK_SIZE);

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

void write_inode_to_block(const inode& curr_inode, int blocknum){
    // AI helped me to figure out how to go to a certain byte of a file and overwrite those bytes in place rather than reading everything in
    std::fstream disk(VM_DISK, std::ios::binary | std::ios::in | std::ios::out);

    int block_byte_offset = blocknum * BLOCK_SIZE;

    // Move to byte offset for the blocknum intiially
    disk.seekp(block_byte_offset);
    disk.write(reinterpret_cast<const char*>(&curr_inode.type), sizeof(curr_inode.type));
    block_byte_offset += sizeof(curr_inode.type);

    disk.seekp(block_byte_offset);
    uint32_t size_be = to_big_endian(curr_inode.size);
    disk.write(reinterpret_cast<const char*>(&size_be), sizeof(size_be));
    block_byte_offset += sizeof(curr_inode.size);

    disk.seekp(block_byte_offset);
    disk.write(reinterpret_cast<const char*>(&curr_inode.owner), sizeof(curr_inode.owner));
    block_byte_offset += sizeof(curr_inode.owner);

    disk.seekp(block_byte_offset);
    disk.write(reinterpret_cast<const char*>(&curr_inode.tbl_name), sizeof(curr_inode.tbl_name));
    block_byte_offset += sizeof(curr_inode.tbl_name);

    for(int i = 0; i < curr_inode.size; ++i){
        disk.seekp(block_byte_offset);
        uint32_t blocknum_be = to_big_endian(curr_inode.blocks[i]);
        disk.write(reinterpret_cast<const char*>(&blocknum_be), sizeof(blocknum_be));
        block_byte_offset += sizeof(curr_inode.blocks[i]);
    }

    return;
}

std::unordered_set<int> get_free_blocks(){

    std::unordered_set<int> all_free_blocks;
    std::unordered_set<int> curr_used_blocks;
    std::unordered_set<int> free_blocks;
    std::ifstream file(VM_DISK, std::ios::binary);

    for(int i = 0; i < NUM_DISK_BLOCKS; ++i){
        all_free_blocks.insert(i);
    }

    int curr_block_num = 0;
    while(curr_block_num < NUM_DISK_BLOCKS){
        inode curr_inode;
        read_block_to_inode(curr_inode, curr_block_num);
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