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
    NONE
};

struct where_args{
    std::string lhs_expression;
    std::string rhs_expression;
    std::string comparator;
};

struct select_additional_args{
    where_args where;
};

struct select_args{
    std::string tbl_name;
    std::vector<std::string> sel_columns;
    std::vector<std::string> additionals;

    select_additional_args additional_args;
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

struct delete_args{
    std::string tbl_name;
};

struct cmd_args{
    cmd_type cmd;

    select_args select;
    insert_args insert;
    create_args create;
    add_col_args add_cols;
    delete_args deleted;
};

extern std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, cmd_args&)>> fill_in_cmd;
extern std::unordered_map<std::string, std::function<void(const cmd_args&)>> cmd_impls;
extern std::unordered_map<std::string, std::function<bool(std::string&)>> check_value_against_type;
extern std::string db_path;
extern std::vector<std::string> in_memory_script;
extern int executing_line_num;

bool valid_table(std::string table);
bool valid_pathname(std::string pathname);

std::vector<std::vector<std::string>> read_schema(const std::string &schema_path);
std::vector<std::vector<std::string>> read_in_table(const std::string &table_path);
void display_in_memory_table(const std::vector<std::vector<std::string>> &table, const std::vector<std::vector<std::string>> &schema);