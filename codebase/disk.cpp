#include <iostream>
#include <cassert>
#include <fstream>

#include "disk.h"
#include "globals_disk.h"

void create_qmt_disk(std::string tbl_name, std::string owner){

    inode new_inode;
    for(size_t i = 0; i < owner.size(); ++i){
        new_inode.owner[i] = owner[i];
    }
    new_inode.owner[owner.size()] = '\0';

    for(size_t i = 0; i < tbl_name.size(); ++i){
        new_inode.tbl_col_name[i] = tbl_name[i];
    }
    new_inode.tbl_col_name[tbl_name.size()] = '\0';

    new_inode.col_type[0] = '\0';

    new_inode.size = 0;
    new_inode.type = 'i';
    
    int next_free_block = 0;

    inode root_inode;
    read_block_to_inode(root_inode, 0);

    // Get a new disk block that we can give the inode
    next_free_block = *free_disk_blocks.begin();
    free_disk_blocks.erase(next_free_block);

    // Get the disk block, or allocate a new one, to write the col entries to
    int col_entry_block = 0;
    bool empty_flag = false;
    if(root_inode.size == 0){
        col_entry_block = *free_disk_blocks.begin();
        free_disk_blocks.erase(col_entry_block);
        empty_flag = true;
    }
    else{
        col_entry_block = root_inode.blocks[root_inode.size - 1];
    }

    std::vector<column_entries> block_col_entries;
    read_block_to_col_entries(block_col_entries, col_entry_block);

    assert(block_col_entries.size() == NUM_COL_ENTRIES);

    bool block_is_full = true;
    for(int i = 0; i < block_col_entries.size(); ++i){
        column_entries& col_entry = block_col_entries[i];
        if(col_entry.inode_blocknum == 0){
            block_is_full = false;
            for(int j = 0; j < tbl_name.size(); ++j){
                col_entry.tbl_col_name[j] = tbl_name[j];
            }
            col_entry.inode_blocknum = next_free_block;
            break;
        }
    }

    if(block_is_full){
        block_col_entries.clear();
        col_entry_block = *free_disk_blocks.begin();
        free_disk_blocks.erase(col_entry_block);
        for(int i = 0; i < NUM_COL_ENTRIES; ++i){
            column_entries col_entry;
            if(i == 0){
                for(int j = 0; j < tbl_name.size(); ++j){
                    col_entry.tbl_col_name[j] = tbl_name[j];
                }
                col_entry.inode_blocknum = next_free_block;
            }
            else{
                col_entry.inode_blocknum = 0;
            }
            block_col_entries.push_back(col_entry);
        }
    }

    write_inode_to_block(new_inode, next_free_block);
    write_col_entries_to_block(block_col_entries, col_entry_block);

    if(block_is_full || root_inode.size == 0){
        root_inode.size++;
        root_inode.blocks[root_inode.size - 1] = col_entry_block;
        write_inode_to_block(root_inode, 0);
    }

}

uint32_t find_table_inode_block(inode &root_inode, std::string tbl_name){
    uint32_t tbl_inode_block = 0;
    bool found = false;
    for(int i = 0; i < root_inode.size; ++i){
        uint32_t curr_search_block = root_inode.blocks[i];

        std::vector<column_entries> curr_col_entries;
        read_block_to_col_entries(curr_col_entries, curr_search_block);

        for(int i = 0; i < curr_col_entries.size(); ++i){
            column_entries &curr_col_entry = curr_col_entries[i];
            if(curr_col_entry.tbl_col_name == tbl_name){
                tbl_inode_block = curr_col_entry.inode_blocknum;
                found = true;
                break;
            }
        }

        if(found){
            break;
        }

    }

    return tbl_inode_block;
}

