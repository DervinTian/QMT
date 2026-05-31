#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "interpreter.h"
#include "globals.h"
#include "fill_args.h"
#include "impls.h"

void init_function_map(){
    fill_in_cmd["select"] = fill_select_args;
    fill_in_cmd["insert"] = fill_insert_args;
    fill_in_cmd["create"] = fill_create_args;
    fill_in_cmd["addcol"] = fill_add_col_args;
    fill_in_cmd["delete"] = fill_delete_args;
}

void init_impl_map(){
    cmd_impls["select"] = select_qmt;
    cmd_impls["insert"] = insert_qmt;
    cmd_impls["create"] = create_qmt;
    cmd_impls["addcol"] = add_col_qmt;
    cmd_impls["delete"] = delete_qmt;
}

void init_check_map(){
    check_value_against_type["string"] = check_string;
    check_value_against_type["int"] = check_int;
    check_value_against_type["bool"] = check_bool;
    check_value_against_type["char"] = check_char;
}

bool end_statement(std::string line){
    std::stringstream ss(line);
    std::string tmp;
    while(ss >> tmp){
        for(size_t i = 0; i < tmp.size(); ++i){
            if(tmp[i] == ';'){
                return true;
            }
        }
    }
    return false;
}

bool run_interpreter(int starting_line, int end_line){

    std::vector<std::string> command;
    for(int i = starting_line; i < end_line + 1; ++i){
        command.push_back(in_memory_script[i]);
    }
    
    // Have the constraint that each line represents a single of QMT code, can chain multiple lines into a command, like SQL
    for(int i = 0; i < command.size(); ){
        std::string cmd_type;
        cmd_args arguments;

        std::string line = command[i];

        if(command.size() == 0){
            std::cout << "Empty command!\n";
            return false;
        }

        if(line.back() == ';'){
            line = line.substr(0, line.size() - 1);
        }

        std::stringstream ss(line);
        std::string temp;
        ss >> cmd_type;

        for (char& c : cmd_type) {
            c = std::tolower(static_cast<unsigned char>(c));
        }

        auto it = fill_in_cmd.find(cmd_type);

        if (it == fill_in_cmd.end()) {
            std::cout << "Command " << cmd_type << " doesn't exist!\n";
            return false;
        }

        // read in the proper arguments to execute the statement
        fill_in_cmd[cmd_type](command, arguments);

        // actually run the implemntation
        cmd_impls[cmd_type](arguments);

        i = executing_line_num - starting_line;
    }

    return true;
}
