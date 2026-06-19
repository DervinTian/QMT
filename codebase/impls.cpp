#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <bitset>
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
    executing_line_num++; // update the execution line number

    bool join = false;
    bool order = false;

    // vector to hold all the different additional constraints, like WHERE clauses and stuff
    std::vector<select_additional_args> where_additional_args;
    std::vector<select_additional_args> join_additional_args;
    std::vector<select_additional_args> order_additional_args;

    std::vector<std::string> additional_cmd;
    select_additional_args add_args; // an auxillary variable to be used to populate the additional_args vector

    cmd_type curr_mode = NONE;
    cmd_type prev_mode = NONE;

    std::string prev_cmd_type;
    std::string cmd_type;

    // For any additional lines after the select statement, such as WHERE
    for(int i = 0; i < arguments.select.additionals.size(); ++i){
        prev_cmd_type = cmd_type;
        prev_mode = curr_mode;
        std::stringstream ss(arguments.select.additionals[i]);
        ss >> cmd_type;
        
        // lower-case the command to not worry about any weird camel-casing cases
        for (char& c : cmd_type) {
            c = std::tolower(static_cast<unsigned char>(c));
        }

        // Fill in the arguments for the specified keyword
        if(cmd_type == "where"){
            curr_mode = WHERE;
        }
        else if(cmd_type == "join"){
            curr_mode = JOIN;
            join = true;
        }
        else if(cmd_type == "order"){
            curr_mode = ORDER;
            order = true;
        }

        if(prev_mode == curr_mode){
            additional_cmd.push_back(arguments.select.additionals[i]);
        }
        else if(prev_mode == NONE){
            additional_cmd.push_back(arguments.select.additionals[i]);
        }
        else if(prev_mode != curr_mode){
            // for the command, in the future there will be more than just where, fill in those arguments
            try{
                fill_in_additional_cmds[prev_cmd_type](additional_cmd, add_args);
                additional_cmd.clear();

                if(prev_mode == WHERE){
                    where_additional_args.push_back(add_args);
                }
                else if(prev_mode == JOIN){
                    join_additional_args.push_back(add_args);
                }
                else if(prev_mode == ORDER){
                    order_additional_args.push_back(add_args);
                }
            }
            catch(...){
                exit_with_error(UNKNOWN_CMD, cmd_type);
            }
            additional_cmd.push_back(arguments.select.additionals[i]);
        }
        else{
            exit_with_error(UNKNOWN_CMD, cmd_type);
        }

        executing_line_num++; // essentially we just processed a where statement, so add an extra new line
    }

    try{

        if(curr_mode == WHERE){
            fill_in_additional_cmds["where"](additional_cmd, add_args);
            where_additional_args.push_back(add_args);
        }
        else if(curr_mode == JOIN){
            fill_in_additional_cmds["join"](additional_cmd, add_args);
            join_additional_args.push_back(add_args);
        }
        else if(curr_mode == ORDER){
            fill_in_additional_cmds["order"](additional_cmd, add_args);
            order_additional_args.push_back(add_args);
        }
        additional_cmd.clear();
    }
    catch(const std::bad_function_call& e){}
    catch(...){
        std::cerr << "Caught exception" << std::endl;
    }

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }


    std::string smaller_table_size_name;
    int smallest_table_so_far = INT_MAX;
    for(auto &pair : arguments.select.table_columns){
        std::string path = db_path + "/" + pair.first;
        std::string binary_row_count;
        if(!fs::exists(path)){
            exit_with_error(NULL_TABLE, pair.first);
        }

        std::ifstream input_file(path);
        std::getline(input_file, binary_row_count);

        int decimal_row_count = std::stoi(binary_row_count, nullptr, 2);

        if(decimal_row_count < smallest_table_so_far){
            smallest_table_so_far = decimal_row_count;
            smaller_table_size_name = pair.first;
        }
    }


    std::string table_path = db_path + "/" + smaller_table_size_name;
    std::string schema_path = db_path + "/schemas/" + smaller_table_size_name;

    if(!fs::exists(table_path)){
        exit_with_error(NULL_TABLE, smaller_table_size_name);
    }

    std::vector<std::vector<std::string>> schema = read_schema(schema_path);

    // Create a mapping to find which attribute goes to which column index
    std::unordered_map<std::string, int> attr_to_idx_mapping;
    for(size_t i = 0; i < schema[0].size(); ++i){
        attr_to_idx_mapping[schema[0][i]] = i;
    }

    // Read in the table into memory with the given constraints, to try and reduce on the memory load
    std::vector<std::vector<std::string>> table = from_qmt(table_path, where_additional_args);
    executing_line_num++; // Update because we actually are executing the FROM keyword

    std::vector<std::vector<std::string>> result_table;
    std::vector<std::vector<std::string>> result_schema;

    if(!join){ 
        result_schema.push_back(std::vector<std::string>{});
        result_schema.push_back(std::vector<std::string>{});

        // If they select all columns, then just display everything
        if(arguments.select.sel_columns.size() == 1 && arguments.select.sel_columns[0] == "*"){
            if(order){
                table = order_qmt(order_additional_args, table, schema);
            }
            display_in_memory_table(table, schema);
            return;
        }

        // Otherwise, just show the columns that were specified
        for(size_t i = 0; i < arguments.select.sel_columns.size(); ++i){
            result_table.push_back(table[attr_to_idx_mapping[arguments.select.sel_columns[i]]]);
            result_schema[0].push_back(schema[0][attr_to_idx_mapping[arguments.select.sel_columns[i]]]);
            result_schema[1].push_back(schema[1][attr_to_idx_mapping[arguments.select.sel_columns[i]]]);;
        }

    }
    else{

        std::string join_result_schema;
        result_table = join_qmt(join_additional_args, table, schema, join_result_schema);

        result_schema = vectorize_schema(join_result_schema);

    }

    if(order){
        result_table = order_qmt(order_additional_args, result_table, result_schema);
    }
    // Print it out
    display_in_memory_table(result_table, result_schema);

    return;

}