void delete_qmt_disk(std::string tbl_name, std::string owner){
    /*
    First step is to first find the inode for the given tbl_name. 
    What we need to do is to recursively and find all the inodes that are attached to it
    Then once we have that, to deallocate them, set the first byte to be \0 or something to tell future stuff not to look at it
    Then add the block that we deallocated to the free blocks list so that nothing is part of it

    If a column_entries thing is empty, then just deallocate it entirely and remove that block's mapping from the root
    */

    inode root_inode;
    read_block_to_inode(root_inode, 0);

    uint32_t tbl_inode_block = find_table_inode_block(root_inode, tbl_name);

    std::queue<uint32_t> search_queue;
    search_queue.push(tbl_inode_block);

    int root_col_entries_blocknum = 0;
    int root_col_entries_valid_size = 0;
    bool valid_flag = false;
    for(int i = 0; i < root_inode.size; ++i){

        std::vector<column_entries> root_inode_col_entries;
        std::vector<column_entries> root_inode_col_entries_copy;
        read_block_to_col_entries(root_inode_col_entries, root_inode.blocks[i]);

        int temp_valid_size = 0;
        for(int j = 0; j < root_inode_col_entries.size(); ++j){
            column_entries &curr_col_entry = root_inode_col_entries[j];

            if(curr_col_entry.inode_blocknum == tbl_inode_block){
                root_col_entries_blocknum = root_inode.blocks[i];
                valid_flag = true;
                continue;
            }
            
            root_inode_col_entries_copy.push_back(curr_col_entry);
        }

        while(root_inode_col_entries_copy.size() < root_inode_col_entries.size()){
            column_entries blank;
            blank.inode_blocknum = 0;
            blank.tbl_col_name[0] = '\0';
            root_inode_col_entries_copy.push_back(blank);
        }

        if(valid_flag){
            for(int k = 0; k < root_inode_col_entries_copy.size(); ++k){
                if(root_inode_col_entries_copy[k].inode_blocknum == 0){
                    break;
                }
                root_col_entries_valid_size++;
            }

            if(root_col_entries_valid_size == 0){
                int broken_inode_idx = 0;
                bool use_broken = false;

                for(int l = 0; l < root_inode.size; ++l){
                    if(root_inode.blocks[l] == root_col_entries_blocknum){
                        use_broken = true;
                        root_inode.blocks[l] = 0;
                        continue;
                    }

                    if(use_broken){
                        root_inode.blocks[l - 1] = root_inode.blocks[l];
                    }
                }

                root_inode.size--;

                free_disk_blocks.insert(root_col_entries_blocknum);

                write_inode_to_block(root_inode, 0);
            }

            write_col_entries_to_block(root_inode_col_entries_copy, root_col_entries_blocknum);
        }
    }

    // I guess this isn't very crash-consistent as I'm going to be taking a top-down approach, whereas it should be bottom-up, so essentially traverse all the way down and then go up
    // Will fix this later when I have more time, currently in a rush just to get something working
    while(search_queue.size() > 0){
        uint32_t front_inode_block = search_queue.front();
        search_queue.pop();
        inode front_inode;
        read_block_to_inode(front_inode, front_inode_block);

        for(int i = 0; i < front_inode.size; ++i){
            std::vector<column_entries> block_col_entries;
            read_block_to_col_entries(block_col_entries, front_inode.blocks[i]);

            for(int j = 0; j < block_col_entries.size(); ++j){

                column_entries &curr_col_entry = block_col_entries[j];

                if(curr_col_entry.inode_blocknum == 0){
                    break;
                }

                uint32_t associated_block_num = block_col_entries[j].inode_blocknum;
                search_queue.push(associated_block_num);

                curr_col_entry.inode_blocknum = 0;
            }

            write_col_entries_to_block(block_col_entries, front_inode.blocks[i]);
        }

        front_inode.type = '\0';
        front_inode.size = 0;

        free_disk_blocks.insert(front_inode_block);
        write_inode_to_block(front_inode, front_inode_block);
    }

}

