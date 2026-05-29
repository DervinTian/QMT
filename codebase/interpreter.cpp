#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "interpreter.h"
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

bool run_interpreter(std::vector<std::string> command){
    
    // Have the constraint that each line represents a single of QMT code, can chain multiple lines into a command, like SQL
    for(size_t i = 0; i < command.size(); ++i){
        std::string cmd_type;
        cmd_args arguments;

        std::string line = command[i];

        if(command.size() == 0){
            std::cout << "Empty command!\n";
            return false;
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
    }

    return true;
}
