#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>
#include <filesystem>

#include "interpreter.h"
#include "globals.h"

namespace fs = std::filesystem;

/*
Custom function in order to check whether or not the value read in is of the STRING type, formatted with, ex: "String".
Arguments:
    - value: Represents the read in value, always going to be read in as C++ string type, but could be of different type in the QMT table schema 
*/
bool check_string(const std::string &value){
    bool return_val = false;
    if(value.front() == '\"' && value.back() == '\"'){
        return_val = true;
    }
    return return_val;
}

/*
Custom function in order to check whether or not the value read in is of the INT type, formatted with, ex: 18.
Arguments:
    - value: Represents the read in value, always going to be read in as C++ string type, but could be of different type in the QMT table schema 
*/
bool check_int(const std::string &value){
    bool all_numeric = true;
    for(size_t i = 0; i < value.size(); ++i){
        if(!std::isdigit(value[i])){
            all_numeric = false;
            break;
        }
    }
    return all_numeric;
}

/*
Custom function in order to check whether or not the value read in is of the BOOL type, formatted with, ex: true.
Arguments:
    - value: Represents the read in value, always going to be read in as C++ string type, but could be of different type in the QMT table schema 
*/
bool check_bool(const std::string &value){
    if(value == "True" || value == "False"){
        return true;
    }
    return false;
}

/*
Custom function in order to check whether or not the value read in is of the CHAR type, formatted with, ex: 'c'.
Arguments:
    - value: Represents the read in value, always going to be read in as C++ string type, but could be of different type in the QMT table schema 
*/
bool check_char(const std::string &value){
    bool return_val = false;
    if(value.front() == '\'' && value.back() == '\''){
        return_val = true;
    }
    return return_val;
}

/*
Function in order to fill in the arguments for SELECT to be passed into the QMT command implementation.
Arguments:
    - command: A string of QMT lines/commands
    - args: The arguments to fill in, later to be passed into the implementation
*/
void fill_select_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Select function added to the function map, can fill out args for select statements" << std::endl;
    args.cmd = SELECT;

    bool table = false;

    // go through all the lines in the command, helps for future context like WHERE constraints etc.
    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        bool go_time = false;

        // skip if there is an empty command
        if(line.size() == 0){
            continue;
        }

        if(i > 0){
            args.select.additionals.push_back(line);
        }

        std::stringstream ss(line);
        std::string tmp;

        // Read in each white-space terminated string into the tmp string
        while(ss >> tmp){
            if(tmp == "SELECT"){ // set everything to go if we are executing the SELECT statement
                go_time = true;
                continue;
            }

            // If we are good to go, then go through the parenthesis, and for each comma delimited value assign it to it's appropriate field in the arguments
            if(go_time){
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.select.sel_columns.push_back(attr);
                        continue;
                    }

                    if(tmp[j] == ','){
                        if(!table){
                            args.select.tbl_name = attr;
                            table = true;
                        }
                        else{
                            args.select.sel_columns.push_back(attr);
                        }
                    }
                    else{
                        attr += tmp[j];
                    }
                }
            }
        }
    }
}

/*
Function in order to fill in the arguments for WHERE to be passed into the QMT command implementation.
Arguments:
    - command: A string of QMT lines/commands
    - args: The arguments to fill in, later to be passed into the implementation
*/
void fill_where_args(const std::string &command, select_additional_args &args){
    std::cout << "Where function added to the function map, can fill out args for where statements" << std::endl;
    std::stringstream ss(command);
    std::string tmp;

    bool go_time = false;
    int mode = 0;

    while(ss >> tmp){
        if(tmp == "WHERE"){
            go_time = true;
            continue;
        }

        // If we are good to go, then go through the parenthesis, and for each comma delimited value assign it to it's appropriate field in the arguments
        if(go_time){
            std::string attr;
            for(size_t j = 0; j < tmp.size(); ++j){
                if(tmp[j] == '('){
                    continue;
                }
                if(tmp[j] == ')'){
                    args.where.rhs_expression = attr;
                    break;
                }

                if(tmp[j] == ','){
                    if(mode == 0){
                        args.where.lhs_expression = attr;
                        mode = 1;
                    }
                    else if(mode == 1){
                        args.where.comparator = attr;
                    }
                }
                else{
                    attr += tmp[j];
                }
            }
        }
    }
}