void addcol_qmt_disk(std::string tbl_name, std::string owner, std::string col_name, std::string col_type){
    // First step is to find the table itself, by searching through the root directory
    inode root_inode;
    read_block_to_inode(root_inode, 0);

    uint32_t tbl_inode_block = find_table_inode_block(root_inode, tbl_name);

    int next_free_block = *free_disk_blocks.begin();
    free_disk_blocks.erase(next_free_block);

    inode tbl_inode;
    read_block_to_inode(tbl_inode, tbl_inode_block);

    bool modify_tbl_inode = false;
    std::vector<column_entries> tbl_col_entries;
    int col_entry_block = 0;

    inode column_inode;
    for(int i = 0; i < col_name.size(); ++i){
        column_inode.tbl_col_name[i] = col_name[i];
    }
    column_inode.tbl_col_name[col_name.size()] = '\0';
    for(int i = 0; i < col_type.size(); ++i){
        column_inode.col_type[i] = col_type[i];
    }
    column_inode.col_type[col_type.size()] = '\0';
    column_inode.size = 0;
    column_inode.type = 'i';
    for(int i = 0; i < owner.size(); ++i){
        column_inode.owner[i] = owner[i];
    }
    column_inode.owner[owner.size()] = '\0';

    int num_col_entries = 0;
    if(tbl_inode.size != 0){
        col_entry_block = tbl_inode.blocks[tbl_inode.size - 1];
        std::vector<column_entries> tmp_buffer_block;
        read_block_to_col_entries(tmp_buffer_block, col_entry_block);

        for(int i = 0; i < tmp_buffer_block.size(); ++i){
            column_entries &curr_entry = tmp_buffer_block[i];
            if(curr_entry.inode_blocknum == 0){
                break;
            }
            num_col_entries++;
        }
    }

    if(num_col_entries > 0 && num_col_entries < NUM_COL_ENTRIES){
        col_entry_block = tbl_inode.blocks[tbl_inode.size - 1];
        read_block_to_col_entries(tbl_col_entries, col_entry_block);

        for(int i = 0; i < tbl_col_entries.size(); ++i){
            column_entries &curr_entry = tbl_col_entries[i];
            if(curr_entry.inode_blocknum == 0){

                curr_entry.inode_blocknum = next_free_block;
                for(int j = 0; j < col_name.size(); ++j){
                    curr_entry.tbl_col_name[j] = col_name[j];
                }
                curr_entry.tbl_col_name[col_name.size()] = '\0';
                for(int k = 0; k < col_type.size(); ++k){
                    curr_entry.col_type[k] = col_type[k];
                }
                curr_entry.col_type[col_type.size()] = '\0';
                break;
            }
        }
    }

    if(num_col_entries == 0 || num_col_entries == NUM_COL_ENTRIES){
        modify_tbl_inode = true;
        col_entry_block = *free_disk_blocks.begin();
        free_disk_blocks.erase(col_entry_block);

        for(int i = 0; i < NUM_COL_ENTRIES; ++i){
            column_entries curr_entry;
            if(i == 0){
                curr_entry.inode_blocknum = next_free_block;
                for(int j = 0; j < col_name.size(); ++j){
                    curr_entry.tbl_col_name[j] = col_name[j];
                }
                curr_entry.tbl_col_name[col_name.size()] = '\0';
                for(int k = 0; k < col_type.size(); ++k){
                    curr_entry.col_type[k] = col_type[k];
                }
                curr_entry.col_type[col_type.size()] = '\0';
            }
            else{
                curr_entry.tbl_col_name[0] = '\0';
                curr_entry.col_type[0] = '\0';
                curr_entry.inode_blocknum = 0;
            }
            tbl_col_entries.push_back(curr_entry);
        }
    }

    // Now need to allocate the first new disk block for the column to use
    int new_entry_block = *free_disk_blocks.begin();
    free_disk_blocks.erase(new_entry_block);

    std::fstream disk(VM_DISK, std::ios::binary | std::ios::in | std::ios::out);
    disk.seekp(BLOCK_SIZE * new_entry_block);

    uint16_t initial_data_block_size = 2;
    uint16_t initial_data_block_size_be = to_big_endian_16(initial_data_block_size);
    disk.write(reinterpret_cast<const char*>(&initial_data_block_size_be), sizeof(initial_data_block_size_be));
    disk.close();

    column_inode.size++;
    column_inode.blocks[0] = new_entry_block;

    write_inode_to_block(column_inode, next_free_block);
    write_col_entries_to_block(tbl_col_entries, col_entry_block);
    if(modify_tbl_inode){
        tbl_inode.size++;
        tbl_inode.blocks[tbl_inode.size - 1] = col_entry_block;
        write_inode_to_block(tbl_inode, tbl_inode_block);
    }
}

