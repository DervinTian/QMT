#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>
#include <unordered_map>
#include <filesystem>

#include "fill_args.h"
#include "globals.h"

namespace fs = std::filesystem;

std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, cmd_args&)>> fill_in_cmd;
std::unordered_map<std::string, std::function<void(const std::string&, select_additional_args&)>> fill_in_additional_cmds;
std::unordered_map<std::string, std::function<void(const cmd_args&)>> cmd_impls;
std::unordered_map<std::string, std::function<void(const select_additional_args&)>> additional_cmd_impls;
std::unordered_map<std::string, std::function<bool(const std::string&)>> check_value_against_type;
std::unordered_map<std::string, std::function<bool(const cmp_object&, const cmp_object&)>> comparators;
std::string db_path;
std::vector<std::string> in_memory_script;

int executing_line_num = 0;

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

std::vector<std::vector<std::string>> read_schema(const std::string &schema_path){

    std::vector<std::vector<std::string>> result;

    std::vector<std::string> schema;
    std::vector<std::string> schema_types;
    std::vector<std::string> schema_names;

    if(!valid_pathname(schema_path)){
        return result;
    }

    size_t pos = schema_path.rfind('/');
    std::string tbl_name = schema_path.substr(pos + 1);
    if(!fs::exists(schema_path)){
        std::cout << "Schema for " << tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::ifstream schema_file(schema_path);

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

        schema_names.push_back(first);
        schema_types.push_back(second);
    }

    result.push_back(schema_names);
    result.push_back(schema_types);

    return result;
}

