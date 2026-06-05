#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "interpreter.h"
#include "comp.h"
#include "globals.h"
#include "fill_args.h"
#include "impls.h"

void init_function_map(){
    fill_in_cmd["select"] = fill_select_args;
    fill_in_cmd["insert"] = fill_insert_args;
    fill_in_cmd["create"] = fill_create_args;
    fill_in_cmd["addcol"] = fill_add_col_args;
    fill_in_cmd["delete"] = fill_delete_args;
    fill_in_cmd["update"] = fill_update_args;
    fill_in_cmd["alter"] = fill_alter_args;
    fill_in_additional_cmds["where"] = fill_where_args;
    fill_in_additional_cmds["from"] = fill_from_args;

    cmd_impls["select"] = select_qmt;
    cmd_impls["insert"] = insert_qmt;
    cmd_impls["create"] = create_qmt;
    cmd_impls["addcol"] = add_col_qmt;
    cmd_impls["delete"] = delete_qmt;
    cmd_impls["update"] = update_qmt;
    cmd_impls["alter"] = alter_qmt;

    check_value_against_type["string"] = check_string;
    check_value_against_type["int"] = check_int;
    check_value_against_type["bool"] = check_bool;
    check_value_against_type["char"] = check_char;

    comparators["equal"] = comp_equal;
    comparators["less"] = comp_less;
    comparators["greater"] = comp_greater;
    comparators["not_equal"] = comp_not_equal;
    comparators["less_than_or_equal"] = comp_less_than_or_equal_to;
    comparators["greater_than_or_equal"] = comp_greater_than_or_equal_to;
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
        if(in_memory_script[i].front() != '#'){
            command.push_back(in_memory_script[i]);
        }
    }
    
    // Have the constraint that each line represents a single of QMT code, can chain multiple lines into a command, like SQL
    for(int i = 0; i < command.size(); ){
        std::string cmd_type;
        cmd_args arguments;

        std::string line = command[i];

        if(line.size() == 0){
            executing_line_num++;
            i = executing_line_num - starting_line;
            continue;
        }

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
