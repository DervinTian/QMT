#include "disk.h"
#include <iostream>

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

    int next_free_block = 0;
    for(auto &x : free_disk_blocks){
        next_free_block = x;
        break;
    }
    free_disk_blocks.erase(next_free_block);

    new_inode.size = 0;
    new_inode.type = 'i';

    write_inode_to_block(new_inode, next_free_block);

    // Need to also initialize an empty block of column_entries as well

}