/*
Implementation for the INSERT function
Arguments:
    - arguments: contains tbl_name, and values to be inserted
*/
void insert_qmt(const cmd_args &arguments){
    std::cout << "Running insert implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }

    if(!valid_table(arguments.insert.tbl_name)){
        exit_with_error(INVALID_TABLENAME, arguments.insert.tbl_name);
    }

    std::string table_path = db_path + "/" + arguments.insert.tbl_name;

    if(!fs::exists(table_path)){
        exit_with_error(NULL_TABLE, arguments.insert.tbl_name);
    }

    std::ifstream input_tbl(table_path);
    std::string binary_row_count;
    std::getline(input_tbl, binary_row_count);
    std::cout << binary_row_count << std::endl;
    int num_rows = std::stoi(binary_row_count, nullptr, 2);
    input_tbl.close();

    // Go through the values and create a comma-separated line for the values and insert into the table file
    std::ofstream tbl(table_path, std::ios::app);

    // For the values that we want to insert, go through and write it all out to disk with a ',' delimiter
    for(size_t i = 0; i < arguments.insert.values.size(); ++i){
        tbl << arguments.insert.values[i] << ",";
    }

    tbl << '\n';
    tbl.close();


    num_rows++;
    binary_row_count = std::bitset<12>(num_rows).to_string(); // HARDCODED TO HAVE PAGE SIZE OF 4096

    // Used AI to generate how to modify the a line_inplace given the same amount of characters
    std::fstream tbl_file(table_path, std::ios::in | std::ios::out);

    if (!tbl_file){
        exit_with_error(NULL_INPUT_FILE, table_path);
    }

    std::string line;
    // Read the first line
    std::getline(tbl_file, line);
    // Go back to the beginning of the file
    tbl_file.seekp(0);
    // Overwrite the first line
    tbl_file << binary_row_count << std::endl;

    return;
}

