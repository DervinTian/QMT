#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <bitset>
#include <cassert>

#include "fill_args.h"
#include "impls.h"
#include "globals.h"

namespace fs = std::filesystem;

// Declare global variables
std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, cmd_args&)>> fill_in_cmd;
std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, select_additional_args&)>> fill_in_additional_cmds;
std::unordered_map<std::string, std::function<void(const cmd_args&)>> cmd_impls;
std::unordered_map<std::string, std::function<void(const select_additional_args&)>> additional_cmd_impls;
std::unordered_map<std::string, std::function<bool(const std::string&)>> check_value_against_type;
std::unordered_map<std::string, std::function<bool(const cmp_object&, const cmp_object&)>> comparators;
std::string db_path;
std::vector<std::string> in_memory_script;
std::unordered_map<std::string, std::string> alias_to_attr_mapping;
intermediate_results_buffer intermediate_results;

int int_default_value = 0;
double double_default_value = 0.0;
std::string str_default_value = "NULL";
char char_default_value = '0';
bool bool_default_value = false;

int executing_line_num = 0;
uint32_t PAGE_SIZE = 4096;
uint64_t DISK_SIZE = 1048576; // Just set it to 1MB for now

/*
Function to exit the code with a certain error code and message
Arguments:
    - pathname: pathname to be checked
*/
void exit_with_error(int error_code, std::string message){
    if(error_code == NULL_QMT){
        std::cout << "No QMT script provided to run!\n";
        exit(1);
    }
    else if(error_code == NULL_INPUT_FILE){
        std::cout << "Input file" << message << " failed to open!\n";
        exit(2);
    }
    else if(error_code == LINE_ERROR){
        std::cout << "Error in command ending at line " << executing_line_num << std::endl;
        exit(3);
    }
    else if(error_code == FS_DATABASE){
        std::cout << "Unable to create database directory!\n";
        exit(4);
    }
    else if(error_code == UNKNOWN_CMD){
        std::cout << "Unknown command: " << message << "\n";
        exit(5);
    }
    else if(error_code == INVALID_DATABASE_NAME){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(6);
    }
    else if(error_code == INVALID_TABLENAME){
        std::cout << "Invalid tablename (tablename is: " << message << ") detected.\n";
        exit(7);
    }
    else if(error_code == NULL_TABLE){
        std::cout << "Table " << message << " doesn't exist!\n";
        exit(8);
    }
    else if(error_code == SCHEMA_EXISTS){
        std::cout << "Schema for table " << message << " already exists!" << std::endl;
        exit(9);
    }
    else if(error_code == EMPTY_TABLE){
        std::cout << "Empty table, cannot update records on an empty table!\n";
        exit(10);
    }
    else if(error_code == NULL_SCHEMA){
        std::cout << "Schema for Table " << message << " doesn't exist!\n";
        exit(11);
    }
    else if(error_code == UNKNOWN_TYPE){
        std::cout << message << " is not a recognized type!\n";
        exit(12);
    }
    else if(error_code == TYPE_MISMATCH){
        std::cout << "Wrong type being, expected " << message << ", inserted into the column!\n";
        exit(13);
    }
    else if(error_code == UNKNWON_SORT){
        std::cout << "Unknown sort direction for ORDER: " << message << std::endl;
        exit(14);
    }
    else if(error_code == INVALID_CMP){
        std::cout << "Values for the left hand side and the right hand side are not comparable" << std::endl;
        exit(15);
    }
    else if(error_code == UNKNOWN_CMP){
        std::cout << "Unknown comparator: " << message << std::endl;
        exit(16);
    }
    else if(error_code == DIFF_SCHEMAS){
        std::cout << "Schemas differ across the two tables!\n";
        exit(17);
    }
}

/*
Function to convert a string into it's lower-case conuter-part
Arguments:
    - pathname: pathname to be checked
*/
std::string convert_to_lower_case(std::string &str, bool copy){

    if(!copy){
        for (char& c : str) {
            c = std::tolower(static_cast<unsigned char>(c));
        }
        return str;
    }
    else{
        std::string str_copy = str;
        for (char& c : str_copy) {
            c = std::tolower(static_cast<unsigned char>(c));
        }
        return str_copy;
    }

}

/*
Function to check if the pathname is a valid one to look within the database.
Arguments:
    - pathname: pathname to be checked
*/
bool valid_pathname(std::string pathname){
    if(db_path[0] != '/'){
        return false;
    }
    return true;
}

/*
Function to check if the table name is a valid one to look within the database.
Arguments:
    - table: table name to be checked
*/
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

