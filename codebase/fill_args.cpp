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

    bool go_cols = false;
    bool go_vals = false;

    bool col_state = true;

    bool col_start = false;
    bool col_finished = false;

    bool val_start = false;
    bool val_finished = false;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;
        while(ss >> tmp){

            auto it = tmp.find('(');
            if(it != std::string::npos){
                if(col_state){
                    col_start = true;
                }
                else{
                    val_start = true;
                }
                continue;
            }

            it = tmp.find(')');
            if(it != std::string::npos){
                if(col_state){
                    col_finished = true;
                }
                else{
                    val_finished = true;
                }
                continue;
            }

            line_content.push_back(tmp);
        }

        if(!go_cols){
            for (char& c : line_content[0]) {
                c = std::tolower(static_cast<unsigned char>(c));
            }

            if(line_content[0] == "insert"){
                args.insert.tbl_name = line_content[1];
                go_cols = true;
            }
        }
        else{
            for (char& c : line_content[0]) {
                c = std::tolower(static_cast<unsigned char>(c));
            }

            if(line_content[0] == "values"){
                go_vals = true;
            }

            if(go_vals){
                if(val_start && !val_finished){
                    
                }
            }
        }
    }
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

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;
        while(ss >> tmp){

            auto it = tmp.find('(');
            if(it != std::string::npos){
                start = true;
                continue;
            }

            it = tmp.find(')');
            if(it != std::string::npos){
                finished = true;
                continue;
            }

            line_content.push_back(tmp);
        }

        // for(int i = 0; i < line_content.size(); ++i){
        //     std::cout << "$: " << line_content[i] << std::endl;
        // }
        // std::cout << std::endl;

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
            if(start && !finished){
                args.create.attributes.push_back(line_content); // right now this assumes like the pretty formatting, but will fail with other formatting
            }

            if(start && finished){
                break;
            }
        }
    }

    // for(int i = 0; i < args.create.attributes.size(); ++i){
    //     for(int j = 0; j < args.create.attributes[i].size(); ++j){
    //         std::cout << "#: " << args.create.attributes[i][j] << " ";
    //     }
    //     std::cout << std::endl;
    // }
}

void fill_delete_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Delete function added to the function map, can fill out args for delete statements" << std::endl;
    args.cmd = DELETE;
}