/*
// object to be used in the comparisons
struct cmp_object{
    std::string param_string;
    int param_int;
    double param_double;
    bool param_bool;
    char param_char;

    cmp_object_type type;
};
*/
void write_qmt_disk(int blocknum, std::string owner, const cmp_object &input_obj, char mode){
    /*
    First find go to the disk block specified
    Then just go in and then since all the values will be delimited by '\0', find the last occurrence of that 
    */
    int byte_offset = BLOCK_SIZE * blocknum;

    std::ifstream disk(VM_DISK, std::ios::binary);
    disk.seekg(byte_offset);

    char block[BLOCK_SIZE];
    disk.read(block, BLOCK_SIZE);

    disk.close();

    uint16_t size =
        (static_cast<uint8_t>(block[0]) << 8)  |
        static_cast<uint8_t>(block[1]);
    
    
    std::string write_val;
    if(input_obj.type == STRING){
        write_val = input_obj.param_string;
    }
    else if(input_obj.type == CHAR){
        write_val += input_obj.param_char;
    }
    else if(input_obj.type == INT){
        write_val = std::to_string(input_obj.param_int);
    }
    else if(input_obj.type == DOUBLE){
        write_val = std::to_string(input_obj.param_double);
    }
    else if(input_obj.type == BOOL){
        if(input_obj.param_bool){
            write_val = "true";
        }
        else{
            write_val = "false";
        }
    }
    else{
        exit_with_error(UNKNOWN_TYPE, "");
    }
    
    if(size + write_val.size() > BLOCK_SIZE){
        // Can't fit all the data into the current block, will need a new one so call this function with a new blocknum
        return;
    }
    else{
        std::fstream disk(VM_DISK, std::ios::binary | std::ios::in | std::ios::out);
        if(mode == 'a'){
            disk.seekp(byte_offset + size);
        }
        else{
            disk.seekp(byte_offset + 2);
            size = 2;
        }
        disk.write(reinterpret_cast<const char*>(&write_val), sizeof(write_val));
        size += write_val.size() + 1;
        disk.seekp(byte_offset);
        uint16_t size_be = to_big_endian_16(size);
        disk.write(reinterpret_cast<const char*>(&size_be), sizeof(size_be));
    }
}

void read_qmt_disk(int blocknum, std::string val_type, std::string owner, std::vector<cmp_object> &return_object){
    /*
    data blocks will need to contain the number of bytes that are actually used
    The first two bytes will specify the number of bytes that are like not computer garble
    First find go to the disk block specified
    Need to handle error checking at some point
    Then just read in the value for that block
    Going to read everything in as strings I guess for now, and then we can let the code above decide how to parse it afterwards
    */

    int byte_offset = BLOCK_SIZE * blocknum;

    std::ifstream disk(VM_DISK, std::ios::binary);
    disk.seekg(byte_offset);

    char block[BLOCK_SIZE];
    disk.read(block, BLOCK_SIZE);

    uint16_t bytes_used =
        (static_cast<uint8_t>(block[0]) << 8)  |
        static_cast<uint8_t>(block[1]);
    

    std::string attr;
    cmp_object attr_obj;
    for(int i = 2; i < bytes_used; ++i){
        if(block[i] != '\0'){
            attr += block[i];
        }
        else{
            if(val_type == "string"){
                attr_obj.param_string = attr;
                attr_obj.type = STRING;
            }
            else if(val_type == "char"){
                attr_obj.param_char = attr[0];
                attr_obj.type = CHAR;
            }
            else if(val_type == "int"){
                attr_obj.param_int = std::stoi(attr);
                attr_obj.type = INT;
            }
            else if(val_type == "double"){
                attr_obj.param_int = std::stod(attr);
                attr_obj.type = DOUBLE;
            }
            else if(val_type == "bool"){
                if(attr == "true"){
                    attr_obj.param_bool = true;
                }
                else{
                    attr_obj.param_bool = false;
                }
                attr_obj.type = BOOL;
            }
            else{
                exit_with_error(UNKNOWN_TYPE, "");
            }
            return_object.push_back(attr_obj);
            attr.clear();
        }
    }

}

std::vector<int> get_blocknums_for_col(std::string tbl_name, std::string col_name, std::string owner){
    std::vector<int> result_blocks;

    inode root_inode;
    read_block_to_inode(root_inode, 0);

    int tbl_inode_blocknum = find_table_inode_block(root_inode, tbl_name);

    inode tbl_inode;
    read_block_to_inode(tbl_inode, tbl_inode_blocknum);

    int column_inode_blocknum = 0;

    bool done_flag = false;
    for(int i = 0; i < tbl_inode.size; ++i){
        int col_entries_block = tbl_inode.blocks[i];
        std::vector<column_entries> block_col_entries;

        read_block_to_col_entries(block_col_entries, col_entries_block);

        for(int j = 0; j < block_col_entries.size(); ++j){
            column_entries &curr_col_entry = block_col_entries[j];
            if(curr_col_entry.tbl_col_name == col_name){
                column_inode_blocknum = curr_col_entry.inode_blocknum;
                done_flag = true;
                break;
            }
        }

        if(done_flag){
            break;
        }
    }

    inode column_inode;
    read_block_to_inode(column_inode, column_inode_blocknum);

    for(int k = 0; k < column_inode.size; ++k){
        result_blocks.push_back(column_inode.blocks[k]);
    }

    return result_blocks;
}

