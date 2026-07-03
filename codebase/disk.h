#pragma once
#include <iostream>

#include "globals_disk.h"

/*
Needs to allocate the parquet format direntries-esque block, essentially have a table for each column versus storing all the columns in one table with a delimiter
Allows us to infer the schema instead of like having to store the schema somewhere separately
*/
void create_qmt_disk(std::string tbl_name, std::string owner);

void delete_qmt_disk(std::string tbl_name, std::string owner);

void addcol_qmt_disk(std::string tbl_name, std::string owner, std::string col_name);

void write_qmt_disk();

void read_qmt_disk();
