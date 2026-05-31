#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cctype>

#include "interpreter.h"
#include "globals.h"

namespace fs = std::filesystem;

void select_qmt(cmd_args arguments){
    std::cout << "Running select implementation, can fill out semantics later\n";

    std::cout << arguments.select.additionals[0] << std::endl;

    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.select.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.select.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.select.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.select.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.select.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::vector<std::vector<std::string>> schema = read_schema(schema_path);
    std::unordered_map<std::string, int> attr_to_idx_mapping;

    for(size_t i = 0; i < schema[0].size(); ++i){
        attr_to_idx_mapping[schema[0][i]] = i;
    }

    std::vector<std::vector<std::string>> table = read_in_table(table_path);

    std::vector<std::vector<std::string>> result_table;
    std::vector<std::vector<std::string>> result_schema;

    result_schema.push_back(std::vector<std::string>{});
    result_schema.push_back(std::vector<std::string>{});

    if(arguments.select.sel_columns.size() == 1 && arguments.select.sel_columns[0] == "*"){
        display_in_memory_table(table, schema);
        return;
    }

    for(size_t i = 0; i < arguments.select.sel_columns.size(); ++i){
        result_table.push_back(table[attr_to_idx_mapping[arguments.select.sel_columns[i]]]);
        result_schema[0].push_back(schema[0][attr_to_idx_mapping[arguments.select.sel_columns[i]]]);
        result_schema[1].push_back(schema[1][attr_to_idx_mapping[arguments.select.sel_columns[i]]]);;
    }

    display_in_memory_table(result_table, result_schema);
    executing_line_num++;
    return;

}

void insert_qmt(cmd_args arguments){
    std::cout << "Running insert implementation, can fill out semantics later\n";

    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.insert.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.insert.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.insert.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.insert.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::ofstream tbl(table_path, std::ios::app);

    for(size_t i = 0; i < arguments.insert.values.size(); ++i){
        tbl << arguments.insert.values[i];
        if(i != arguments.insert.values.size() - 1){
            tbl << ',';
        }
    }

    tbl << '\n';
    tbl.close();
    executing_line_num++;

    return;
}

void create_qmt(cmd_args arguments){
    std::cout << "Running create implementation, can fill out semantics later\n";

    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.create.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.create.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.create.tbl_name;

    if(fs::exists(table_path)){
        std::cout << "Table " << arguments.create.tbl_name << " already exists!\n";
        exit(3);
    }

    std::ofstream tbl(table_path);

    executing_line_num++;
    return;

}

void add_col_qmt(cmd_args arguments){
    std::cout << "Running addcol implementation, can fill out semantics later\n";

    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.create.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.add_cols.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.add_cols.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.add_cols.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.create.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    if(!fs::exists(schema_path)){
        std::ofstream schema(schema_path);
    }

    std::string line;
    std::ifstream schema(schema_path);
    while(std::getline(schema, line));
    schema.close();

    line += arguments.add_cols.column_name + "_" + arguments.add_cols.type + ",";

    std::ofstream out_schema(schema_path);
    out_schema << line;

    out_schema.close();

    executing_line_num++;
    return;

}

void delete_qmt(cmd_args arguments){
    std::cout << "Running delete implementation, can fill out semantics later\n";

    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.deleted.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.deleted.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.deleted.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.deleted.tbl_name  << " doesn't exist!\n";
        exit(3);
    }

    fs::remove(table_path);

    executing_line_num++;
    return;
}