#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "interpreter.h"

void fill_select_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Select function added to the function map, can fill out args for select statements" << std::endl;
    args.cmd = SELECT;
}

void fill_insert_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Insert function added to the function map, can fill out args for insert statements" << std::endl;
    args.cmd = INSERT;
}

void fill_create_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Create function added to the function map, can fill out args for create statements" << std::endl;
    args.cmd = CREATE;

    bool time_go = false;
    bool start = false; // keep track of what goes into the parenthesis for the columns, true when first paren is reached
    bool finished = false; // true when second paren is reached

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];
        std::string tmp;
        for(size_t j = 0; j < line.size(); ++j){
            if(line[j] != ' '){
                tmp += line[j];
                if(line[j] == '('){
                    start = true;
                }
                if(line[j] == ')'){
                    finished = true;
                }
            }
            else{
                line_content.push_back(tmp);
                tmp.clear();
            }
        }

        if(!time_go){
            for (char& c : line_content[0]) {
                c = std::tolower(static_cast<unsigned char>(c));
            }

            if(line_content[0] == "create"){
                args.create.tbl_name = line_content[1];
                time_go = true;
            }
        }
        else{
            if(start){
                args.create.attributes.push_back(line_content); // right now this assumes like the pretty formatting, but will fail with other formatting
            }

            if(finished){
                break;
            }
        }
    }
}

void fill_delete_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Delete function added to the function map, can fill out args for delete statements" << std::endl;
    args.cmd = DELETE;
}

