#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cctype>

#include "interpreter.h"

namespace fs = std::filesystem;

bool valid_pathname(std::string pathname){
    if(db_path[0] != '/'){
        return false;
    }
    return true;
}

bool valid_table(std::string table){
    auto it = table.find(" ");
    if(it != std::string::npos){
        return false;
    }

    it = table.find("/");
    if(it != std::string::npos){
        return false;
    }

    return true;
}

void select_qmt(cmd_args arguments){
    std::cout << "Running select implementation, can fill out semantics later\n";
}

void insert_qmt(cmd_args arguments){
    std::cout << "Running insert implementation, can fill out semantics later\n";

}

void create_qmt(cmd_args arguments){
    std::cout << "Running create implementation, can fill out semantics later\n";

    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.create.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.create.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.create.tbl_name;

    if(fs::exists(table_path)){
        std::cout << "Table " << arguments.create.tbl_name << " already exists!\n";
        exit(3);
    }

    std::ofstream tbl(table_path);

    for(size_t i = 0; i < arguments.create.attributes.size(); ++i){
        std::vector<std::string> attribute = arguments.create.attributes[i];
        for(size_t j = 0; j < attribute.size(); ++j){
            tbl << attribute[0] + "_" + attribute[1];
            if(j != attribute.size() - 1){
                tbl << ";";
            }
        }
    }

    tbl << std::endl;
}

void delete_qmt(cmd_args arguments){
    std::cout << "Running delete implementation, can fill out semantics later\n";
}