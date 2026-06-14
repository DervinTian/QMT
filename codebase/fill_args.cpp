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

    if(value == "-"){
        return false;
    }

    for(size_t i = 0; i < value.size(); ++i){
        if(i == 0 && value[i] == '-'){
            continue;
        }
        if(!std::isdigit(value[i])){
            all_numeric = false;
            break;
        }
    }
    return all_numeric;
}

/*
Custom function in order to check whether or not the value read in is of the INT type, formatted with, ex: 18.
Arguments:
    - value: Represents the read in value, always going to be read in as C++ string type, but could be of different type in the QMT table schema 
*/
bool check_double(const std::string &value){
    try{
        double value_double = std::stod(value);
    }
    catch(const std::invalid_argument& e){
        return false;
    }
    return true;
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
        bool from_time = false;

        // skip if there is an empty command
        if(line.size() == 0){
            continue;
        }

        if(i > 1){
            args.select.additionals.push_back(line);
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;

        // Read in each white-space terminated string into the tmp string
        while(ss >> tmp){
            if(tmp == "SELECT"){ // set everything to go if we are executing the SELECT statement
                go_time = true;
                continue;
            }
            if(tmp == "FROM"){
                from_time = true;
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
                        go_time = false;
                        continue;
                    }

                    if(tmp[j] == ','){
                        args.select.sel_columns.push_back(attr);
                    }
                    else{
                        attr += tmp[j];
                    }
                }
            }

            if(from_time){
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.select.tbl_name = attr;
                        from_time = false;
                        continue;
                    }

                    attr += tmp[j];
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
void fill_where_args(const std::vector<std::string> &command, select_additional_args &args){
    std::cout << "Where function added to the function map, can fill out args for where statements" << std::endl;

    for(size_t i = 0; i < command.size(); ++i){
        std::string line = command[i];
        std::stringstream ss(line);
        std::string tmp;

        bool go_time = false;
        int mode = 0; // mode 0 is for reading in lhs expression, 1 is for comparator, and after that is the mode can only be for the rhs expression
        bool math_mode = false;
        bool finished_reading_math_expression = false;

        while(ss >> tmp){
            if(tmp == "WHERE"){
                go_time = true;
                args.curr_type = WHERE;
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
                        if(math_mode && !finished_reading_math_expression && mode == 0){
                            args.where.left_math.expression_pieces.push_back(attr);
                            finished_reading_math_expression = true;
                            math_mode = false;
                            mode = 1;
                        }
                        else if(math_mode && mode == 0){
                            args.where.left_math.variables.push_back(attr);
                            mode = 1;
                            math_mode = false;
                        }
                        else if(math_mode && !finished_reading_math_expression && mode == 2){
                            args.where.right_math.expression_pieces.push_back(attr);
                            finished_reading_math_expression = true;
                            math_mode = false;
                        }
                        else if(math_mode && mode == 2){
                            args.where.right_math.variables.push_back(attr);
                            math_mode = false;
                        }
                        else{
                            args.where.rhs_expression = attr;
                            go_time = false;
                        }
                        break;
                    }

                    if(tmp[j] == ','){
                        if(math_mode && finished_reading_math_expression && mode == 0){
                            args.where.left_math.variables.push_back(attr);
                        }
                        else if(mode == 0 && !math_mode){
                            args.where.lhs_expression = attr;
                            mode = 1;
                        }
                        else if(mode == 1 && !math_mode){
                            args.where.comparator = attr;
                            mode = 2;
                        }
                        else if(math_mode && finished_reading_math_expression && mode == 2){
                            args.where.right_math.variables.push_back(attr);
                        }

                        if(!finished_reading_math_expression && mode == 0){
                            finished_reading_math_expression = true;
                            args.where.left_math.expression_pieces.push_back(attr);
                        }
                        else if(!finished_reading_math_expression && mode == 2){
                            finished_reading_math_expression = true;
                            args.where.right_math.expression_pieces.push_back(attr);
                        }
                    }
                    else{
                        attr += tmp[j];
                    }
                }
                if(math_mode && !finished_reading_math_expression && mode == 0){
                    args.where.left_math.expression_pieces.push_back(attr);
                }
                else if(math_mode && !finished_reading_math_expression && mode == 2){
                    args.where.right_math.expression_pieces.push_back(attr);
                }
                if(attr == "MATH"){
                    math_mode = true;
                    finished_reading_math_expression = false;
                    args.where.lhs_expression = "";
                    args.where.rhs_expression = "";
                    args.where.type = MATH;
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

    int type = 0;
    int value_idx = 0;

    std::vector<std::string> schema;
    std::vector<std::string> schema_types;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        bool go_time = false;
        bool values_time = false;

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
            if(tmp == "VALUES"){
                values_time = true;
                continue;
            }

            // If we are good to go, then go through the parenthesis, and read the table to find the schema first
            if(go_time){
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){

                    if(tmp[j] == '('){
                        continue;
                    }

                    if(tmp[j] == ')'){
                        args.insert.tbl_name = attr;

                        if(!valid_table(args.insert.tbl_name)){
                            std::cout << "Invalid tablename (tablename is: " << args.insert.tbl_name << ") detected.\n";
                            exit(2);
                        }
                        
                        std::string schema_path = db_path + "/schemas/" + args.insert.tbl_name;

                        schema_types = read_schema(schema_path)[1];

                        attr.clear();
                        break;
                    }
                    else{
                        attr += tmp[j];
                    }

                }
            }

            if(values_time){
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){

                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ',' || tmp[j] == ')'){
                        std::string actual_type = schema_types[value_idx];
                        if(!check_value_against_type[actual_type](attr)){
                            std::cout << "Wrong type being, expected " << actual_type << ", inserted into the column!\n";
                            exit(7);
                        }

                        attr = trim_string(attr);
                        args.insert.values.push_back(attr);
                        value_idx++;

                        if(tmp[j] == ')'){
                            values_time = false;
                        }
                    }
                    else{
                        attr += tmp[j];
                    }
                }
                executing_line_num++; // For now since VALUES isn't like an operation, just move the next line here. Maybe will change in the future 
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

    bool additionals_time = false;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        bool time_go = false;

        if(line.size() == 0){
            continue;
        }

        if(additionals_time){
            args.create.additionals.push_back(line);
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
                        time_go = false;
                        additionals_time = true;
                        break;
                    }
                    tbl_name += tmp[j];
                }
            }   
        }
    }
}