std::vector<std::vector<std::string>> read_in_table(const std::string &table_path, const select_additional_args &constraints){

    // Structure of the in-memory table will be defined here
    // Made up of a vector of vectors
    // Outer vectors represent a type
    // Inner vectors hold the values for that type
    // Values align across all the vectors
    std::vector<std::vector<std::string>> result;

    bool where_constraint = false;

    if(constraints.where.tbl_name.size() > 0){
        where_constraint = true;
    }

    size_t pos = table_path.rfind('/');
    std::string tbl_name = table_path.substr(pos + 1);

    if(!fs::exists(table_path)){
        std::cout << "Table " << tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::ifstream table_file(table_path);

    std::string schema_path = db_path + "/schemas/" + tbl_name;

    std::vector<std::vector<std::string>> schema = read_schema(schema_path);
    std::vector<std::string>& column_names = schema[0];
    std::vector<std::string>& column_types = schema[1];
    int num_cols = (int)column_names.size();

    for(int i = 0; i < num_cols; ++i){
        std::vector<std::string> temp;
        result.push_back(temp);
    }

    int curr_attribute_counter = 0;
    int curr_col_idx = 0;
    std::string table_line;
    while(std::getline(table_file, table_line)){

        std::stringstream table_ss(table_line);
        std::string table_val;
        std::string curr_col;
        std::string curr_col_type;
        bool good_to_push = true;
        int skipped_checks = 0;
        cmp_object rhs_val;
        cmp_object lhs_val;
        std::string comparator;

        std::string cmp_type;
        std::vector<std::string> buffer; // a temporary spot to hold the column values

        while(std::getline(table_ss, table_val, ',')){

            curr_col = column_names[curr_attribute_counter];
            curr_col_type = column_types[curr_attribute_counter];
            buffer.push_back(table_val);
            // result[curr_attribute_counter].push_back(table_val);
            curr_attribute_counter = (curr_attribute_counter + 1) % num_cols;
            
            if(where_constraint){
                // for now just assuming that the left-hand-side will represent the column name, no other operations for now
                if(curr_col == constraints.where.lhs_expression){ // check to make sure that we are looking at the right column
                    if(check_string(constraints.where.rhs_expression)){
                        cmp_type = "string";
                        rhs_val.param_string = constraints.where.rhs_expression.substr(1, constraints.where.rhs_expression.size() - 2);
                        rhs_val.type = STRING;
                    }
                    else if(check_char(constraints.where.rhs_expression)){
                        cmp_type = "char";
                        rhs_val.param_char = constraints.where.rhs_expression[1];
                        rhs_val.type = CHAR;
                    }
                    else if(check_bool(constraints.where.rhs_expression)){
                        cmp_type = "bool";
                        if(constraints.where.rhs_expression == "true"){
                            rhs_val.param_bool = true;
                        }
                        else{
                            rhs_val.param_bool = false;
                        }
                        rhs_val.type = BOOL;
                    }
                    else if(check_int(constraints.where.rhs_expression)){
                        cmp_type = "int";
                        rhs_val.param_int = std::stoi(constraints.where.rhs_expression);
                        rhs_val.type = INT;
                    }
                    else{
                        std::cout << "Unknown type error: " << constraints.where.rhs_expression << " does not fall under any of the accepted types!\n";
                        exit(8);
                    }

                    if(curr_col_type == cmp_type){ // Check to make sure that the two objects are comparable
                        if(curr_col_type == "string"){
                            table_val = table_val.substr(1, table_val.size() - 2);
                            lhs_val.param_string = table_val;
                            lhs_val.type = STRING;
                        }
                        else if(curr_col_type == "char"){
                            lhs_val.param_char = table_val[1];
                            lhs_val.type = CHAR;
                        }
                        else if(curr_col_type == "bool"){
                            if(table_val == "true"){
                                lhs_val.param_bool = true;
                            }
                            else{
                                lhs_val.param_bool = false;
                            }
                            lhs_val.type = BOOL;
                        }
                        else if(curr_col_type == "int"){
                            lhs_val.param_int = std::stoi(table_val);
                            lhs_val.type = INT;
                        }
                        else{
                            std::cout << "Unknown type in the table? Gotta go check that one for this value: " << table_val << std::endl;
                            exit(67);
                        }

                        if(constraints.where.comparator == "EQUAL" || constraints.where.comparator == "="){
                            comparator = "equal";
                        }
                        else if(constraints.where.comparator == "GREATER" || constraints.where.comparator == ">"){
                            comparator = "greater";
                        }
                        else if(constraints.where.comparator == "LESS" || constraints.where.comparator == "<"){
                            comparator = "less";
                        }
                        else if(constraints.where.comparator == "NOT_EQUAL" || constraints.where.comparator == "!="){
                            comparator = "not_equal";
                        }
                        else if(constraints.where.comparator == "LESS_THAN_OR_EQUAL" || constraints.where.comparator == "<="){
                            comparator = "less_than_or_equal";
                        }
                        else if(constraints.where.comparator == "GREATER_THAN_OR_EQUAL" || constraints.where.comparator == ">="){
                            comparator = "greater_than_or_equal";
                        }
                        else{
                            std::cout << "Unknown comparator: " << constraints.where.comparator << std::endl;
                            exit(9);
                        }

                        good_to_push = comparators[comparator](lhs_val, rhs_val) && good_to_push;

                    }
                    else{
                        skipped_checks++;
                    }
                }
                else{
                    skipped_checks++;
                }
            }

        }

        if(good_to_push && skipped_checks > 0){
            for(size_t i = 0; i < buffer.size(); ++i){
                result[i].push_back(buffer[i]);
            }
        }

    }

    table_file.close();
    return result;

}

void display_in_memory_table(const std::vector<std::vector<std::string>> &table, const std::vector<std::vector<std::string>> &schema){

    std::vector<int> column_widths;

    // Going to use AI to help me do some pretty printing so just beware, I am not a cout wizard
    std::cout << std::left;
    int total_width = 0;
    for(size_t i = 0; i < schema[0].size(); ++i){
        std::string column_header = schema[0][i] + " (" + schema[1][i] + ")";
        int width = column_header.size() + 4;
        column_widths.push_back(width);
        total_width += width;
        std::cout << std::setw(width) << column_header;
    }
    std::cout << std::endl;

    std::cout << std::string(total_width + 5, '-') << "\n";

    for (size_t col = 0; col < table[0].size(); col++) {
        std::cout << std::left;
        for (size_t row = 0; row < table.size(); row++) {
            std::cout << std::setw(column_widths[row]) << table[row][col];
        }
        std::cout << "\n";
    }

}