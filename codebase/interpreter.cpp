#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>
#include <algorithm>

#include "interpreter.h"

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
    for(size_t i = 0; i < command.size(); ++i){
        std::string line = command[i];

        cmd_args arguments;
        std::stringstream ss(line);
        std::string temp;
        std::string cmd_type;

        ss >> cmd_type;

        for (char& c : cmd_type) {
            c = std::tolower(static_cast<unsigned char>(c));
        }

        if(cmd_type == "select"){
            arguments.cmd = SELECT;
            select_qmt(arguments);
        }
        else if(cmd_type == "insert"){
            arguments.cmd = INSERT;
            insert_qmt(arguments);
        }
        else if(cmd_type == "delete"){
            arguments.cmd = DELETE;
            delete_qmt(arguments);
        }
        else{
            std::cout << "Unknown cmd\n";
            return false;
        }

    }

    return true;
}

void select_qmt(cmd_args arguments){
    std::cout << "Running select statement\n";
}

void insert_qmt(cmd_args arguments){
    std::cout << "Running insert statement\n";
}

void delete_qmt(cmd_args arguments){
    std::cout << "Running delete statement\n";
}   
