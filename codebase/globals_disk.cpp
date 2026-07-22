#include <iostream>
#include <fstream>
#include <cassert>

#include "globals_disk.h"

std::unordered_set<int> free_disk_blocks;
std::unordered_set<int> used_disk_blocks;
std::unordered_map<std::string, int> table_inode_to_disk_block;

const std::string VM_DISK = "vm_disk.bin";

uint32_t to_big_endian(uint32_t x) {
    return htonl(x);
}

uint16_t to_big_endian_16(uint16_t x){
    return htons(x);
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
        curr_inode.tbl_col_name[i] = attr[i];
    }
    attr.clear();

    for(int i = running_idx; i < MAX_COLUMN_NAME_SIZE + 1 + running_idx; ++i){
        attr += block[i];
    }
    running_idx = MAX_COLUMN_NAME_SIZE + 1 + running_idx;
    for(int i = 0; i < attr.size(); ++i){
        curr_inode.col_type[i] = attr[i];
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

void read_block_to_col_entries(std::vector<column_entries>& column_entires, int blocknum){
    int block_byte_offset = blocknum * BLOCK_SIZE;

    std::ifstream disk(VM_DISK, std::ios::binary);
    disk.seekg(block_byte_offset);

    column_entries col_entry;
    char block[BLOCK_SIZE];
    disk.read(block, BLOCK_SIZE);

    int running_idx = 0;

    for(int i = 0; i < NUM_COL_ENTRIES; ++i){

        for(int j = 0; j < MAX_TABLENAME_SIZE + 1; ++j){
            col_entry.tbl_col_name[j] = block[running_idx + j];
        }

        running_idx += MAX_TABLENAME_SIZE + 1;
        for(int k = 0; k < MAX_COLUMN_NAME_SIZE + 1; ++k){
            col_entry.col_type[k] = block[running_idx + k];
        }
        running_idx += MAX_COLUMN_NAME_SIZE + 1;

        uint32_t inode_blocknum =
                (static_cast<uint8_t>(block[running_idx + 0]) << 24) |
                (static_cast<uint8_t>(block[running_idx + 1]) << 16) |
                (static_cast<uint8_t>(block[running_idx + 2]) << 8)  |
                static_cast<uint8_t>(block[running_idx + 3]);

        col_entry.inode_blocknum = inode_blocknum;

        column_entires.push_back(col_entry);
        running_idx += sizeof(uint32_t);
        disk.seekg(block_byte_offset + running_idx);
    }
}

void write_col_entries_to_block(const std::vector<column_entries>& column_entires, int blocknum){
    std::fstream disk(VM_DISK, std::ios::binary | std::ios::in | std::ios::out);
    int block_byte_offset = blocknum * BLOCK_SIZE;

    for(int i = 0; i < column_entires.size(); ++i){
        column_entries col_entry = column_entires[i];
        disk.seekp(block_byte_offset);
        disk.write(reinterpret_cast<const char*>(&col_entry.tbl_col_name), sizeof(col_entry.tbl_col_name));
        block_byte_offset += sizeof(col_entry.tbl_col_name);

        disk.seekp(block_byte_offset);
        disk.write(reinterpret_cast<const char*>(&col_entry.col_type), sizeof(col_entry.col_type));
        block_byte_offset += sizeof(col_entry.col_type);

        disk.seekp(block_byte_offset);
        uint32_t blocknum_be = to_big_endian(col_entry.inode_blocknum);
        disk.write(reinterpret_cast<const char*>(&blocknum_be), sizeof(blocknum_be));
        block_byte_offset += sizeof(blocknum_be);
    }

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
    disk.write(reinterpret_cast<const char*>(&curr_inode.tbl_col_name), sizeof(curr_inode.tbl_col_name));
    block_byte_offset += sizeof(curr_inode.tbl_col_name);

    disk.seekp(block_byte_offset);
    disk.write(reinterpret_cast<const char*>(&curr_inode.col_type), sizeof(curr_inode.col_type));
    block_byte_offset += sizeof(curr_inode.col_type);

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
    int starting_blocknum = 0;

    for(int i = 0; i < NUM_DISK_BLOCKS; ++i){
        all_free_blocks.insert(i);
    }

    inode curr_inode;
    read_block_to_inode(curr_inode, starting_blocknum);

    std::stack<int> searched_blocknums;
    searched_blocknums.push(starting_blocknum);
    curr_used_blocks.insert(starting_blocknum);

    while(searched_blocknums.size() > 0){
        int curr_blocknum = searched_blocknums.top();
        inode curr_inode;
        searched_blocknums.pop();

        read_block_to_inode(curr_inode, curr_blocknum);

        if(curr_inode.type == 'i'){
            for(int i = 0; i < curr_inode.size; ++i){
                std::vector<column_entries> curr_col_entries;
                read_block_to_col_entries(curr_col_entries, curr_inode.blocks[i]);
                curr_used_blocks.insert(curr_inode.blocks[i]);
                assert(curr_col_entries.size() == NUM_COL_ENTRIES);

                for(int j = 0; j < curr_col_entries.size(); ++j){
                    column_entries curr_col_entry = curr_col_entries[j];
                    if(curr_col_entry.inode_blocknum == 0){
                        break;
                    }
                    
                    searched_blocknums.push(curr_col_entry.inode_blocknum);
                    curr_used_blocks.insert(curr_col_entry.inode_blocknum);
                }
            }
        }

    }

    used_disk_blocks = curr_used_blocks;

    for(auto& x : all_free_blocks){
        if(curr_used_blocks.find(x) == curr_used_blocks.end()){
            // means that we didn't find the block number in the curr_used_blocks, it is free
            free_blocks.insert(x);
        }
    }

    return free_blocks;
}

void print_out_disk(){

    inode root_inode;
    read_block_to_inode(root_inode, 0);

    for(int i = 0; i < root_inode.size; ++i){
        std::vector<column_entries> block_col_entries;
        int next_inode_blocknum = root_inode.blocks[i];
        read_block_to_col_entries(block_col_entries, next_inode_blocknum);

        for(int j = 0; j < block_col_entries.size(); ++j){
            if(block_col_entries[j].inode_blocknum == 0){
                break;
            }
            column_entries &curr_col_entry = block_col_entries[j];
            inode tbl_inode;
            read_block_to_inode(tbl_inode, curr_col_entry.inode_blocknum);

            std::cout << "The table " << tbl_inode.tbl_col_name << " contains the following columns: " << std::endl;

            for(int k = 0; k < tbl_inode.size; ++k){
                std::vector<column_entries> tbl_column_entries;
                int tbl_col_blocknum = tbl_inode.blocks[k];
                read_block_to_col_entries(tbl_column_entries, tbl_col_blocknum);

                for(int l = 0; l < tbl_column_entries.size(); ++l){
                    if(tbl_column_entries[l].inode_blocknum == 0){
                        break;
                    }
                    std::cout << tbl_column_entries[l].tbl_col_name << " (" << tbl_column_entries[l].col_type << ") " << std::endl;
                }
                
            }

            std::cout << std::endl;
        }
        
    }
}