/*
Function in order to fill in the arguments for FROM to be passed into the QMT command implementation.
Arguments:
    - command: A string of QMT lines/commands
    - args: The arguments to fill in, later to be passed into the implementation
*/
void fill_from_args(const std::vector<std::string> &command, select_additional_args &args){
    for(size_t i = 0; i < command.size(); ++i){
        std::string line = command[i];

        bool time_go = false;
        std::stringstream ss(line);
        std::string tmp;
        std::string data_source;
        
        while(ss >> tmp){
            if(tmp == "FROM"){
                time_go = true;
                args.curr_type = FROM;
                continue;
            }

            if(tmp == "NO_HEADER"){
                args.from.header = 0;
            }
            else if(tmp == "HEADER"){
                args.from.header = 1;
            }
            
            // If we are good to go, then go through the parenthesis, and for each comma delimited value assign it to it's appropriate field in the arguments
            if(time_go){
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.from.data_source = data_source;
                        time_go = false;
                        break;
                    }
                    data_source += tmp[j];
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
                        go_time = false;
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
    std::cout << "Update function added to the function map, can fill out args for update statements" << std::endl;
    args.cmd = UPDATE;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i]; 
        
        bool autobots = false;
        bool set = false;
        bool additional = false;

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;
        std::string tbl_name;
        std::pair<std::string, std::string> set_value_pair;

        while(ss >> tmp){
            if(tmp == "UPDATE"){
                autobots = true;
                continue;
            }
            else if(tmp == "SET"){
                set = true;
                continue;
            }
            else if(tmp != "SET" && !set && !autobots){
                if(!additional){    
                    args.update.additionals.push_back(line);
                    additional = true;
                }

                if(tmp.find('\n') != std::string::npos){
                    additional = false;
                }
                
                continue;
            }

            if(autobots){
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.update.tbl_name = tbl_name;
                        autobots = false;
                        break;
                    }
                    tbl_name += tmp[j];
                }
            }

            if(set){
                int mode = 0;
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        set_value_pair.second = attr;
                        args.update.set_values.push_back(set_value_pair);
                        set = false;
                        break;
                    }
                    if(tmp[j] == ','){
                        set_value_pair.first = attr;
                    }
                    attr += tmp[j];
                }
            }
        }
    }
}

