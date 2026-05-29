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

    std::vector<std::string> command_content;
    
    bool col_start = false;
    bool col_finished = false;

    bool val_start = false;
    bool val_finished = false;

    for(size_t i = 0; i < command.size(); ++i){
        std::string line = command[i];

        std::string tmp;
        std::stringstream ss(line);

        while(ss >> tmp){
            command_content.push_back(tmp);
        }

    }

    for(size_t i = 1; i < command_content.size(); ++i){
        std::string word = command_content[i];
        if(i == 1){
            args.insert.tbl_name = word;
            continue;
        }

        auto it = word.find('(');
        if(it != std::string::npos && !col_start){
            col_start = true;
        }

        if(col_start && !col_finished){
            std::string tmp;
            if(col_start && !col_finished){
                for(size_t i1 = it; i1 < word.size(); ++i1){
                    tmp += word[i1];
                }
            }
        }

        it = word.find(')');
        if(it != std::string::npos && !col_start){
            col_finished = true;
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
            
            for(size_t i = 0; i < tmp.size(); ++i){
                if(tmp[i] == '('){
                    continue;
                }
                if(tmp[i] == ')'){
                    break;
                }
                tbl_name += tmp[i];
            }
        }

        args.create.tbl_name = tbl_name;
    }
}

void fill_add_col_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Addcol function added to the function map, can fill out args for addcol statements" << std::endl;
    args.cmd = ADD_COL;

    for(size_t i = 0; i < command.size(); ++i){
        std::string line = command[i];
        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;

        bool table = false;
        bool type = false;
        bool name = false;
        int mode = 0;

        while(ss >> tmp){
            if(tmp == "ADDCOL"){
                continue;
            }

            std::string attr;
            for(size_t i = 0; i < tmp.size(); ++i){
                if(tmp[i] == '('){
                    continue;
                }
                
                if(tmp[i] == ')'){
                    args.add_cols.column_name = attr;
                    break;
                }

                if(tmp[i] == ','){
                    if(mode == 0){
                        args.add_cols.tbl_name = attr;
                        mode = 1;
                    }
                    else if(mode == 1){
                        args.add_cols.type = attr;
                        mode = 2;
                    }
                }
                else{
                    attr += tmp[i];
                }
                
            }
        }
    }

}

void fill_delete_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Delete function added to the function map, can fill out args for delete statements" << std::endl;
    args.cmd = DELETE;
}