/*
Function in order to fill in the arguments for INSERT to be passed into the QMT command implementation.
Arguments:
    - command: A string of QMT lines/commands
    - args: The arguments to fill in, later to be passed into the implementation
*/
void fill_insert_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Insert function added to the function map, can fill out args for insert statements" << std::endl;
    args.cmd = INSERT;

    int table = 0;
    int type = 0;
    int value_idx = 0;
    int char_idx = 6;

    std::vector<std::string> schema;
    std::vector<std::string> schema_types;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        bool go_time = false;

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;

        while(ss >> tmp){
            if(tmp == "INSERT"){
                go_time = true;
                continue;
            }

            // If we are good to go, then go through the parenthesis, and read the table to find the schema first
            if(go_time){
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){
                    char_idx++;

                    if(tmp[j] == '('){
                        continue;
                    }

                    if(tmp[j] == ','){
                        args.insert.tbl_name = attr;

                        if(!valid_table(args.insert.tbl_name)){
                            std::cout << "Invalid tablename (tablename is: " << args.insert.tbl_name << ") detected.\n";
                            exit(2);
                        }
                        
                        std::string schema_path = db_path + "/schemas/" + args.insert.tbl_name;

                        schema_types = read_schema(schema_path)[1];

                        table = 1;
                        attr.clear();
                        break;
                    }
                    else{
                        attr += tmp[j];
                    }

                }

                if(table == 1){ // used for early termination, since we just want to read in the table name here
                    char_idx++; // helps us get out of the comma
                    break;
                }
            }

        }

        // For each comma delimited value assign it to it's appropriate field in the arguments and ensure that it is of the proper type
        if(go_time){
            bool inside_string = false;
            std::string val;
            for(size_t j = char_idx; j < line.size(); ++j){

                if(line[j] == ' ' && !inside_string){
                    continue;
                }

                if(line[j] == '\"'){
                    inside_string = true;
                }

                if(line[j] == ',' || line[j] == ')'){
                    std::string actual_type = schema_types[value_idx];
                    if(!check_value_against_type[actual_type](val)){
                        std::cout << "Wrong type being, expected " << actual_type << ", inserted into the column!\n";
                        exit(7);
                    }

                    val = trim_string(val);
                    args.insert.values.push_back(val);
                    value_idx++;
                    val.clear();
                    inside_string = false;
                }
                else{
                    val += line[j];
                }
            }
        }

    }
}

/*
Function in order to fill in the arguments for CREATE to be passed into the QMT command implementation.
Arguments:
    - command: A string of QMT lines/commands
    - args: The arguments to fill in, later to be passed into the implementation
*/
void fill_create_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Create function added to the function map, can fill out args for create statements" << std::endl;
    args.cmd = CREATE;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        bool time_go = false;

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;
        std::string tbl_name;
        
        while(ss >> tmp){
            if(tmp == "CREATE"){
                time_go = true;
                continue;
            }
            
            // If we are good to go, then go through the parenthesis, and for each comma delimited value assign it to it's appropriate field in the arguments
            if(time_go){
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.create.tbl_name = tbl_name;
                        break;
                    }
                    tbl_name += tmp[j];
                }
            }   

        }
    }
}

/*
Function in order to fill in the arguments for ADDCOL to be passed into the QMT command implementation.
Arguments:
    - command: A string of QMT lines/commands
    - args: The arguments to fill in, later to be passed into the implementation
*/
void fill_add_col_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Addcol function added to the function map, can fill out args for addcol statements" << std::endl;
    args.cmd = ADD_COL;

    int mode = 0;

    for(size_t i = 0; i < command.size(); ++i){
        std::string line = command[i];
        if(line.size() == 0){
            continue;
        }

        bool go_time = false;

        std::stringstream ss(line);
        std::string tmp;

        while(ss >> tmp){
            if(tmp == "ADDCOL"){
                go_time = true;
                continue;
            }

            // If we are good to go, then go through the parenthesis, and for each comma delimited value assign it to it's appropriate field in the arguments
            if(go_time){
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    
                    if(tmp[j] == ')'){
                        args.add_cols.column_name = attr;
                        attr.clear();
                        break;
                    }

                    if(tmp[j] == ','){
                        if(mode == 0){
                            args.add_cols.tbl_name = attr;
                            mode = 1;
                        }
                        else if(mode == 1){
                            args.add_cols.type = attr;
                        }
                    }
                    else{
                        attr += tmp[j];
                    }
                    
                }
            }
            
        }
    }

}

/*
Function in order to fill in the arguments UPDATE to be passed into the QMT command implementation.
Arguments:
    - command: A string of QMT lines/commands
    - args: The arguments to fill in, later to be passed into the implementation
*/
void fill_update_args(const std::vector<std::string> &command, cmd_args &args){

}

/*
Function in order to fill in the arguments DELETE to be passed into the QMT command implementation.
Arguments:
    - command: A string of QMT lines/commands
    - args: The arguments to fill in, later to be passed into the implementation
*/
void fill_delete_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Delete function added to the function map, can fill out args for delete statements" << std::endl;
    args.cmd = DELETE;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        bool time_go = false;

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;
        std::string tbl_name;
        
        while(ss >> tmp){
            if(tmp == "DELETE"){
                time_go = true;
                continue;
            }
            
            // If we are good to go, then go through the parenthesis, and for each comma delimited value assign it to it's appropriate field in the arguments
            if(time_go){
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.deleted.tbl_name = tbl_name;
                        break;
                    }
                    tbl_name += tmp[j];
                }
            }
        }
    }
}

