#pragma once
#include <iostream>

#include "globals_disk.h"
#include "globals.h"

/*
Needs to allocate the parquet format direntries-esque block, essentially have a table for each column versus storing all the columns in one table with a delimiter
Allows us to infer the schema instead of like having to store the schema somewhere separately
*/
void create_qmt_disk(std::string tbl_name, std::string owner);

void delete_qmt_disk(std::string tbl_name, std::string owner);

void addcol_qmt_disk(std::string tbl_name, std::string owner, std::string col_name, std::string col_type);

void write_qmt_disk(int blocknum, std::string owner, const cmp_object &input_obj, char mode);

void read_qmt_disk(int blocknum, std::string val_type, std::string owner, std::vector<cmp_object> &return_object);

uint32_t find_table_inode_block(inode &root_inode, std::string tbl_name);

std::vector<int> get_blocknums_for_col(std::string tbl_name, std::string col_name, std::string owner);

std::vector<int> get_blocknums_for_all_cols_in_tbl(std::string tbl_name, std::string owner);

void write_columns_qmt_disk(std::string tbl_name, std::string col_name, std::string col_type, std::string owner, const std::vector<cmp_object> &input_obj);