/*
Implementation for the CREATE function
Arguments:
    - arguments: contains tbl_name to be created
*/
void create_qmt(const cmd_args &arguments){
    std::cout << "Running create implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }

    if(!valid_table(arguments.create.tbl_name)){
        exit_with_error(INVALID_TABLENAME, arguments.create.tbl_name);
    }
    
    std::string table_path = db_path + "/" + arguments.create.tbl_name;

    if(fs::exists(table_path)){
        exit_with_error(NULL_TABLE, arguments.create.tbl_name);
    }

    // If there is an additional parameter, which would be the FROM to specify a certain .csv file to read in from
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

            fill_in_additional_cmds[cmd_type](arguments.create.additionals, add_args);
            
            // If we are executing a FROm command go here
            if(add_args.curr_type == FROM){
                std::vector<std::vector<std::string>> in_memory_table;
                std::string csv_tbl_name;
                size_t pos = add_args.from.data_source.rfind('/'); // find the last /, after that will be the table name
                for(size_t q = pos + 1; q < add_args.from.data_source.size() - 4; ++q){ // omit the last .csv part of it
                    csv_tbl_name += add_args.from.data_source[q];
                }

                std::string schema_path = db_path + "/schemas/" + csv_tbl_name;

                // There shouldn't be a schema since we are creating it from scratch
                if(!fs::exists(schema_path)){
                    std::vector<std::string> column_names;
                    std::ifstream csv_file(add_args.from.data_source);
                    std::string header;

                    std::getline(csv_file, header); // get the top line
                    std::stringstream header_ss(header);
                    std::string attr;
                    if(add_args.from.header){ // if there is a header, that top line is the header
                        while(std::getline(header_ss, attr, ',')){
                            column_names.push_back(attr); // add that to our column names, representing our table schema
                        }
                    }
                    else{ // if there was no header, we can use the top line to count how many columns we need to create
                        std::string tmp_col_name = "column";
                        int counter = 1;
                        while(std::getline(header_ss, attr, ',')){
                            column_names.push_back(tmp_col_name + std::to_string(counter)); // make temporary column names, like column1 if no header given
                            counter++;
                        }
                    }

                    // Since csv files do not really come with the type, just assume everything is a string type as a default
                    std::string schema_line;
                    for(size_t col = 0; col < column_names.size(); ++col){
                        schema_line += column_names[col] + "_string,";
                    }

                    // write out the newly created schema to the schema path
                    std::ofstream schema_file(schema_path);
                    schema_file << schema_line << '\n';
                    schema_file.close();
                }
                else{
                    exit_with_error(SCHEMA_EXISTS, csv_tbl_name);
                }

                // Now that we have a schema, read in the table from the data source
                in_memory_table = from_qmt(add_args.from.data_source, constraints);

                // Write the table according to what we read
                write_table_to_disk(in_memory_table, arguments.create.tbl_name);
            }

            // updating our PC essentially to move to the next line to interpret
            executing_line_num++;
        }

    }
    else{
        std::cout << "Here\n";
        // Create a blank table in the database with the given table name
        std::ofstream tbl(table_path);
        int num_leading_zeros = std::log2(PAGE_SIZE);
        for(int i = 0; i < num_leading_zeros; ++i){
            tbl << "0";
        }
        tbl << std::endl;
        tbl.close();
    }

    return;

}

