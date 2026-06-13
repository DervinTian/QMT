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
#include <filesystem>
#include <cassert>

#include "fill_args.h"
#include "impls.h"
#include "globals.h"

namespace fs = std::filesystem;

// Declare global variables
std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, cmd_args&)>> fill_in_cmd;
std::unordered_map<std::string, std::function<void(const std::string&, select_additional_args&)>> fill_in_additional_cmds;
std::unordered_map<std::string, std::function<void(const cmd_args&)>> cmd_impls;
std::unordered_map<std::string, std::function<void(const select_additional_args&)>> additional_cmd_impls;
std::unordered_map<std::string, std::function<bool(const std::string&)>> check_value_against_type;
std::unordered_map<std::string, std::function<bool(const cmp_object&, const cmp_object&)>> comparators;
std::string db_path;
std::vector<std::string> in_memory_script;

int executing_line_num = 0;

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
        std::cout << "Schema for " << tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::ifstream schema_file(schema_path);

    std::string schema_line;
    std::getline(schema_file, schema_line); // get the table schema, column names and column types
    schema_file.close();

    std::stringstream schema_ss(schema_line);
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
                    constraint.where.left_math.expression_pieces[curr_variable_idx] = std::to_string(curr_variable.param_int);
                }
                else{
                    constraint.where.left_math.expression_pieces[curr_variable_idx] = std::to_string(curr_variable.param_double);
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
                std::cout << "Unknown type for " << constraint.where.lhs_expression << ". Did you forget to use the MATH keyword?" << std::endl;
                exit(17);
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
                    constraint.where.right_math.expression_pieces[curr_variable_idx] = std::to_string(curr_variable.param_int);
                }
                else{
                    constraint.where.right_math.expression_pieces[curr_variable_idx] = std::to_string(curr_variable.param_double);
                }
                curr_variable_idx++;
            }

            expression_result = math_qmt(constraint.where.left_math.expression_pieces);

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
                std::cout << "Unknown type for " << constraint.where.rhs_expression << ". Did you forget to use the MATH keyword?" << std::endl;
                exit(17);
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
                std::cout << "Unknown type in the expression\n" << std::endl;
                exit(67);
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
                std::cout << "Unknown type in the expression\n" << std::endl;
                exit(67);
            }
        }

        // Make sure that the types for the lhs and the rhs are comparable
        if(lhs_val.type != rhs_val.type){
            std::cout << "Values for the left hand side and the right hand side are not comparable" << std::endl;
            exit(67);
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
        std::cout << "Unknown comparator: " << constraint.where.comparator << std::endl;
        exit(9);
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
std::vector<std::vector<std::string>> from_qmt(const std::string &table_path, const std::vector<select_additional_args> &constraints){

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
            std::cout << "Table " << tbl_name << " doesn't exist!\n";
            exit(3);
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

    for(int i = 0; i < num_cols; ++i){
        std::vector<std::string> temp;
        result.push_back(temp);
    }

    // initialize variables to be used later
    int curr_attribute_counter = 0; // used into index into column_names and column_types
    std::string table_line;

    // Go line by line and get each row of data
    while(std::getline(table_file, table_line)){

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
            buffer.push_back(table_val);
            curr_attribute_counter = (curr_attribute_counter + 1) % num_cols;

            // go through the constraints, maybe more than one line like two WHERE clauses ANDED together
            // and fill in the variables
            for(size_t x = 0; x < constraints.size(); ++x){
                select_additional_args constraint = constraints[x]; // update to deal with all the constraints, not just one at a time

                // If math, fill it in for math things, otherwise just default to a regular compare between like the value type
                if(constraint.where.type == MATH){
                    
                    // Want to fill in the variables, so see if it is in the left
                    for(size_t i = 0; i < constraint.where.left_math.variables.size(); ++i){
                        if(constraint.where.left_math.variables[i] == curr_col){
                            cmp_object variable;
                            if(curr_col_type == "int"){
                                variable.param_int = std::stoi(table_val); // convert it into an int
                                variable.type = INT;
                            }
                            else if(curr_col_type == "double"){
                                variable.param_double = std::stod(table_val); // convert it into a double
                                variable.type = DOUBLE;
                            }
                            expression_variables[curr_col] = variable;
                            continue;
                        }
                    }

                    // Want to fill in the variables, so see if it is in the right
                    for(size_t i = 0; i < constraint.where.right_math.variables.size(); ++i){
                        if(constraint.where.right_math.variables[i] == curr_col){
                            cmp_object variable;
                            if(curr_col_type == "int"){
                                variable.param_int = std::stoi(table_val); // convert it into an int
                                variable.type = INT;
                            }
                            else if(curr_col_type == "double"){
                                variable.param_double = std::stod(table_val); // convert it into a double
                                variable.type = DOUBLE;
                            }
                            expression_variables[curr_col] = variable;
                            continue;
                        }
                    }
                }
                else{
                    // otherwise, check to see if the lhs_expression or rhs_expression contains the column, then just add it to the variables map
                    if(curr_col == constraint.where.lhs_expression || curr_col == constraint.where.rhs_expression){
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
                        expression_variables[curr_col] = variable; // map that variable to it's corresponding object
                    }
                }

                // idempodent operation to set whether or not the where operation is even there, since it will have a tbl_name for sure
                if(constraint.where.tbl_name.size() > 0){
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

    }

    table_file.close();
    return result;

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
                std::cout << std::setw(column_widths[row]) << table[row][col];
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
        std::cout << "Table " << tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::ofstream tbl_file(table_path);
    std::string table_line;

    int num_attributes = table.size();

    // Go through the table and write out the table out to disk
    if(num_attributes > 0){
        for (size_t col = 0; col < table[0].size(); col++) {
            for (size_t row = 0; row < table.size(); row++) {
                tbl_file << table[row][col] << ",";
            }

            tbl_file << "\n";
        }
    }

    tbl_file.close();

}