#pragma once
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

enum cmd_type{
    SELECT,
    INSERT,
    CREATE,
    ADD_COL,
    DELETE,
};

struct select_args{
    std::vector<std::string> sel_columns;
    std::string data_dest;
    std::vector<std::string> additionals;
};

struct insert_args{
    std::string tbl_name;
    std::vector<std::string> values;
};

struct create_args{
    std::string tbl_name;
};

struct add_col_args{
    std::string tbl_name;
    std::string column_name;
    std::string type;
};

struct cmd_args{
    cmd_type cmd;

    select_args select;
    insert_args insert;
    create_args create;
    add_col_args add_cols;
};

extern std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, cmd_args&)>> fill_in_cmd;
extern std::unordered_map<std::string, std::function<void(const cmd_args&)>> cmd_impls;
extern std::unordered_map<std::string, std::function<bool(std::string&)>> check_value_against_type;
extern std::string db_path;

bool valid_table(std::string table);
bool valid_pathname(std::string pathname);