/*
Function to check if the two schemas are compatible to transfer data across
Arguments:
    - table: value, the value to be trimmed
*/
void check_compatible_schemas(const std::vector<std::string> &source_tbl_schema_types, const std::vector<std::string> &dest_tbl_schema_types){
    // Go through the types and if there is a type conversion that cannot be made, then error out
    // Also copy values completely later, so that we can avoid having partially copied over tables and stuff
    
    if(source_tbl_schema_types.size() != dest_tbl_schema_types.size()){
        exit_with_error(DIFF_SCHEMAS, "");
    }

    for(size_t i = 0; i < source_tbl_schema_types.size(); ++i){
        if(dest_tbl_schema_types[i] == "string"){
            continue; // Anything can convert into a string
        }
        else if(dest_tbl_schema_types[i] == "char"){
            if(source_tbl_schema_types[i] != "char"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else if(dest_tbl_schema_types[i] == "bool"){
            if(source_tbl_schema_types[i] != "bool"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else if(dest_tbl_schema_types[i] == "int" || dest_tbl_schema_types[i] == "double"){
            if(source_tbl_schema_types[i] != "int" && source_tbl_schema_types[i] != "double"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else{
            exit_with_error(UNKNOWN_TYPE, dest_tbl_schema_types[i]);
        }
    }
}

/*
Function to trim the string so that we just have the values inside of it, not like the " ", or the ' ' surrounding it.
Arguments:
    - table: value, the value to be trimmed
*/
std::string trim_string(std::string value){
    if(check_string(value) || check_char(value)){
        return value.substr(1, value.size() - 2);
    }
    return value;
}

/*
Function to split a variable like table_name.column_name into (table_name, column_name)
Arguments:
    - table: value, the value to be trimmed
*/
std::pair<std::string, std::string> split_table_column(std::string value){
    std::stringstream ss(value);
    std::string first, second;

    // delimit on the '_' character
    std::getline(ss, first, '.');
    std::getline(ss, second, '.');

    return std::make_pair(first, second);
}

/*
Helper function ton convert a line of string into a valid schema
Arguments:
    - schema_string: the contents of hte schema like this name_type,name2_type2,...

Return:
    - 2D vector containig the results in vectors containing two inner vectors of strings
    - Index 0 is the column names
    - Index 1 is the column types
*/
std::vector<std::vector<std::string>> vectorize_schema(const std::string &schema_string){
    std::vector<std::vector<std::string>> result;
    std::vector<std::string> schema;
    std::vector<std::string> schema_types;
    std::vector<std::string> schema_names;

    std::stringstream schema_ss(schema_string);
    std::string token;
    
    // go through the schema, and delimit by the ',' character
    while(std::getline(schema_ss, token, ',')){
        if(token.size() > 0){
            schema.push_back(token);
        }
    }
    
    // go through the tokens that we just read in, like ColumnName_ColumnType
    for(size_t k = 0; k < schema.size(); ++k){
        std::stringstream temp_ss(schema[k]);

        std::string first, second;

        // delimit on the '_' character
        std::getline(temp_ss, first, '_');
        std::getline(temp_ss, second, '_');

        // add that to the names and the types of the table
        schema_names.push_back(first);
        schema_types.push_back(second);
    }

    // append those to our result:
    // [0]: column names
    // [1]: column_types
    result.push_back(schema_names);
    result.push_back(schema_types);

    return result;
}

/*
Function to read in the schema into memory.
Arguments:
    - schema_path: path of the schema to be read in

Return:
    - 2D vector containig the results in vectors containing two inner vectors of strings
    - Index 0 is the column names
    - Index 1 is the column types
*/
std::vector<std::vector<std::string>> read_schema(const std::string &schema_path){

    std::vector<std::vector<std::string>> result;

    std::vector<std::string> schema;
    std::vector<std::string> schema_types;
    std::vector<std::string> schema_names;

    // check to make sure that the schema actually exists
    if(!valid_pathname(schema_path)){
        return result;
    }
    
    // find the last slash to find the table name for the schema
    size_t pos = schema_path.rfind('/');
    std::string tbl_name = schema_path.substr(pos + 1);
    if(!fs::exists(schema_path)){
        exit_with_error(NULL_SCHEMA, tbl_name);
    }

    std::ifstream schema_file(schema_path);

    std::string schema_line;
    std::getline(schema_file, schema_line); // get the table schema, column names and column types
    schema_file.close();

    result = vectorize_schema(schema_line);

    return result;
}

// Enum to represent the different ordering of operators according to PEMDAS convention
enum operator_priority {
    SUB_ADD,
    MULT_DIVIDE,
};

// struct to represent the operation "+", "-", "*", "/" as an in-memory object
struct operation{
    std::string op_symbol;
    operator_priority priority;
    int position;
};

// custom comparator for a priority queue
struct operation_comparator{
    bool operator()(operation &lhs, operation &rhs){
        if(lhs.priority == rhs.priority){
            return lhs.position > rhs.position;
        }
        else{
            return lhs.priority < rhs.priority;
        }
    }
}; 

/*
Function to evaluate the expression recursively
Arguments:

    - expression: represents the expression, broken down into individual components.
        
Recursively evaluates the expression going down to lowest level and then evaluating up to the other levels.
*/
void recursive_evaluate(std::vector<std::string> &expression){

    // Go through the expression pieces, and we can process everything down after that
    for(size_t i = 0; i < expression.size(); ++i){
        if(expression[i] == "("){ // if we encounter a parenthesis, then we need to go down the parenthesis to evaluate the value within the parenthesis
            int count_extra_parens = 0;
            int start_of_expression = i;
            int end_of_expression = i;
            bool end_expression = false;
            for(int j = i + 1; j < expression.size(); ++j){ // go from the '(' and find it's matching ')'
                if(expression[j] == "("){
                    count_extra_parens++; // if we found another '(', then make sure that we find an extra ')'
                }
                if(expression[j] == ")"){
                    if(count_extra_parens == 0){ // if we found a ')' and went passed all the offsets, then we found the matching corresponding parenthesis to the first one
                        end_expression = true;
                        end_of_expression = j; // mark that as the end of the expression
                        break;
                    }
                    count_extra_parens--;
                }
            }

            if(end_expression){ // now that we finished the end of the expression, we can evaluate the stuff inside the parenthesis
                std::vector<std::string> nested_expression;
                for(int k = start_of_expression + 1; k < end_of_expression; ++k){
                    nested_expression.push_back(expression[k]); // now we want to find the value inside of the expression
                }

                recursive_evaluate(nested_expression); // call the function recursively to go down to the lowest level of the parenthesis and evaluate each term
                
                std::vector<std::string> expression_copy;
                for(int q = 0; q < expression.size(); ++q){ // go through the original expression, and replace the quantity term represented by the () with it's evaluated value
                    if(q == start_of_expression){
                        expression_copy.push_back(nested_expression[0]); // the evaluated expression result is located at index 0 of the expression
                        q = end_of_expression + 1;
                        if(q >= expression.size()){
                            break;
                        }
                    }
                    expression_copy.push_back(expression[q]);
                }
                expression = expression_copy; // now update the original expression with the newly evaluated term
            }
            else{
                std::cout << "Wut\n";
            }

        }
    }

    // Now that we are at a level that has no more parenthesis, we can solve it mathematically
    // First step is to get the order of operations across the + - * /, using PEMDAS
    std::unordered_map<int, operation> operation_positions;
    for(size_t i = 0; i < expression.size(); ++i){
        // keep track of each operation
        if(expression[i] == "+"){
            operation op;
            op.op_symbol = "+";
            op.position = i;
            op.priority = SUB_ADD; // addition and subtraction have same priority, otherwise go left to right
            operation_positions[i] = op;
        }
        else if(expression[i] == "-"){
            operation op;
            op.op_symbol = "-";
            op.position = i;
            op.priority = SUB_ADD; // addition and subtraction have same priority, otherwise go left to right
            operation_positions[i] = op;
        }
        else if(expression[i] == "*"){
            operation op;
            op.op_symbol = "*";
            op.position = i;
            op.priority = MULT_DIVIDE; // multiplication and division have same priority, otherwise go left to right
            operation_positions[i] = op;
        }
        else if(expression[i] == "/"){
            operation op;
            op.op_symbol = "/";
            op.position = i;
            op.priority = MULT_DIVIDE; // multiplication and division have same priority, otherwise go left to right
            operation_positions[i] = op;
        }
    }

    // have a pq, with custom comparators so that we can keep track of the order of the terms that we need to solve with
    std::priority_queue<operation, std::vector<operation>, operation_comparator> operation_pq;

    for(auto &pair : operation_positions){
        int curr_operator_position = pair.first;
        operation curr_operation = pair.second;

        // populate the priority queue
        operation_pq.push(curr_operation);
    }

    // go through the operations until the end of the expression is done
    while(operation_pq.size() > 0){
        operation top_operation = operation_pq.top(); // get the first operation to be done
        operation_pq.pop(); // pop it off because we are done with it

        std::vector<std::string> expression_copy; // have an expression copy, since we will be updating the expression as we go
        std::priority_queue<operation, std::vector<operation>, operation_comparator> operation_pq_copy; // have a pq copy, since we will be updaying the operation's position
        
        // the left_pos and the right_pos represent the two terms that the operation is meant to operate on
        int left_pos = top_operation.position - 1;
        int right_pos = top_operation.position + 1;

        double op_result = 0;
        // do the math operations in order to solve each term one by one
        if(top_operation.op_symbol == "+"){
            op_result = std::stod(expression[left_pos]) + std::stod(expression[right_pos]);
        }
        else if(top_operation.op_symbol == "-"){
            op_result = std::stod(expression[left_pos]) - std::stod(expression[right_pos]);
        }
        else if(top_operation.op_symbol == "*"){
            op_result = std::stod(expression[left_pos]) * std::stod(expression[right_pos]);
        }
        else if(top_operation.op_symbol == "/"){
            op_result = std::stod(expression[left_pos]) / std::stod(expression[right_pos]);
        }

        // Now we update the expression with the result of the operation, replacing something like 3 * 4 with 12
        for(size_t i = 0; i < expression.size(); ++i){
            if(i == left_pos){
                expression_copy.push_back(std::to_string(op_result));
                i = right_pos;
            }
            else{
                expression_copy.push_back(expression[i]);
            }
        }

        expression = expression_copy; // update the original expression

        // go through each operation, and now update their positions, since we just eliminated a couple of terms
        while(operation_pq.size() > 0){
            operation top = operation_pq.top();
            operation_pq.pop();
            if(top.position < left_pos + 1){
                operation_pq_copy.push(top);
                continue;
            }

            top.position -= 2;
            operation_pq_copy.push(top);
        }

        operation_pq = operation_pq_copy; // update the original pq to be the updated one
    }

}

/*
Function to evaluate the expression for the MATH keyword
Arguments:

    - constraint: contains the details of the where statement itself
        - each term in the expression, containing integers, decimals, operators, parenthesis
    
    - table_val: represents the table variable value that corresponds to "?" in the expression

Return:
    - double: the result of the expression as a decimal value to be used in future comparisons
*/
double math_qmt(const std::vector<std::string> &expression_pieces){

    std::vector<std::string> expression_copy = expression_pieces; // make a copy cuz we might want the original still
 
    recursive_evaluate(expression_copy); // run the solving function
    double result = std::stod(expression_copy[0]);

    return result; // return the result as a double, so it can be cast to int if needed
}

/*
Function to check whether or not the table value passes the where constraint.
Arguments:
    - curr_col: the current column name for the table value
    - curr_col_type: the current column type for the table value
    - table_val: the current table value to be checked

    - constraint: contains the details of the where statement itself
        - lhs_expression: left side of the where
        - rhs_expression: right side of the where
        - comparator: how to compare the two

Return:
    - cmp_return_type: has three possible values, TRUE if passes, FALSE if fails, UNKNOWN if values were not comparabale
*/
cmp_return_type where_qmt(select_additional_args &constraint, std::unordered_map<std::string, cmp_object> &expression_variables){
    std::cout << "Running where implementation\n";
    bool good_to_push = false;

    // The types of the values to be compared
    std::string cmp_type;
    // The comparison that will be used
    std::string comparator;
    // The two objects to be compared in the end
    cmp_object lhs_val;
    cmp_object rhs_val;

    // if the constraint is of math type, then do run through the expression and solve for the terms
    if(constraint.where.type == MATH){
        double expression_result = 0; // master result

        // find the variables that will be used
        std::vector<int> variable_idx;
        int curr_variable_idx = 0;
        for(size_t i = 0; i < constraint.where.left_math.expression_pieces.size(); ++i){
            if(constraint.where.left_math.expression_pieces[i] == "?"){
                variable_idx.push_back(i);
            }
        }

        assert(variable_idx.size() == constraint.where.left_math.variables.size());

        // if there are variables, then fill them out, otherwise just use the hard-coded values
        cmp_object_type lhs_type;
        if(constraint.where.left_math.variables.size() > 0){
            cmp_object curr_variable;
            // go through the variables and fill them in with the variable values that were read in and passed in here
            for(size_t i = 0; i < constraint.where.left_math.variables.size(); ++i){
                std::string variable_name = constraint.where.left_math.variables[i];
                curr_variable = expression_variables[variable_name];
                lhs_type = curr_variable.type;

                // do it different for INTs and DOUBLEs
                if(lhs_type == INT){
                    constraint.where.left_math.expression_pieces[variable_idx[curr_variable_idx]] = std::to_string(curr_variable.param_int);
                }
                else{
                    constraint.where.left_math.expression_pieces[variable_idx[curr_variable_idx]] = std::to_string(curr_variable.param_double);
                }
                curr_variable_idx++;
            }

            // find the result and store it into the lhs_val
            expression_result = math_qmt(constraint.where.left_math.expression_pieces);
            if(lhs_type == INT){
                lhs_val.param_int = expression_result;
                lhs_val.type = INT;
            }
            else if(lhs_type == DOUBLE){
                lhs_val.param_double = expression_result;
                lhs_val.type = DOUBLE;
            }
        }
        else{ // if there were no variables, then lhs_expression is just a hard_coded value, so just read it in
            if(check_int(constraint.where.lhs_expression)){
                lhs_val.param_int = std::stoi(constraint.where.lhs_expression);
                lhs_val.type = INT;
            }
            else if(check_double(constraint.where.lhs_expression)){
                lhs_val.param_double = std::stod(constraint.where.lhs_expression);
                lhs_val.type = DOUBLE;
            }
            else{
                exit_with_error(UNKNOWN_TYPE, constraint.where.lhs_expression);
            }
        }

        // Repeat the same process for the right hand side
        variable_idx.clear();
        curr_variable_idx = 0;
        for(size_t i = 0; i < constraint.where.right_math.expression_pieces.size(); ++i){
            if(constraint.where.right_math.expression_pieces[i] == "?"){
                variable_idx.push_back(i);
            }
        }

        assert(variable_idx.size() == constraint.where.right_math.variables.size());

        cmp_object_type rhs_type;
        if(constraint.where.right_math.variables.size() > 0){
            cmp_object curr_variable;
            for(size_t i = 0; i < constraint.where.right_math.variables.size(); ++i){
                std::string variable_name = constraint.where.right_math.variables[i];
                curr_variable = expression_variables[variable_name];
                rhs_type = curr_variable.type;

                if(rhs_type == INT){
                    constraint.where.right_math.expression_pieces[variable_idx[curr_variable_idx]] = std::to_string(curr_variable.param_int);
                }
                else{
                    constraint.where.right_math.expression_pieces[variable_idx[curr_variable_idx]] = std::to_string(curr_variable.param_double);
                }
                curr_variable_idx++;
            }

            expression_result = math_qmt(constraint.where.right_math.expression_pieces);
            if(rhs_type == INT){
                rhs_val.param_int = expression_result;
                rhs_val.type = INT;
            }
            else if(rhs_type == DOUBLE){
                rhs_val.param_double = expression_result;
                rhs_val.type = DOUBLE;
            }
        }
        else{
            if(check_int(constraint.where.rhs_expression)){
                rhs_val.param_int = std::stoi(constraint.where.rhs_expression);
                rhs_val.type = INT;
            }
            else if(check_double(constraint.where.rhs_expression)){
                rhs_val.param_double = std::stod(constraint.where.rhs_expression);
                rhs_val.type = DOUBLE;
            }
            else{
                exit_with_error(UNKNOWN_TYPE, constraint.where.rhs_expression);
            }
        }

        if(lhs_type != rhs_type){
            if(lhs_type == DOUBLE || rhs_type == INT){
                rhs_val.param_double = rhs_val.param_int;
            }
            else if(lhs_type == INT || rhs_type == DOUBLE){
                lhs_val.param_double = lhs_val.param_int;
            }
        }

    }
    else{ // if not math, then just default to a regular comparison
        bool lhs_is_variable = true; // find if the lhs is a column value, or if it is just a hard-coded value
        if(expression_variables.find(constraint.where.lhs_expression) == expression_variables.end()){
            lhs_is_variable = false;
        }

        bool rhs_is_variable = true; // find if the rhs is a column value, or if it is just a hard-coded value
        if(expression_variables.find(constraint.where.rhs_expression) == expression_variables.end()){
            rhs_is_variable = false;
        }

        if(lhs_is_variable){ // read in lhs_val as a variable if it is one
            lhs_val = expression_variables[constraint.where.lhs_expression];
        }
        else{ // otherwise, just convert the hard-coded value into a cmp_object
            if(check_string(constraint.where.lhs_expression)){
                lhs_val.param_string = constraint.where.lhs_expression.substr(1, constraint.where.lhs_expression.size() - 2);
                lhs_val.type = STRING;
            }
            else if(check_char(constraint.where.lhs_expression)){
                lhs_val.param_char = constraint.where.lhs_expression.substr(1, constraint.where.lhs_expression.size() - 2)[0];
                lhs_val.type = CHAR;
            }
            else if(check_bool(constraint.where.lhs_expression)){
                if(constraint.where.lhs_expression == "true"){
                    lhs_val.param_bool = true;
                }
                else{
                    lhs_val.param_bool = false;
                }
                lhs_val.type = BOOL;
            }
            else if(check_int(constraint.where.lhs_expression)){
                lhs_val.param_int = std::stoi(constraint.where.lhs_expression);
                lhs_val.type = INT;
            }
            else if(check_double(constraint.where.lhs_expression)){
                lhs_val.param_double = std::stod(constraint.where.lhs_expression);
                lhs_val.type = DOUBLE;
            }
            else{
                exit_with_error(UNKNOWN_TYPE, constraint.where.lhs_expression);
            }
        }

        // Do the exact same thing for the right-hand side 
        if(rhs_is_variable){
            rhs_val = expression_variables[constraint.where.rhs_expression];
        }
        else{
            if(check_string(constraint.where.rhs_expression)){
                rhs_val.param_string = constraint.where.rhs_expression.substr(1, constraint.where.rhs_expression.size() - 2);
                rhs_val.type = STRING;
            }
            else if(check_char(constraint.where.rhs_expression)){
                rhs_val.param_char = constraint.where.rhs_expression.substr(1, constraint.where.rhs_expression.size() - 2)[0];
                rhs_val.type = CHAR;
            }
            else if(check_bool(constraint.where.rhs_expression)){
                if(constraint.where.rhs_expression == "true"){
                    rhs_val.param_bool = true;
                }
                else{
                    rhs_val.param_bool = false;
                }
                rhs_val.type = BOOL;
            }
            else if(check_int(constraint.where.rhs_expression)){
                rhs_val.param_int = std::stoi(constraint.where.rhs_expression);
                rhs_val.type = INT;
            }
            else if(check_double(constraint.where.rhs_expression)){
                rhs_val.param_double = std::stod(constraint.where.rhs_expression);
                rhs_val.type = DOUBLE;
            }
            else{
                exit_with_error(UNKNOWN_TYPE, constraint.where.rhs_expression);
            }
        }

        // Make sure that the types for the lhs and the rhs are comparable
        if(lhs_val.type != rhs_val.type){
            exit_with_error(INVALID_CMP, "");
        }
    }

    // Go through the comparators and update the comparator to be what was passed in
    if(constraint.where.comparator == "EQUAL" || constraint.where.comparator == "="){
        comparator = "equal";
    }
    else if(constraint.where.comparator == "GREATER" || constraint.where.comparator == ">"){
        comparator = "greater";
    }
    else if(constraint.where.comparator == "LESS" || constraint.where.comparator == "<"){
        comparator = "less";
    }
    else if(constraint.where.comparator == "NOT_EQUAL" || constraint.where.comparator == "!="){
        comparator = "not_equal";
    }
    else if(constraint.where.comparator == "LESS_THAN_OR_EQUAL" || constraint.where.comparator == "<="){
        comparator = "less_than_or_equal";
    }
    else if(constraint.where.comparator == "GREATER_THAN_OR_EQUAL" || constraint.where.comparator == ">="){
        comparator = "greater_than_or_equal";
    }
    else{
        exit_with_error(UNKNOWN_CMD, constraint.where.comparator);
    }

    // run the comparison between the two values filled in with the comparator
    good_to_push = comparators[comparator](lhs_val, rhs_val);

    // If it is good, then return true, otherwise return false
    if(good_to_push){
        return TRUE;
    }
    else{
        return FALSE;
    }

}

/*
Function for the FROM keyword to load the table into memory with the given where constraints.
Arguments:
    - table_path: the path of the table to read in
    - constraints: the constraint(s) to be placed on the table

Return:
    - 2D vector containing the table structure
    - Is a vector that contains vectors of column values
*/
std::vector<std::vector<std::string>> from_qmt(const std::string &table_path, const std::vector<select_additional_args> &constraints, const select_args &column_constrants){

    // Structure of the in-memory table will be defined here
    // Made up of a vector of vectors
    // Outer vectors represent a type
    // Inner vectors hold the values for that type
    // Values align across all the vectors
    std::vector<std::vector<std::string>> result;
    
    size_t pos = table_path.rfind('/'); // find the last / to find the table name
    std::string tbl_name;
    if(pos != std::string::npos){

        // get the last attribute after the /, supposed to represent the table name (may have .csv after it)
        tbl_name = table_path.substr(pos + 1);
        std::string file_extension;
        if(tbl_name.size() > 4){
            // find the last 4 characters and store them
            for(size_t idx = tbl_name.size() - 4; idx < tbl_name.size(); ++idx){
                file_extension += tbl_name[idx];
            }

            // if they are .csv, then it is a csv file so we should trim those off of the table name
            if(file_extension == ".csv"){
                tbl_name = tbl_name.substr(0, tbl_name.size() - 4);
            }
        }

        if(!fs::exists(table_path)){
            exit_with_error(NULL_TABLE, tbl_name);
        }
    }
    else{
        tbl_name = table_path.substr(0, table_path.size() - 4);
    }

    std::ifstream table_file(table_path);
    std::string schema_path = db_path + "/schemas/" + tbl_name;

    // Read in the schema
    std::vector<std::vector<std::string>> schema = read_schema(schema_path);
    std::vector<std::string>& column_names = schema[0];
    std::vector<std::string>& column_types = schema[1];
    int num_cols = (int)column_names.size();

    // initialize variables to be used later
    int curr_attribute_counter = 0; // used into index into column_names and column_types
    std::string table_line;

    int line_num = 0;

    bool select_everyting = false;
    std::unordered_set<std::string> columns_that_we_want;
    if(column_constrants.table_columns.find(tbl_name) == column_constrants.table_columns.end()){
        select_everyting = true;
    }
    else{
        columns_that_we_want = column_constrants.table_columns.at(tbl_name);
        for (const std::string& x : columns_that_we_want) {
            if(x == "*"){
                select_everyting = true;

                for(size_t i = 0; i < column_names.size(); ++i){
                    result.push_back(std::vector<std::string>{});
                }
                break;
            }
            result.push_back(std::vector<std::string>{});
        }
    }

    // Go line by line and get each row of data
    while(std::getline(table_file, table_line)){

        if(line_num == 0){
            line_num++;
            continue;
        }

        // initialize variables to be used
        std::stringstream table_ss(table_line);
        std::string table_val;
        std::string curr_col;
        std::string curr_col_type;

        cmp_return_type good_to_push;
        bool push = true;
        
        int skipped_checks = 0;
        cmp_object rhs_val;
        cmp_object lhs_val;
        std::string comparator;

        std::string cmp_type;
        std::vector<std::string> buffer; // a temporary spot to hold the column values
        std::unordered_map<std::string, cmp_object> expression_variables; // holds all the variables that are going to be used
        bool where_constraint = false;

        // Parse through each value and update the expression variables and stuff for that row
        while(std::getline(table_ss, table_val, ',')){

            // find the column names and the types for the line
            curr_col = column_names[curr_attribute_counter];
            curr_col_type = column_types[curr_attribute_counter];
            if(select_everyting){
                buffer.push_back(table_val);
            }
            else{
                if(columns_that_we_want.find(column_names[curr_attribute_counter]) != columns_that_we_want.end()){
                    buffer.push_back(table_val);
                }
            }
            curr_attribute_counter = (curr_attribute_counter + 1) % num_cols;

            // go through the constraints, maybe more than one line like two WHERE clauses ANDED together
            // and fill in the variables
            for(size_t x = 0; x < constraints.size(); ++x){
                select_additional_args constraint = constraints[x]; // update to deal with all the constraints, not just one at a time

                // If math, fill it in for math things, otherwise just default to a regular compare between like the value type
                if(constraint.where.type == MATH){
                    // Want to fill in the variables, so see if it is in the left
                    for(size_t i = 0; i < constraint.where.left_math.variable_pairs[tbl_name].size(); ++i){
                        if(constraint.where.left_math.variable_pairs[tbl_name][i] == curr_col){
                            cmp_object variable;
                            if(curr_col_type == "int"){
                                variable.param_int = std::stoi(table_val); // convert it into an int
                                variable.type = INT;
                            }
                            else if(curr_col_type == "double"){
                                variable.param_double = std::stod(table_val); // convert it into a double
                                variable.type = DOUBLE;
                            }
                            expression_variables[tbl_name + "." + curr_col] = variable;
                            continue;
                        }
                    }

                    // Want to fill in the variables, so see if it is in the right
                    for(size_t i = 0; i < constraint.where.right_math.variable_pairs[tbl_name].size(); ++i){
                        if(constraint.where.right_math.variable_pairs[tbl_name][i] == curr_col){
                            cmp_object variable;
                            if(curr_col_type == "int"){
                                variable.param_int = std::stoi(table_val); // convert it into an int
                                variable.type = INT;
                            }
                            else if(curr_col_type == "double"){
                                variable.param_double = std::stod(table_val); // convert it into a double
                                variable.type = DOUBLE;
                            }
                            expression_variables[tbl_name + "." + curr_col] = variable;
                            continue;
                        }
                    }
                }
                else{
                    // otherwise, check to see if the lhs_expression or rhs_expression contains the column, then just add it to the variables map
                    std::pair<std::string, std::string> left_variable_table_col = split_table_column(constraint.where.lhs_expression);
                    std::string left_var_col = left_variable_table_col.second;
                    std::pair<std::string, std::string> right_variable_table_col = split_table_column(constraint.where.rhs_expression);
                    std::string right_var_col = right_variable_table_col.second;
                    if(curr_col == left_var_col || curr_col == right_var_col){
                        cmp_object variable;
                        // go through the expression and convert it into it's appropriate type
                        if(curr_col_type == "string"){
                            variable.param_string = table_val;
                            variable.type = STRING;
                        }
                        else if(curr_col_type == "bool"){
                            if(table_val == "true"){
                                variable.param_bool = true;
                            }
                            else{
                                variable.param_bool = false;
                            }
                            variable.type = BOOL;
                        }
                        else if(curr_col_type == "char"){
                            variable.param_char = table_val[0];
                            variable.type = CHAR;
                        }
                        else if(curr_col_type == "int"){
                            variable.param_int = std::stoi(table_val);
                            variable.type = INT;
                        }
                        else if(curr_col_type == "double"){
                            variable.param_double = std::stod(table_val);
                            variable.type = DOUBLE;
                        }
                        expression_variables[tbl_name + "." + curr_col] = variable; // map that variable to it's corresponding object
                    }
                }

                // idempodent operation to set whether or not the where operation is even there, since it will have a tbl_name for sure
                if(constraint.where.run_where){
                    where_constraint = true;
                }
                
            }

        }

        // if we are supposed to run a where statement
        if(where_constraint){
            for(size_t x = 0; x < constraints.size(); ++x){
                select_additional_args constraint = constraints[x]; 
                good_to_push = where_qmt(constraint, expression_variables); // run the where implementation
                
                // if the where worked returned false, then don't add it to the result set
                if(good_to_push == FALSE){
                    push = false;
                }
            }
        }

        // if we are good to go, then add the record to our result set
        if(push){
            for(size_t i = 0; i < buffer.size(); ++i){
                result[i].push_back(buffer[i]);
            }
        }

        line_num++;

    }

    table_file.close();
    return result;

}

/*
Helper function to turn a vector of strings like val1,val2,val3, ... into an in-memory table
Arguments:
    - arguments: contains left tbl_name, right tbl_name, the type of join, and the constraint on which to join

Returns:
    - an in-memory table that represents the result of the join, where only rows that are in both tables that are joined together are kept, no null values at all
*/
std::vector<std::vector<std::string>> vectorize_csv(const std::vector<std::string> &csv_format_table){
    
    std::vector<std::vector<std::string>> result_table;
    int num_attributes = 0;
    if(csv_format_table.size() > 0){
        std::stringstream ss(csv_format_table[0]);
        std::string token;
        while(std::getline(ss, token, ',')){
            num_attributes++;
        }
    }
    else{
        return result_table;
    }
    for(int j = 0; j < num_attributes; ++j){
        result_table.push_back(std::vector<std::string>{});
    }

    int attr_idx = 0;
    for(size_t j = 0; j < csv_format_table.size(); ++j){
        std::stringstream ss(csv_format_table[j]);
        std::string token;
        while(std::getline(ss, token, ',')){
            result_table[attr_idx].push_back(token);
            attr_idx = (attr_idx + 1) % num_attributes;
        }
    }

    return result_table;
}

/*
Helper function to run an inner join
Arguments:
    - arguments: contains left tbl_name, right tbl_name, the type of join, and the constraint on which to join

Returns:
    - an in-memory table that represents the result of the join, where only rows that are in both tables that are joined together are kept, no null values at all
*/
std::vector<std::vector<std::string>> inner_join_qmt(const select_args &select_constraint, const select_additional_args &constraint, std::vector<std::vector<std::string>> &left_tbl, std::vector<std::vector<std::string>> &left_tbl_schema, std::string &join_result_schema){
    std::vector<std::vector<std::string>> join_result;
    std::unordered_map<std::string, std::vector<std::string>> parse_hash;
    // Create a mapping to find which attribute goes to which column index
    std::unordered_map<std::string, int> attr_to_idx_mapping;
    for(size_t j = 0; j < left_tbl_schema[0].size(); ++j){
        join_result_schema += left_tbl_schema[0][j] + "_" + left_tbl_schema[1][j] + ",";
        attr_to_idx_mapping[left_tbl_schema[0][j]] = j;
    }

    int num_attributes = left_tbl.size();
    // Go through the table and write out the table out to disk
    if(num_attributes > 0){
        for (size_t col = 0; col < left_tbl[0].size(); col++) {
            std::string hash_key;
            std::string tbl_line;
            for (size_t row = 0; row < left_tbl.size(); row++) {
                if(row == attr_to_idx_mapping[constraint.join.on.lhs_expression]){
                    hash_key = left_tbl[row][col];
                }
                tbl_line += left_tbl[row][col] + ",";
            }

            parse_hash[hash_key].push_back(tbl_line);
        }
    }

    std::string right_tbl_path = db_path + "/" + constraint.join.right_tbl;
    std::string right_tbl_schema_path = db_path + "/schemas/" + constraint.join.right_tbl;

    // Now that we have hashed the left table, we need to go to the second set

    std::vector<std::vector<std::string>> right_tbl = from_qmt(right_tbl_path, std::vector<select_additional_args>{}, select_constraint);
    
    // In the future, if the select statement only wants like some from the right table, we can update that, for now assume the second set is SELECT (*), like select all columns
    std::vector<std::vector<std::string>> right_tbl_schema = read_schema(right_tbl_schema_path);

    std::unordered_set<std::string> right_tbl_column_names = select_constraint.table_columns.at(constraint.join.right_tbl);
    std::vector<std::vector<std::string>> new_right_tbl_schema;
    new_right_tbl_schema.push_back(std::vector<std::string>{});
    new_right_tbl_schema.push_back(std::vector<std::string>{});

    for(size_t x = 0; x < right_tbl_schema[0].size(); ++x){
        if(right_tbl_column_names.find(right_tbl_schema[0][x]) != right_tbl_column_names.end()){
            new_right_tbl_schema[0].push_back(right_tbl_schema[0][x]);
            new_right_tbl_schema[1].push_back(right_tbl_schema[1][x]);
        }
    }
    
    attr_to_idx_mapping.clear();
    for(size_t j = 0; j < new_right_tbl_schema[0].size(); ++j){
        join_result_schema += new_right_tbl_schema[0][j] + "_" + new_right_tbl_schema[1][j] + ",";
        attr_to_idx_mapping[new_right_tbl_schema[0][j]] = j;
    }

    num_attributes = right_tbl.size();

    std::vector<std::string> csv_format_join_results;

    if(num_attributes > 0){
        for (size_t col = 0; col < right_tbl[0].size(); col++) {
            std::string hash_key;
            std::string tbl_line;
            for (size_t row = 0; row < right_tbl.size(); row++) {
                
                if(row == attr_to_idx_mapping[constraint.join.on.rhs_expression]){
                    hash_key = right_tbl[row][col];
                }
                tbl_line += right_tbl[row][col] + ",";
            }

            auto it = parse_hash.find(hash_key);
            if(it != parse_hash.end()){
                for(size_t j = 0; j < parse_hash[hash_key].size(); ++j){
                    csv_format_join_results.push_back(parse_hash[hash_key][j] + tbl_line);
                }
            }
        }
    }

    join_result = vectorize_csv(csv_format_join_results);

    std::cout << "done joining\n";
    return join_result;

}

/*
Helper function to run a left join
Arguments:
    - arguments: contains left tbl_name, right tbl_name, the type of join, and the constraint on which to join

Returns:
    - an in-memory table that represents the result of the join, where only rows that are in both tables that are joined together are kept, no null values at all
*/
std::vector<std::vector<std::string>> left_join_qmt(const select_args &select_constraint, const select_additional_args &constraint, std::vector<std::vector<std::string>> &left_tbl, std::vector<std::vector<std::string>> &left_tbl_schema, std::string &join_result_schema){
    std::vector<std::vector<std::string>> join_result;
    
    std::unordered_map<std::string, std::vector<std::string>> parse_hash;

    // Create a mapping to find which attribute goes to which column index
    std::unordered_map<std::string, int> attr_to_idx_mapping;
    for(size_t j = 0; j < left_tbl_schema[0].size(); ++j){
        join_result_schema += left_tbl_schema[0][j] + "_" + left_tbl_schema[1][j] + ",";
        attr_to_idx_mapping[left_tbl_schema[0][j]] = j;
    }

    int num_attributes = left_tbl.size();
    // Go through the table and write out the table out to disk
    if(num_attributes > 0){
        for (size_t col = 0; col < left_tbl[0].size(); col++) {
            std::string hash_key;
            std::string tbl_line;
            for (size_t row = 0; row < left_tbl.size(); row++) {
                if(row == attr_to_idx_mapping[constraint.join.on.lhs_expression]){
                    hash_key = left_tbl[row][col];
                }
                tbl_line += left_tbl[row][col] + ",";
            }

            parse_hash[hash_key].push_back(tbl_line);
        }
    }

    std::string right_tbl_path = db_path + "/" + constraint.join.right_tbl;
    std::string right_tbl_schema_path = db_path + "/schemas/" + constraint.join.right_tbl;

    // Now that we have hashed the left table, we need to go to the second set
    std::vector<std::vector<std::string>> right_tbl = from_qmt(right_tbl_path, std::vector<select_additional_args>{}, select_constraint);
    
    // In the future, if the select statement only wants like some from the right table, we can update that, for now assume the second set is SELECT (*), like select all columns
    std::vector<std::vector<std::string>> right_tbl_schema = read_schema(right_tbl_schema_path);

    std::unordered_set<std::string> right_tbl_column_names = select_constraint.table_columns.at(constraint.join.right_tbl);
    std::vector<std::vector<std::string>> new_right_tbl_schema;
    new_right_tbl_schema.push_back(std::vector<std::string>{});
    new_right_tbl_schema.push_back(std::vector<std::string>{});

    for(size_t x = 0; x < right_tbl_schema[0].size(); ++x){
        if(right_tbl_column_names.find(right_tbl_schema[0][x]) != right_tbl_column_names.end()){
            new_right_tbl_schema[0].push_back(right_tbl_schema[0][x]);
            new_right_tbl_schema[1].push_back(right_tbl_schema[1][x]);
        }
    }
    
    std::string default_values_string;
    attr_to_idx_mapping.clear();
    for(size_t j = 0; j < new_right_tbl_schema[0].size(); ++j){
        join_result_schema += new_right_tbl_schema[0][j] + "_" + new_right_tbl_schema[1][j] + ",";
        attr_to_idx_mapping[right_tbl_schema[0][j]] = j;
        if(new_right_tbl_schema[1][j] == "string"){
            default_values_string += str_default_value + ",";
        }
        else if(new_right_tbl_schema[1][j] == "int"){
            default_values_string += std::to_string(int_default_value) + ",";
        }
        else if(new_right_tbl_schema[1][j] == "double"){
            default_values_string += std::to_string(double_default_value) + ",";
        }
        else if(new_right_tbl_schema[1][j] == "char"){
            default_values_string += std::to_string(char_default_value) + ",";
        }
        else if(new_right_tbl_schema[1][j] == "bool"){
            default_values_string += std::to_string(bool_default_value) + ",";
        }
    }

    num_attributes = right_tbl.size();

    std::vector<std::string> csv_format_join_results;
    std::unordered_set<std::string> joined_hash_keys;

    if(num_attributes > 0){
        for (size_t col = 0; col < right_tbl[0].size(); col++) {
            std::string hash_key;
            std::string tbl_line;
            for (size_t row = 0; row < right_tbl.size(); row++) {
                
                if(row == attr_to_idx_mapping[constraint.join.on.rhs_expression]){
                    hash_key = right_tbl[row][col];
                }
                tbl_line += right_tbl[row][col] + ",";
            }

            auto it = parse_hash.find(hash_key);
            if(it != parse_hash.end()){
                joined_hash_keys.insert(hash_key);
                for(size_t j = 0; j < parse_hash[hash_key].size(); ++j){
                    csv_format_join_results.push_back(parse_hash[hash_key][j] + tbl_line);
                }
            }
        }
    }

    for(auto &pair : parse_hash){
        std::string hash = pair.first;
        std::vector<std::string> left_tbl_lines = pair.second;
        if(joined_hash_keys.find(hash) == joined_hash_keys.end()){
            assert(left_tbl_lines.size() == 1);
            std::string left_tbl_line = left_tbl_lines[0];
            csv_format_join_results.push_back(left_tbl_line + default_values_string);
        }
    }

    join_result = vectorize_csv(csv_format_join_results);
    return join_result;
}

/*
Implementation for the JOIN function
Arguments:
    - arguments: contains left tbl_name, right tbl_name, the type of join, and the constraint on which to join

Returns:
    - an in-memory table that represents the result of the join
*/
std::vector<std::vector<std::string>> join_qmt(const select_args &select_constraint, const std::vector<select_additional_args> &constraints, std::vector<std::vector<std::string>> &left_tbl, std::vector<std::vector<std::string>> &left_tbl_schema, std::string &join_result_schema){
    std::cout << "Running join implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number
    
    std::vector<std::vector<std::string>> join_result;

    for(size_t i = 0; i < constraints.size(); ++i){
        select_additional_args constraint = constraints[i];
        std::cout << constraint.join.left_tbl << " joined on " << constraint.join.right_tbl << std::endl;
        
        if(constraint.join.join_type == INNER){
            join_result = inner_join_qmt(select_constraint, constraint, left_tbl, left_tbl_schema, join_result_schema);
        }
        else if(constraint.join.join_type == LEFT){
            join_result = left_join_qmt(select_constraint, constraint, left_tbl, left_tbl_schema, join_result_schema);
        }

        left_tbl = join_result;
        left_tbl_schema = vectorize_schema(join_result_schema);
    }

    return join_result;
}

/*
Helper function to sort the hash keys so that we know how we want to order the values during the ORDER keyword
Arguments:
    - arguments: contains left tbl_name, right tbl_name, the type of join, and the constraint on which to join

Returns:
    - an in-memory table that represents the result of the join
*/
template <typename T>
void quicksort_values(std::vector<T> &vec, int start, int end){
    if(start >= end){
        return;
    }

    int pivot = end;

    int j = start;
    int i = start - 1;
    while(j < pivot){
        if(vec[j] < vec[pivot]){
            i++;
            std::swap(vec[i], vec[j]);
        }
        j++;
    }
    i++;
    pivot = i;
    std::swap(vec[i], vec[j]);

    quicksort_values(vec, start, pivot - 1);
    quicksort_values(vec, pivot + 1, end);
}

/*
Implementation for the ORDER BY function
Arguments:
    - arguments: contains left tbl_name, right tbl_name, the type of join, and the constraint on which to join

Returns:
    - an in-memory table that represents the result of the join
*/
std::vector<std::vector<std::string>> order_qmt(const std::vector<select_additional_args> &constraints, const std::vector<std::vector<std::string>> &tbl, const std::vector<std::vector<std::string>> &tbl_schema){
    std::cout << "Running order implementation, can fill out semantics later\n";

    std::vector<std::vector<std::string>> result_tbl = tbl;
    for(size_t i = 0; i < constraints.size(); ++i){
        select_additional_args curr_constraint = constraints[i];

        int num_attributes = result_tbl.size();

        std::vector<std::string> sorted_csv_tables;
        std::unordered_map<std::string, std::vector<std::string>> order_key_pairs;
        std::string col_type = "string";

        // Create a mapping to find which attribute goes to which column index
        std::unordered_map<std::string, int> attr_to_idx_mapping;
        for(size_t j = 0; j < tbl_schema[0].size(); ++j){
            attr_to_idx_mapping[tbl_schema[0][j]] = j;
            if(tbl_schema[0][j] == curr_constraint.order.col){
                col_type = tbl_schema[1][j];
            }
        }

        // Go through the table and write out the table out to disk
        if(num_attributes > 0){
            for (size_t col = 0; col < result_tbl[0].size(); col++) {
                std::string hash_key;
                std::string tbl_line;
                for (size_t row = 0; row < result_tbl.size(); row++) {
                    if(row == attr_to_idx_mapping[curr_constraint.order.col]){
                        hash_key = result_tbl[row][col];
                    }
                    tbl_line += result_tbl[row][col] + ",";
                }
                order_key_pairs[hash_key].push_back(tbl_line);
            }
        }
        else{
            return result_tbl;
        }

        std::vector<std::string> sort_keys;
        for(auto &pair : order_key_pairs){
            sort_keys.push_back(pair.first);
        }

        if(col_type == "string"){
            quicksort_values(sort_keys, 0, sort_keys.size() - 1);
        }
        else if(col_type == "int"){
            std::vector<int> int_sort_keys;
            for(size_t i = 0; i < sort_keys.size(); ++i){
                int_sort_keys.push_back(std::stoi(sort_keys[i]));
            }
            quicksort_values(int_sort_keys, 0, int_sort_keys.size() - 1);
            sort_keys.clear();
            for(size_t i = 0; i < int_sort_keys.size(); ++i){
                sort_keys.push_back(std::to_string(int_sort_keys[i]));
            }
        }
        else if(col_type == "double"){
            std::vector<double> double_sort_keys;
            for(size_t i = 0; i < sort_keys.size(); ++i){
                double_sort_keys.push_back(std::stod(sort_keys[i]));
            }
            quicksort_values(double_sort_keys, 0, double_sort_keys.size() - 1);
            sort_keys.clear();
            for(size_t i = 0; i < double_sort_keys.size(); ++i){
                sort_keys.push_back(std::to_string(double_sort_keys[i]));
            }
        }
        else if(col_type == "bool"){
            std::vector<bool> bool_sort_keys;
            for(size_t i = 0; i < sort_keys.size(); ++i){
                if(sort_keys[i] == "true"){
                    bool_sort_keys.push_back(true);
                }
                else{
                    bool_sort_keys.push_back(false);
                }
            }
            quicksort_values(bool_sort_keys, 0, bool_sort_keys.size() - 1);
            sort_keys.clear();
            for(size_t i = 0; i < bool_sort_keys.size(); ++i){
                sort_keys.push_back(std::to_string(bool_sort_keys[i]));
            }
        }
        else if(col_type == "char"){
            std::vector<char> char_sort_keys;
            for(size_t i = 0; i < sort_keys.size(); ++i){
                char_sort_keys.push_back(sort_keys[i][0]);
            }
            quicksort_values(char_sort_keys, 0, char_sort_keys.size() - 1);
            sort_keys.clear();
            for(size_t i = 0; i < char_sort_keys.size(); ++i){
                sort_keys.push_back(std::to_string(char_sort_keys[i]));
            }
        }
        else{
            exit_with_error(UNKNOWN_TYPE, col_type);
        }

        if(curr_constraint.order.sort_direction == ASC){
            for(size_t i = 0; i < sort_keys.size(); ++i){
                for(size_t j = 0; j < order_key_pairs[sort_keys[i]].size(); ++j){
                    sorted_csv_tables.push_back(order_key_pairs[sort_keys[i]][j]);
                }
            }
        }
        else{
            for(int i = sort_keys.size() - 1; i >= 0; --i){
                for(size_t j = 0; j < order_key_pairs[sort_keys[i]].size(); ++j){
                    sorted_csv_tables.push_back(order_key_pairs[sort_keys[i]][j]);
                }
            }
        }

        result_tbl = vectorize_csv(sorted_csv_tables);

    }

    return result_tbl;
}

/*
Function to display the table.
Arguments:
    - table: the in memory table to be printed out 
    - schema: the in memory schema to print out column headers
*/
void display_in_memory_table(const std::vector<std::vector<std::string>> &table, const std::vector<std::vector<std::string>> &schema){

    std::vector<int> column_widths;
    std::string result_path = "query_result";
    std::ofstream result_file(result_path);

    // Going to use AI to help me do some pretty printing so just beware, I am not a cout wizard
    std::cout << std::left;
    result_file << std::left;

    // print out how many results there are
    std::cout << "Query returned " << table[0].size() << " results.\n";
    result_file << "Query returned " << table[0].size() << " results.\n";

    int total_width = 0;
    // go through the column names and types, and print them out nicely
    for(size_t i = 0; i < schema[0].size(); ++i){
        std::string column_header = schema[0][i] + " (" + schema[1][i] + ")";
        int width = column_header.size() + 4;
        column_widths.push_back(width);
        total_width += width;
        std::cout << std::setw(width) << column_header;
        result_file << std::setw(width) << column_header;
    }

    // with a nice ------- underlining the column names and types
    std::cout << std::endl;
    std::cout << std::string(total_width + 5, '-') << "\n";

    result_file << std::endl;
    result_file << std::string(total_width + 5, '-') << "\n";

    // Go through the table and print it out nicely
    if(table[0].size() > 0){
        for (size_t col = 0; col < table[0].size(); col++) {
            std::cout << std::left;
            result_file << std::left;

            for (size_t row = 0; row < table.size(); row++) {
                std::string print_out_value = table[row][col];
                if(table[row][col].size() > column_widths[row] - 4){
                    print_out_value = table[row][col].substr(0, column_widths[row] - 4);
                    print_out_value += "...";
                }
                std::cout << std::setw(column_widths[row]) << print_out_value;
                result_file << std::setw(column_widths[row]) << table[row][col];
            }

            std::cout << "\n";
            result_file << "\n";
        }
    }

}

/*
Function to write the in-memory table onto disk.
Arguments:
    - table: the in memory table to be printed out 
    - tbl_name: the table name for the table
*/
void write_table_to_disk(const std::vector<std::vector<std::string>> &table, std::string tbl_name){
    std::string table_path = db_path + "/" + tbl_name;

    // Check to see if the table exists
    if(!fs::exists(table_path)){
        exit_with_error(NULL_TABLE, tbl_name);
    }

    std::ofstream tbl_file(table_path);
    std::string table_line;

    int num_attributes = table.size();
    int num_rows = 0;
    size_t num_leading_zeros = std::log2(PAGE_SIZE);

    // Go through the table and write out the table out to disk
    if(num_attributes > 0){
        num_rows = table[0].size();
        std::string binary_num_rows = std::bitset<12>(num_rows).to_string(); // HARDCODED TO HAVE PAGE SIZE OF 4096
        tbl_file << binary_num_rows << std::endl;
        std::string output_line;
        for (size_t col = 0; col < table[0].size(); col++) {
            for (size_t row = 0; row < table.size(); row++) {
                output_line += table[row][col] + ",";
            }

            tbl_file << output_line << "\n";
        }
    }else{
        for(int i = 0; i < num_leading_zeros; ++i){
            tbl_file << "0";
        }
        tbl_file << std::endl;
    }

    tbl_file.close();

}