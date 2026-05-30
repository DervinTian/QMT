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

bool check_string(std::string &value){
    bool return_val = false;
    if(value.front() == '\"' && value.back() == '\"'){
        return_val = true;
    }
    return return_val;
}

bool check_int(std::string &value){
    bool all_numeric = true;
    for(size_t i = 0; i < value.size(); ++i){
        if(!std::isdigit(value[i])){
            all_numeric = false;
            break;
        }
    }
    return all_numeric;
}

bool check_bool(std::string &value){
    if(value == "True" || value == "False"){
        return true;
    }
    return false;
}

bool check_char(std::string &value){
    bool return_val = false;
    if(value.front() == '\'' && value.back() == '\''){
        return_val = true;
    }
    return return_val;
}

void fill_select_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Select function added to the function map, can fill out args for select statements" << std::endl;
    args.cmd = SELECT;
}

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

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;

        while(ss >> tmp){
            if(tmp == "INSERT"){
                continue;
            }

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
                    std::ifstream schema_file(schema_path);

                    if(!fs::exists(schema_path)){
                        std::cout << "Schema for " << args.insert.tbl_name << " doesn't exist!\n";
                        exit(3);
                    }

                    std::string schema_line;
                    while(std::getline(schema_file, schema_line));
                    schema_file.close();

                    std::stringstream schema_ss(schema_line);
                    std::string token;
                    
                    while(std::getline(schema_ss, token, ',')){
                        if(token.size() > 0){
                            schema.push_back(token);
                        }
                    }
                    
                    for(size_t k = 0; k < schema.size(); ++k){
                        std::stringstream temp_ss(schema[k]);

                        std::string first, second;

                        std::getline(temp_ss, first, '_');
                        std::getline(temp_ss, second, '_');

                        schema_types.push_back(second);
                    }

                    table = 1;
                    attr.clear();
                    break;
                }
                else{
                    attr += tmp[j];
                }

            }

            if(table == 1){
                char_idx++; // helps us get out of the comma
                break;
            }
        }

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

void fill_create_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Create function added to the function map, can fill out args for create statements" << std::endl;
    args.cmd = CREATE;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;
        std::string tbl_name;
        
        while(ss >> tmp){
            if(tmp == "CREATE"){
                continue;
            }
            
            for(size_t j = 0; j < tmp.size(); ++j){
                if(tmp[j] == '('){
                    continue;
                }
                if(tmp[j] == ')'){
                    break;
                }
                tbl_name += tmp[j];
            }
        }

        args.create.tbl_name = tbl_name;
    }
}

void fill_add_col_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Addcol function added to the function map, can fill out args for addcol statements" << std::endl;
    args.cmd = ADD_COL;

    int mode = 0;

    for(size_t i = 0; i < command.size(); ++i){
        std::string line = command[i];
        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;

        while(ss >> tmp){
            if(tmp == "ADDCOL"){
                continue;
            }

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
                        continue;
                    }
                    else if(mode == 1){
                        args.add_cols.type = attr;
                        mode = 2;
                        continue;
                    }
                    attr.clear();
                }
                else{
                    attr += tmp[j];
                }
                
            }
        }
    }

}

void fill_delete_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Delete function added to the function map, can fill out args for delete statements" << std::endl;
    args.cmd = DELETE;
}

