#include <iostream>
#include <cassert>

#include "disk.h"
#include "globals_disk.h"

void create_qmt_disk(std::string tbl_name, std::string owner){

    inode new_inode;
    for(size_t i = 0; i < owner.size(); ++i){
        new_inode.owner[i] = owner[i];
    }
    new_inode.owner[owner.size()] = '\0';

    for(size_t i = 0; i < tbl_name.size(); ++i){
        new_inode.tbl_name[i] = tbl_name[i];
    }
    new_inode.tbl_name[tbl_name.size()] = '\0';

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
                col_entry.tbl_name[j] = tbl_name[j];
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
                    col_entry.tbl_name[j] = tbl_name[j];
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
            if(curr_col_entry.tbl_name == tbl_name){
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

    std::cout << "Root inode size is: " << root_inode.size << std::endl;
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
            blank.tbl_name[0] = '\0';
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
        std::cout << "Currently searching " << front_inode_block << std::endl;
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

void addcol_qmt_disk(std::string tbl_name, std::string owner, std::string col_name){
    // First step is to find the table itself, by searching through the root directory
    inode root_inode;
    read_block_to_inode(root_inode, 0);

    uint32_t tbl_inode_block = find_table_inode_block(root_inode, tbl_name);

    int next_free_block = *free_disk_blocks.begin();
    free_disk_blocks.erase(next_free_block);

    std::cout << "Found table inode block at " << tbl_inode_block << std::endl;
    std::cout << "The block to use for the column's inode is " << next_free_block << std::endl;

    inode tbl_inode;
    read_block_to_inode(tbl_inode, tbl_inode_block);

    bool is_full = true;
    bool modify_tbl_inode = false;
    std::vector<column_entries> tbl_col_entries;
    int col_entry_block = 0;

    inode column_inode;
    for(int i = 0; i < col_name.size(); ++i){
        column_inode.tbl_name[i] = col_name[i];
    }
    column_inode.size = 0;
    column_inode.type = 'i';
    for(int i = 0; i < owner.size(); ++i){
        column_inode.owner[i] = owner[i];
    }

    if(tbl_inode.size != 0){
        col_entry_block = tbl_inode.blocks[tbl_inode.size];
        read_block_to_col_entries(tbl_col_entries, tbl_inode.blocks[tbl_inode.size]);

        for(int i = 0; i < tbl_col_entries.size(); ++i){
            column_entries &curr_entry = tbl_col_entries[i];
            if(curr_entry.inode_blocknum == 0){
                is_full = false;

                curr_entry.inode_blocknum = next_free_block;
                for(int j = 0; j < col_name.size(); ++j){
                    curr_entry.tbl_name[j] = col_name[j];
                }
            }
        }
    }

    if(is_full || tbl_inode.size == 0){
        modify_tbl_inode = true;
        col_entry_block = *free_disk_blocks.begin();
        free_disk_blocks.erase(col_entry_block);

        for(int i = 0; i < NUM_COL_ENTRIES; ++i){
            column_entries curr_entry;
            if(i == 0){
                curr_entry.inode_blocknum = next_free_block;
                for(int j = 0; j < col_name.size(); ++j){
                    curr_entry.tbl_name[j] = col_name[j];
                }
            }
            else{
                curr_entry.inode_blocknum = 0;
            }
            tbl_col_entries.push_back(curr_entry);
        }
    }

    std::cout << col_entry_block << std::endl;
    write_inode_to_block(column_inode, next_free_block);
    write_col_entries_to_block(tbl_col_entries, col_entry_block);
    if(modify_tbl_inode){
        tbl_inode.size++;
        tbl_inode.blocks[tbl_inode.size - 1] = col_entry_block;
        write_inode_to_block(tbl_inode, tbl_inode_block);
    }
}