/*
Function in order to fill in the arguments ALTER to be passed into the QMT command implementation.
Arguments:
    - command: A string of QMT lines/commands
    - args: The arguments to fill in, later to be passed into the implementation
*/
void fill_alter_args(const std::vector<std::string> &command, cmd_args &args){
    std::cout << "Alter function added to the function map, can fill out args for alter statements" << std::endl;
    args.cmd = ALTER;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        bool roll_out = false;

        bool rename = false;
        bool modify = false;

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;
        std::string tbl_name;

        while(ss >> tmp){
            if(tmp == "ALTER"){
                roll_out = true;
                continue;
            }
            else if(tmp == "MODIFY"){
                modify = true;
                args.alter.modify = 1;
                continue;
            }
            else if(tmp == "RENAME"){
                rename = true;
                args.alter.rename = 1;
                continue;
            }

            if(roll_out){
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.alter.tbl_name = tbl_name;
                        roll_out = false;
                        break;
                    }
                    tbl_name += tmp[j];
                }
            } 
            else if(modify){
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.alter.new_column_type = attr;
                        modify = false;
                        break;
                    }
                    if(tmp[j] == ','){
                        args.alter.column_name = attr;
                    }
                    attr += tmp[j];
                }
            } 
            else if(rename){
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.alter.new_column_name = attr;
                        rename = false;
                        break;
                    }
                    if(tmp[j] == ','){
                        args.alter.column_name = attr;
                    }
                    attr += tmp[j];
                }
            }
        }
    }
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
                        time_go = false;
                        break;
                    }
                    tbl_name += tmp[j];
                }
            }
        }
    }
}

void fill_join_args(const std::vector<std::string> &command, select_additional_args &args){
    std::cout << "Join function added to the function map, can fill out args for join statements" << std::endl;

    bool time_go = false;
    bool on_time = false;

    for(size_t i = 0; i < command.size(); ++i){
        std::vector<std::string> line_content;
        std::string line = command[i];

        int mode = 0;

        if(line.size() == 0){
            continue;
        }

        std::stringstream ss(line);
        std::string tmp;
        std::string tbl_name;

        while(ss >> tmp){
            if(tmp == "ON"){
                continue;
            }
            else if(tmp == "JOIN"){
                time_go = true;
                ss >> tmp;
                if(tmp == "LEFT"){
                    args.join.join_type = LEFT;
                    continue;
                }
                else if(tmp == "INNER"){
                    args.join.join_type = INNER;
                    continue;
                }
            }

            if(on_time){
                std::string attr;
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.join.on.rhs_expression = attr;
                        time_go = false;
                        on_time = true;
                        break;
                    }

                    if(tmp[j] == ','){
                        if(mode == 0){
                            args.join.on.lhs_expression = attr;
                            mode = 1;
                        }
                        else if(mode == 1){
                            args.join.on.comparator = attr;
                            mode = 2;
                        }
                    }
                    attr += tmp[j];

                }
            }

            // If we are good to go, then go through the parenthesis, and for each comma delimited value assign it to it's appropriate field in the arguments
            if(time_go){
                for(size_t j = 0; j < tmp.size(); ++j){
                    if(tmp[j] == '('){
                        continue;
                    }
                    if(tmp[j] == ')'){
                        args.join.right_tbl = tbl_name;
                        time_go = false;
                        on_time = true;
                        break;
                    }
                    tbl_name += tmp[j];
                }
            } 
            
        }
    }
}

