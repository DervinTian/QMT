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

/*
Implementation for the SELECT function
Arguments:
    - arguments: contains tbl_name, selection columns, and additional data structures for additional constraints
*/
void select_qmt(const cmd_args &arguments){
    std::cout << "Running select implementation, can fill out semantics later\n";

    std::vector<select_additional_args> additional_args;
    select_additional_args add_args;

    // For any additional lines after the select statement, such as WHERE
    for(int i = 0; i < arguments.select.additionals.size(); ++i){
        std::stringstream ss(arguments.select.additionals[i]);
        std::string cmd_type;
        ss >> cmd_type;
        
        for (char& c : cmd_type) {
            c = std::tolower(static_cast<unsigned char>(c));
        }

        // Fill in the arguments for the specified keyword
        if(cmd_type == "where"){
            add_args.where.tbl_name = arguments.select.tbl_name;
        }

        fill_in_additional_cmds[cmd_type](arguments.select.additionals[i], add_args);
        additional_args.push_back(add_args);
        executing_line_num++;
    }

    // Check to make sure that all the names and paths are valid
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

    // Create a mapping to find which attribute goes to which column index
    std::unordered_map<std::string, int> attr_to_idx_mapping;
    for(size_t i = 0; i < schema[0].size(); ++i){
        attr_to_idx_mapping[schema[0][i]] = i;
    }

    // Read in the table into memory with the given constraints, to try and reduce on the memory load
    std::vector<std::vector<std::string>> table = from_qmt(table_path, additional_args);

    std::vector<std::vector<std::string>> result_table;
    std::vector<std::vector<std::string>> result_schema;

    result_schema.push_back(std::vector<std::string>{});
    result_schema.push_back(std::vector<std::string>{});

    // If they select all columns, then just display everything
    if(arguments.select.sel_columns.size() == 1 && arguments.select.sel_columns[0] == "*"){
        display_in_memory_table(table, schema);
        return;
    }

    // Otherwise, just show the columns that were specified
    for(size_t i = 0; i < arguments.select.sel_columns.size(); ++i){
        result_table.push_back(table[attr_to_idx_mapping[arguments.select.sel_columns[i]]]);
        result_schema[0].push_back(schema[0][attr_to_idx_mapping[arguments.select.sel_columns[i]]]);
        result_schema[1].push_back(schema[1][attr_to_idx_mapping[arguments.select.sel_columns[i]]]);;
    }

    // Print it out
    display_in_memory_table(result_table, result_schema);

    executing_line_num++; // update the execution line number
    return;

}

/*
Implementation for the INSERT function
Arguments:
    - arguments: contains tbl_name, and values to be inserted
*/
void insert_qmt(const cmd_args &arguments){
    std::cout << "Running insert implementation, can fill out semantics later\n";

    // Check to make sure that all the names and paths are valid
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

    // Go through the values and create a comma-separated line for the values and insert into the table file
    std::ofstream tbl(table_path, std::ios::app);

    for(size_t i = 0; i < arguments.insert.values.size(); ++i){
        tbl << arguments.insert.values[i];
        if(i != arguments.insert.values.size() - 1){
            tbl << ',';
        }
    }

    tbl << '\n';
    tbl.close();

    executing_line_num++; // update the execution line number
    return;
}

/*
Implementation for the CREATE function
Arguments:
    - arguments: contains tbl_name to be created
*/
void create_qmt(const cmd_args &arguments){
    std::cout << "Running create implementation, can fill out semantics later\n";

    // Check to make sure that all the names and paths are valid
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

    // Create a blank table in the database with the given table name
    std::ofstream tbl(table_path);

    if(arguments.create.additionals.size() > 0){
        std::vector<select_additional_args> constraints;
        for(size_t x = 0; x < arguments.create.additionals.size(); ++x){
            std::stringstream ss(arguments.create.additionals[x]);
            std::string cmd_type;
            ss >> cmd_type;
            
            for (char& c : cmd_type) {
                c = std::tolower(static_cast<unsigned char>(c));
            }
            select_additional_args add_args;

            for(size_t x = 0; x < arguments.create.additionals.size(); ++x){
                fill_in_additional_cmds[cmd_type](arguments.create.additionals[x], add_args);
            }

            if(add_args.curr_type == FROM){
                std::vector<std::vector<std::string>> in_memory_table;
                std::string csv_tbl_name;
                size_t pos = add_args.from.data_source.rfind('/');
                for(size_t q = pos + 1; q < add_args.from.data_source.size() - 4; ++q){
                    csv_tbl_name += add_args.from.data_source[q];
                }

                std::string schema_path = db_path + "/schemas/" + csv_tbl_name;

                if(!fs::exists(schema_path)){
                    std::vector<std::string> column_names;
                    std::ifstream csv_file(add_args.from.data_source);
                    std::string header;

                    std::getline(csv_file, header);
                    std::stringstream header_ss(header);
                    std::string attr;
                    if(add_args.from.header){
                        while(std::getline(header_ss, attr, ',')){
                            column_names.push_back(attr);
                        }

                    }
                    else{
                        std::string tmp_col_name = "column";
                        int counter = 1;
                        while(std::getline(header_ss, attr, ',')){
                            column_names.push_back(tmp_col_name + std::to_string(counter));
                            counter++;
                        }
                    }
                    std::string schema_line;
                    for(size_t col = 0; col < column_names.size(); ++col){
                        schema_line += column_names[col] + "_string,";
                    }

                    std::ofstream schema_file(schema_path);
                    schema_file << schema_line << '\n';
                    schema_file.close();
                }
                in_memory_table = from_qmt(add_args.from.data_source, constraints);

                if(in_memory_table.size() > 0){
                    for (size_t col = 0; col < in_memory_table[0].size(); col++) {
                        for (size_t row = 0; row < in_memory_table.size(); row++) {
                            tbl << in_memory_table[row][col] << ',';
                        }
                        tbl << "\n";
                    }
                }
            }

            executing_line_num++;
        }

    }

    executing_line_num++; // update the execution line number
    return;

}

/*
Implementation for the ADDCOL function
Arguments:
    - arguments: contains tbl_name, column name, and the column type to be inserted
*/
void add_col_qmt(const cmd_args &arguments){
    std::cout << "Running addcol implementation, can fill out semantics later\n";

    // Check to make sure that all the names and paths are valid
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

    // Create a schema if there doesn't exist one already
    if(!fs::exists(schema_path)){
        std::ofstream schema(schema_path);
    }

    // construct the comma-delimited value string to be inserted into the database schema
    std::string line;
    std::ifstream schema(schema_path);
    while(std::getline(schema, line));
    schema.close();

    line += arguments.add_cols.column_name + "_" + arguments.add_cols.type + ",";

    std::ofstream out_schema(schema_path);
    out_schema << line;

    out_schema.close();

    executing_line_num++; // update the execution line number
    return;

}

/*
Implementation for the DELETE function
Arguments:
    - arguments: contains tbl_name to be deleted
*/
void delete_qmt(const cmd_args &arguments){
    std::cout << "Running delete implementation, can fill out semantics later\n";

    // Do error checking to ensure that the database and the tables are valid paths
    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.deleted.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.deleted.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.deleted.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.deleted.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.deleted.tbl_name  << " doesn't exist!\n";
        exit(3);
    }

    // If everything is valid, remove the schema and the data table from the database
    fs::remove(table_path);

    if(fs::exists(schema_path)){
        fs::remove(schema_path);
    }

    executing_line_num++; // update the execution line number
    return;
}