/*
Implementation for the UPDATE function. Allows the user to modify the table.
Arguments:
    - arguments: contains tbl_name, the set values that we want to set, and any constraints for the table
*/
void update_qmt(const cmd_args &arguments){
    std::cout << "Running update implementation, can fill out semantics later\n";
    executing_line_num++;

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }

    if(!valid_table(arguments.update.tbl_name)){
        exit_with_error(INVALID_TABLENAME, arguments.update.tbl_name);
    }

    std::string table_path = db_path + "/" + arguments.update.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.update.tbl_name;

    if(!fs::exists(table_path)){
        exit_with_error(NULL_TABLE, arguments.update.tbl_name);
    }

    // Similar to the SELECt keyword, need to keep track of what constraints there are on the UPDATE clause
    std::vector<select_additional_args> additional_args;
    std::vector<std::string> aux_additional_args;
    select_additional_args add_args;

    cmd_type prev_mode = NONE;
    cmd_type curr_mode = NONE;

    std::string prev_cmd_type;
    std::string cmd_type;

    // For any additional lines after the select statement, such as WHERE
    for(int i = 0; i < arguments.update.additionals.size(); ++i){
        prev_mode = curr_mode;
        std::stringstream ss(arguments.update.additionals[i]);
        ss >> cmd_type;
        
        for (char& c : cmd_type) {
            c = std::tolower(static_cast<unsigned char>(c));
        }

        // Fill in the arguments for the specified keyword, just where for now
        if(cmd_type == "where"){
            add_args.where.tbl_name = arguments.update.tbl_name;
            curr_mode = WHERE;
        }

        if(prev_mode == curr_mode){
            aux_additional_args.push_back(arguments.update.additionals[i]);
        }
        else if(prev_mode == NONE){
            aux_additional_args.push_back(arguments.update.additionals[i]);
        }
        else{
            // fill in the appropriate commands for that keyword
            fill_in_additional_cmds[prev_cmd_type](aux_additional_args, add_args);
            aux_additional_args.clear();

            if(curr_mode == WHERE){
                additional_args.push_back(add_args);
            }
            aux_additional_args.push_back(arguments.select.additionals[i]);
        }

        executing_line_num++;
    }

    fill_in_additional_cmds[cmd_type](aux_additional_args, add_args);
    if(curr_mode == WHERE){
        additional_args.push_back(add_args);
    }

    // Read in the schema
    std::vector<std::vector<std::string>> schema = read_schema(schema_path);

    // Create a mapping to find which attribute goes to which column index, used later to find column_names and column_types
    std::unordered_map<std::string, int> attr_to_idx_mapping;
    for(size_t i = 0; i < schema[0].size(); ++i){
        attr_to_idx_mapping[schema[0][i]] = i;
    }

    // Read in the table the filtered results, so we know what to update
    std::vector<std::vector<std::string>> filetered_table = from_qmt(table_path, additional_args);
    std::unordered_set<std::string> filtered_results;
    std::unordered_set<size_t> columns_in_there;

    // Read in the entire table into memory, so that we can change the exact value from the original table to the new value
    std::vector<std::vector<std::string>> whole_table = from_qmt(table_path, std::vector<select_additional_args>{});
    size_t num_cols = whole_table.size();

    if(filetered_table.size() > 0){
        // create a hash for the column, so that we can trace which entry in the filtered table goes with the original table
        std::string search_key;
        for (size_t col = 0; col < filetered_table[0].size(); col++) {
            for (size_t row = 0; row < filetered_table.size(); row++) {
                search_key += filetered_table[row][col] + '/';
            }
            filtered_results.insert(search_key);
        }
    }

    if(whole_table.size() == 0){
        exit_with_error(EMPTY_TABLE, "");
    }

    // For the original table, find which of those columns correspond to the filtered columns and keep track of them in the columns_in_there set
    for (size_t col = 0; col < whole_table[0].size(); col++) {
        std::string table_search_key;
        for (size_t row = 0; row < whole_table.size(); row++) {
            table_search_key += whole_table[row][col] + '/';
        }

        if(filtered_results.find(table_search_key) != filtered_results.end()){
            columns_in_there.insert(col);
        }
    }

    // For the values that we want to set, go through and set them in the original whole table
    for(int i = 0; i < arguments.update.set_values.size(); ++i){
        std::string col_name = arguments.update.set_values[i].first;
        std::string new_val = arguments.update.set_values[i].second;

        int attr_idx = attr_to_idx_mapping[col_name]; // find the index used to find the type and name

        std::string actual_type = schema[1][attr_idx];

        bool types_match = check_value_against_type[actual_type](new_val); // make sure to check that the types actually match up so that we can actually update it properly

        if(types_match){
            if(check_string(new_val) || check_char(new_val)){ // if it is a string or a char, then just trim off the "", or the ''
                new_val = new_val.substr(1, new_val.size() - 2);
            }
        }
        else{
            std::cout << "Value " << new_val << " does not match the type for column " << col_name << std::endl;
        }

        // go through the entire table, and for the rows that corresponded with the ones returned by the filter, change those values
        for(size_t x = 0; x < whole_table[attr_idx].size(); ++x){
            auto it = columns_in_there.find(x);
            if(it != columns_in_there.end()){
                whole_table[attr_idx][x] = new_val;
            }
        }

        executing_line_num++; // update to run the next line after the FROM
    }

    // write the result back to disk
    write_table_to_disk(whole_table, arguments.update.tbl_name);

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
    executing_line_num++; // update the execution line number

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        exit_with_error(6, "");
    }

    if(!valid_table(arguments.create.tbl_name)){
        exit_with_error(7, arguments.create.tbl_name);
    }

    std::string table_path = db_path + "/" + arguments.alter.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.alter.tbl_name;

    if(!fs::exists(table_path)){
        exit_with_error(8, table_path);
    }

    // If we are renaming go here
    if(arguments.alter.rename == 1){
        std::cout << "Column to be modified is: " << arguments.alter.column_name << " and modify it to: " << arguments.alter.new_column_name << std::endl;
        // Create a schema if there doesn't exist one already
        if(!fs::exists(schema_path)){
            exit_with_error(NULL_SCHEMA, arguments.alter.tbl_name);
        }

        std::ifstream schema_file(schema_path);
        std::string schema_line;

        std::vector<std::string> column_names;
        std::vector<std::string> column_types;

        // Get the schema representing the column names and column_types
        std::getline(schema_file, schema_line);
        schema_file.close();

        std::stringstream comma_separated_values(schema_line);
        std::string name_type;
        while(std::getline(comma_separated_values, name_type, ',')){
            std::stringstream underscore_separated_values(name_type);
            std::string attr;

            int name_mode = 1;
            // Go thorugh the line and get the individual column names and values, keeping track of them
            while(std::getline(underscore_separated_values, attr, '_')){
                if(name_mode){
                    // attr represents the name of the column
                    if(attr == arguments.alter.column_name){
                        column_names.push_back(arguments.alter.new_column_name); // if we found the column to rename, then set the column name to that
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

        // write out the schema to disk
        std::ofstream schema_output_file(schema_path);
        for(size_t i = 0; i < column_names.size(); ++i){
            schema_output_file << column_names[i] + "_" + column_types[i] << ",";
        }

        executing_line_num++; // update the execution line number
    }
    else if(arguments.alter.modify == 1){ // if we are modifying the column go here
        std::cout << "Column to be modified is: " << arguments.alter.column_name << " and modify it to: " << arguments.alter.new_column_type << std::endl;
         // Create a schema if there doesn't exist one already
        if(!fs::exists(schema_path)){
            exit_with_error(NULL_SCHEMA, arguments.alter.tbl_name);
        }

        std::ifstream schema_file(schema_path);
        std::string schema_line;

        std::vector<std::string> column_names;
        std::vector<std::string> column_types;

        // read in the first line of the schema to find out the column types and names
        std::getline(schema_file, schema_line);
        schema_file.close();

        std::stringstream comma_separated_values(schema_line);
        std::string name_type;
        // do the same process as above, keeping track of the types this time
        while(std::getline(comma_separated_values, name_type, ',')){
            std::stringstream underscore_separated_values(name_type);
            std::string attr;

            int type_mode = 0;
            bool right_column = false;
            while(std::getline(underscore_separated_values, attr, '_')){
                if(type_mode){
                    // attr represents the name of the column
                    if(right_column){ // if we find the right column name that we want to alter the type of
                        column_types.push_back(arguments.alter.new_column_type); // keep track of the new type instead
                        right_column = false;
                    }
                    else{
                        column_types.push_back(attr);
                    }
                }
                else{
                    if(attr == arguments.alter.column_name){ // we found the right column, next we just need to read in it's type accordingly
                        right_column = true;
                    }
                    column_names.push_back(attr);
                }
                type_mode = (type_mode + 1) % 2;
            }
        }

        // Write out the schema back to disk
        std::ofstream schema_output_file(schema_path);
        for(size_t i = 0; i < column_names.size(); ++i){
            schema_output_file << column_names[i] + "_" + column_types[i] << ",";
        }

        executing_line_num++; // update the execution line number
    }

    return;
}

/*
Implementation for the ADDCOL function
Arguments:
    - arguments: contains tbl_name, column name, and the column type to be inserted
*/
void add_col_qmt(const cmd_args &arguments){
    std::cout << "Running addcol implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }

    if(!valid_table(arguments.create.tbl_name)){
        exit_with_error(INVALID_TABLENAME, arguments.create.tbl_name);
    }

    std::string table_path = db_path + "/" + arguments.add_cols.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.add_cols.tbl_name;

    if(!fs::exists(table_path)){
        exit_with_error(NULL_TABLE, arguments.add_cols.tbl_name);
    }

    // Create a schema if there doesn't exist one already
    if(!fs::exists(schema_path)){
        std::ofstream schema(schema_path);
    }

    // construct the comma-delimited value string to be inserted into the database schema
    std::string line;
    std::ifstream schema(schema_path);
    std::getline(schema, line); // get the top line, representing the schema of the table
    schema.close();

    line += arguments.add_cols.column_name + "_" + arguments.add_cols.type + ","; // add the new column to the line

    std::ofstream out_schema(schema_path); // write out the new schema to disk
    out_schema << line;

    out_schema.close();
    
    // In the case that we already have a populated table and then we want to add a new_col, just add in the default values for the new type for the existing rows
    // Above I defined the default values for each of the types

    std::ifstream table(table_path);
    std::string table_line;

    std::vector<std::string> in_memory_table;

    // Get all the existing lines of the table
    int line_num = 0;
    while(std::getline(table, table_line)){
        in_memory_table.push_back(table_line);
    }
    table.close();

    std::ofstream output_table(table_path);

    // go through the lines and append the default value to the end of the existing row in the table
    for(size_t i = 0; i < in_memory_table.size(); ++i){
        if(i == 0){
            output_table << in_memory_table[i] << std::endl;
            continue;
        }
        std::string curr_line = in_memory_table[i];
        if(arguments.add_cols.type == "string"){
            output_table << curr_line + str_default_value + ",\n";
        }
        else if(arguments.add_cols.type == "int"){
            output_table << curr_line + std::to_string(int_default_value) + ",\n";
        }
        else if(arguments.add_cols.type == "double"){
            output_table << curr_line + std::to_string(double_default_value) + ",\n";
        }
        else if(arguments.add_cols.type == "bool"){
            output_table << curr_line + std::to_string(bool_default_value) + ",\n";
        }
        else if(arguments.add_cols.type == "char"){
            output_table << curr_line + std::to_string(char_default_value) + ",\n";
        }
        else{
            exit_with_error(UNKNOWN_TYPE, arguments.add_cols.type);
        }
    }
    output_table.close();
    

    return;

}

/*
Implementation for the DELETE function
Arguments:
    - arguments: contains tbl_name to be deleted
*/
void delete_qmt(const cmd_args &arguments){
    std::cout << "Running delete implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number

    // Do error checking to ensure that the database and the tables are valid paths
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }

    if(!valid_table(arguments.deleted.tbl_name)){
        exit_with_error(INVALID_TABLENAME, arguments.deleted.tbl_name);
    }

    std::string table_path = db_path + "/" + arguments.deleted.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.deleted.tbl_name;

    if(!fs::exists(table_path)){
        exit_with_error(NULL_TABLE, arguments.deleted.tbl_name);
    }

    // If everything is valid, remove the schema and the data table from the database
    fs::remove(table_path);

    // If we are deleting a table that also has a schema, make sure to delete that schema as well since its table doesn't exist anymore
    if(fs::exists(schema_path)){
        fs::remove(schema_path);
    }

    return;
}

/*
Implementation for the COPY function
Arguments:
    - arguments: contains the table that we want to copy, and the table where want it to be copied to
*/
void copy_qmt(const cmd_args &arguments){
    std::cout << "Running delete implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number

    // Do error checking to ensure that the database and the tables are valid paths
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }

    if(!valid_table(arguments.copy.orig_table)){
        exit_with_error(INVALID_TABLENAME, arguments.copy.orig_table);
    }

    if(!valid_table(arguments.copy.copy_table)){
        exit_with_error(INVALID_TABLENAME, arguments.copy.orig_table);
    }

    std::string orig_table_path = db_path + "/" + arguments.copy.orig_table;
    std::string orig_schema_path = db_path + "/schemas/" + arguments.copy.orig_table;

    std::string copy_table_path = db_path + "/" + arguments.copy.orig_table;
    std::string copy_schema_path = db_path + "/schemas/" + arguments.copy.orig_table;

    if(!fs::exists(orig_table_path)){
        exit_with_error(NULL_TABLE, arguments.copy.orig_table);
    }

    if(!fs::exists(copy_table_path)){
        exit_with_error(NULL_TABLE, arguments.copy.orig_table);
    }

    // read in schema, so that we can compare and make sure that we can actually copy the tables over
    std::vector<std::string> original_tbl_schema_types = read_schema(orig_schema_path)[1];
    std::vector<std::string> copy_tbl_schema_types = read_schema(copy_schema_path)[1];

    if(original_tbl_schema_types.size() != copy_tbl_schema_types.size()){
        exit_with_error(DIFF_SCHEMAS, "");
    }

    // Go through the types and if there is a type conversion that cannot be made, then error out
    // Also copy values completely later, so that we can avoid having partially copied over tables and stuff
    for(size_t i = 0; i < original_tbl_schema_types.size(); ++i){
        if(copy_tbl_schema_types[i] == "string"){
            continue; // Anything can convert into a string
        }
        else if(copy_tbl_schema_types[i] == "char"){
            if(original_tbl_schema_types[i] != "char"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else if(copy_tbl_schema_types[i] == "bool"){
            if(original_tbl_schema_types[i] != "bool"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else if(copy_tbl_schema_types[i] == "int" || copy_tbl_schema_types[i] == "double"){
            if(original_tbl_schema_types[i] != "int" && original_tbl_schema_types[i] != "double"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else{
            exit_with_error(UNKNOWN_TYPE, copy_tbl_schema_types[i]);
        }
    }

    // empty constraints for now, but actually could be a good idea to have some constraints, like only copy select columns over
    std::vector<std::vector<std::string>> original_table = from_qmt(orig_table_path, std::vector<select_additional_args>{});

    write_table_to_disk(original_table, arguments.copy.copy_table);
    return;
}

/*
Implementation for the move function
Arguments:
    - arguments: contains the table that we want to move, and the table where want it to be moved to
*/
void move_qmt(const cmd_args &arguments){
    std::cout << "Running move implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number

    // Do error checking to ensure that the database and the tables are valid paths
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }

    if(!valid_table(arguments.move.source_table)){
        exit_with_error(INVALID_TABLENAME, arguments.move.source_table);
    }

    if(!valid_table(arguments.move.dest_table)){
        exit_with_error(INVALID_TABLENAME, arguments.move.dest_table);
    }

    std::string source_table_path = db_path + "/" + arguments.move.source_table;
    std::string source_schema_path = db_path + "/schemas/" + arguments.move.source_table;

    std::string dest_table_path = db_path + "/" + arguments.move.dest_table;
    std::string dest_schema_path = db_path + "/schemas/" + arguments.move.dest_table;

    if(!fs::exists(source_table_path)){
        exit_with_error(NULL_TABLE, arguments.move.source_table);
    }

    if(!fs::exists(dest_table_path)){
        exit_with_error(NULL_TABLE, arguments.move.dest_table);
    }

    // read in schema, so that we can compare and make sure that we can actually copy the tables over
    std::vector<std::string> source_tbl_schema_types = read_schema(source_schema_path)[1];
    std::vector<std::string> dest_tbl_schema_types = read_schema(dest_schema_path)[1];

    if(source_tbl_schema_types.size() != dest_tbl_schema_types.size()){
        exit_with_error(DIFF_SCHEMAS, "");
    }

    // Go through the types and if there is a type conversion that cannot be made, then error out
    // Also copy values completely later, so that we can avoid having partially copied over tables and stuff
    for(size_t i = 0; i < source_tbl_schema_types.size(); ++i){
        if(dest_tbl_schema_types[i] == "string"){
            continue; // Anything can convert into a string
        }
        else if(dest_tbl_schema_types[i] == "char"){
            if(source_tbl_schema_types[i] != "char"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else if(dest_tbl_schema_types[i] == "bool"){
            if(source_tbl_schema_types[i] != "bool"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else if(dest_tbl_schema_types[i] == "int" || dest_tbl_schema_types[i] == "double"){
            if(source_tbl_schema_types[i] != "int" && source_tbl_schema_types[i] != "double"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else{
            exit_with_error(UNKNOWN_TYPE, dest_tbl_schema_types[i]);
        }
    }

    // empty constraints for now, but actually could be a good idea to have some constraints, like only copy select columns over
    std::vector<std::vector<std::string>> original_table = from_qmt(source_table_path, std::vector<select_additional_args>{});
    std::vector<std::vector<std::string>> empty_table;

    // Going to need to make these atomic somehow
    write_table_to_disk(original_table, arguments.move.dest_table);
    write_table_to_disk(empty_table, arguments.move.source_table);

    return;
}
