#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <unordered_set>

#include "interpreter.h"
#include "fill_args.h"
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
Implementation for the UPDATE function. Allows the user to modify the table.
Arguments:
    - arguments: contains tbl_name, the set values that we want to set, and any constraints for the table
*/
void update_qmt(const cmd_args &arguments){
    std::cout << "Running update implementation, can fill out semantics later\n";

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.update.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.update.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.update.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.update.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.update.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::vector<select_additional_args> additional_args;
    select_additional_args add_args;

    // For any additional lines after the select statement, such as WHERE
    for(int i = 0; i < arguments.update.additionals.size(); ++i){
        std::stringstream ss(arguments.update.additionals[i]);
        std::string cmd_type;
        ss >> cmd_type;
        
        for (char& c : cmd_type) {
            c = std::tolower(static_cast<unsigned char>(c));
        }

        // Fill in the arguments for the specified keyword
        if(cmd_type == "where"){
            add_args.where.tbl_name = arguments.update.tbl_name;
        }

        fill_in_additional_cmds[cmd_type](arguments.update.additionals[i], add_args);
        additional_args.push_back(add_args);
        executing_line_num++;
    }

    std::vector<std::vector<std::string>> schema = read_schema(schema_path);

    // Create a mapping to find which attribute goes to which column index
    std::unordered_map<std::string, int> attr_to_idx_mapping;
    for(size_t i = 0; i < schema[0].size(); ++i){
        attr_to_idx_mapping[schema[0][i]] = i;
    }

    // Read in the table into memory with the given constraints, to try and reduce on the memory load
    std::vector<std::vector<std::string>> filetered_table = from_qmt(table_path, additional_args);
    std::unordered_set<std::string> filtered_results;
    std::unordered_set<size_t> columns_in_there;

    std::vector<std::vector<std::string>> whole_table = from_qmt(table_path, std::vector<select_additional_args>{});
    size_t num_cols = whole_table.size();

    if(filetered_table.size() > 0){
        std::string search_key;
        for (size_t col = 0; col < filetered_table[0].size(); col++) {
            for (size_t row = 0; row < filetered_table.size(); row++) {
                search_key += filetered_table[row][col] + '/';
            }
            filtered_results.insert(search_key);
        }
    }

    if(whole_table.size() == 0){
        std::cout << "Empty table, cannot update records on an empty table!\n";
        exit(10);
    }

    columns_in_there.insert(0);

    for (size_t col = 0; col < whole_table[0].size(); col++) {
        std::string table_search_key;
        for (size_t row = 0; row < whole_table.size(); row++) {
            table_search_key += whole_table[row][col] + '/';
        }

        if(filtered_results.find(table_search_key) != filtered_results.end()){
            columns_in_there.insert(col);
        }
    }

    for(int i = 0; i < arguments.update.set_values.size(); ++i){
        std::string col_name = arguments.update.set_values[i].first;
        std::string new_val = arguments.update.set_values[i].second;

        int attr_idx = attr_to_idx_mapping[col_name];

        std::string actual_type = schema[1][attr_idx];

        bool types_match = check_value_against_type[actual_type](new_val);

        if(types_match){
            if(check_string(new_val) || check_char(new_val)){
                new_val = new_val.substr(1, new_val.size() - 2);
            }
        }
        else{
            std::cout << "Value " << new_val << " does not match the type for column " << col_name << std::endl;
        }

        for(size_t x = 0; x < whole_table[attr_idx].size(); ++x){
            auto it = columns_in_there.find(x);
            if(it != columns_in_there.end()){
                std::cout << "Here\n";
                std::cout << new_val << std::endl;
                whole_table[attr_idx][x] = new_val;
            }
        }

        executing_line_num++;
    }

    write_table_to_disk(whole_table, arguments.update.tbl_name);

    executing_line_num++;
    return;
}

/*
Implementation for the ALTER function. Allows the user to modify the table.
Arguments:
    - arguments: contains tbl_name, column name, and the column type to be inserted
        Supported keywords following alter include:
            - RENAME: rename the column
            - MODIFY: change the type of the column
*/
void alter_qmt(const cmd_args &arguments){
    std::cout << "Running alter implementation, can fill out semantics later\n";

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.create.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.alter.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.alter.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.alter.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.alter.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    if(arguments.alter.rename == 1){
        std::cout << "Column to be modified is: " << arguments.alter.column_name << " and modify it to: " << arguments.alter.new_column_name << std::endl;
        // Create a schema if there doesn't exist one already
        if(!fs::exists(schema_path)){
            std::cout << "Schema for Table " << arguments.alter.tbl_name << " doesn't exist!\n";
            exit(4);
        }

        std::ifstream schema_file(schema_path);
        std::string schema_line;

        std::vector<std::string> column_names;
        std::vector<std::string> column_types;

        std::getline(schema_file, schema_line);
        schema_file.close();

        std::stringstream comma_separated_values(schema_line);
        std::string name_type;
        while(std::getline(comma_separated_values, name_type, ',')){
            std::stringstream underscore_separated_values(name_type);
            std::string attr;

            int name_mode = 1;
            while(std::getline(underscore_separated_values, attr, '_')){
                if(name_mode){
                    // attr represents the name of the column
                    if(attr == arguments.alter.column_name){
                        column_names.push_back(arguments.alter.new_column_name);
                    }
                    else{
                        column_names.push_back(attr);
                    }
                }
                else{
                    column_types.push_back(attr);
                }
                name_mode = (name_mode + 1) % 2;
            }
        }

        std::ofstream schema_output_file(schema_path);
        for(size_t i = 0; i < column_names.size(); ++i){
            schema_output_file << column_names[i] + "_" + column_types[i] << ",";
        }

        executing_line_num++; // update the execution line number
    }
    else if(arguments.alter.modify == 1){
        std::cout << "Column to be modified is: " << arguments.alter.column_name << " and modify it to: " << arguments.alter.new_column_type << std::endl;
         // Create a schema if there doesn't exist one already
        if(!fs::exists(schema_path)){
            std::cout << "Schema for Table " << arguments.alter.tbl_name << " doesn't exist!\n";
            exit(4);
        }

        std::ifstream schema_file(schema_path);
        std::string schema_line;

        std::vector<std::string> column_names;
        std::vector<std::string> column_types;

        std::getline(schema_file, schema_line);
        schema_file.close();

        std::stringstream comma_separated_values(schema_line);
        std::string name_type;
        while(std::getline(comma_separated_values, name_type, ',')){
            std::stringstream underscore_separated_values(name_type);
            std::string attr;

            int type_mode = 0;
            bool right_column = false;
            while(std::getline(underscore_separated_values, attr, '_')){
                if(type_mode){
                    // attr represents the name of the column
                    if(right_column){
                        column_types.push_back(arguments.alter.new_column_type);
                        right_column = false;
                    }
                    else{
                        column_types.push_back(attr);
                    }
                }
                else{
                    if(attr == arguments.alter.column_name){
                        right_column = true;
                    }
                    column_names.push_back(attr);
                }
                type_mode = (type_mode + 1) % 2;
            }
        }

        std::ofstream schema_output_file(schema_path);
        for(size_t i = 0; i < column_names.size(); ++i){
            schema_output_file << column_names[i] + "_" + column_types[i] << ",";
        }

        executing_line_num++; // update the execution line number
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