std::vector<int> get_blocknums_for_all_cols_in_tbl(std::string tbl_name, std::string owner){
    std::vector<int> result_blocks;

    inode root_inode;
    read_block_to_inode(root_inode, 0);

    int tbl_inode_blocknum = find_table_inode_block(root_inode, tbl_name);

    inode tbl_inode;
    read_block_to_inode(tbl_inode, tbl_inode_blocknum);

    for(int i = 0; i < tbl_inode.size; ++i){
        int col_entries_block = tbl_inode.blocks[i];
        std::vector<column_entries> block_col_entries;

        read_block_to_col_entries(block_col_entries, col_entries_block);

        for(int j = 0; j < block_col_entries.size(); ++j){
            column_entries &curr_col_entry = block_col_entries[j];
            if(curr_col_entry.inode_blocknum == 0){
                break;
            }
            result_blocks.push_back(curr_col_entry.inode_blocknum);
        }

    }

    return result_blocks;
}

void write_columns_qmt_disk(std::string tbl_name, std::string col_name, std::string col_type, std::string owner, const std::vector<cmp_object> &input_obj){
    std::vector<int> col_blocks = get_blocknums_for_col(tbl_name, col_name, owner);

    inode root_inode;
    read_block_to_inode(root_inode, 0);

    int tbl_block = find_table_inode_block(root_inode, tbl_name);
    inode tbl_inode;
    read_block_to_inode(tbl_inode, tbl_block);

    int column_inode_blocknum = 0;
    for(int i = 0; i < tbl_inode.size; ++i){
        std::vector<column_entries> block_col_entries;
        read_block_to_col_entries(block_col_entries, tbl_inode.blocks[i]);

        for(int j = 0; j < block_col_entries.size(); ++j){
            if(block_col_entries[j].inode_blocknum == 0){
                break;
            }

            if(block_col_entries[j].tbl_col_name == col_name){
                column_inode_blocknum = block_col_entries[j].inode_blocknum;
                break;
            }
        }
    }
    inode column_inode;
    read_block_to_inode(column_inode, column_inode_blocknum);

    int num_bytes_used = 2;
    std::vector<cmp_object> block;
    int size_of_value = 0;
    int col_blocks_idx = 0;
    int block_to_write_to = 0;

    for(int i = 0; i < input_obj.size(); ++i){

        if(col_blocks_idx < col_blocks.size()){
            block_to_write_to = col_blocks[col_blocks_idx];
        }

        if(input_obj[i].type == STRING){
            size_of_value = sizeof(input_obj[i].param_string);
        }
        else if(input_obj[i].type == CHAR){
            size_of_value = sizeof(input_obj[i].param_char);
        }
        else if(input_obj[i].type == INT){
            size_of_value = sizeof(input_obj[i].param_int);
        }
        else if(input_obj[i].type == DOUBLE){
            size_of_value = sizeof(input_obj[i].param_double);
        }
        else if(input_obj[i].type == BOOL){
            size_of_value = sizeof(input_obj[i].param_bool);
        }
        else{
            exit_with_error(UNKNOWN_TYPE, "");
        }

        if(num_bytes_used + size_of_value > BLOCK_SIZE){
            if(col_blocks_idx + 1 < col_blocks.size()){
                col_blocks_idx++;
                block_to_write_to = col_blocks[col_blocks_idx];
            }
            else{
                column_inode.size++;
                int next_free_block = *free_disk_blocks.begin();
                free_disk_blocks.erase(next_free_block);

                column_inode.blocks[column_inode.size - 1] = next_free_block;
                block_to_write_to = next_free_block;
                if(i == 0){
                    write_qmt_disk(block_to_write_to, owner, input_obj[i], 'w'); // Just to clear it rq
                }
                else{
                    write_qmt_disk(block_to_write_to, owner, input_obj[i], 'a');
                }
                write_inode_to_block(column_inode, column_inode_blocknum);
            }
            // Need to allocate a new block or write to the next block
            num_bytes_used = 2 + size_of_value;
        }
        else{
            num_bytes_used += size_of_value;
            if(i == 0){
                write_qmt_disk(block_to_write_to, owner, input_obj[i], 'w'); // Just to clear it rq
            }
            else{
                write_qmt_disk(block_to_write_to, owner, input_obj[i], 'a');
            }
        }


    }
    

}