#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "globals.h"

/*
Function to run the '=' or "EQUAL" comparator

Arguments:
    - lhs: represents the left hand side value of the expression
    - rhs: represents the right hand side value of the expression

Return:
    - returns a boolean value that represents the whether or not the two values are equal to each other
*/
bool comp_equal(const cmp_object &lhs, const cmp_object &rhs){
    if(lhs.type == STRING){
        return lhs.param_string == rhs.param_string;
    }
    else if(lhs.type == CHAR){
        return lhs.param_char == rhs.param_char;
    }
    else if(lhs.type == BOOL){
        return lhs.param_bool == rhs.param_bool;
    }
    else if(lhs.type == INT){
        return lhs.param_int == rhs.param_int;
    }
    else{
        exit_with_error(UNKNOWN_TYPE, std::to_string(lhs.type));
    }
    return false;
}

/*
Function to run the '<' or "LESS" comparator

Arguments:
    - lhs: represents the left hand side value of the expression
    - rhs: represents the right hand side value of the expression

Return:
    - returns a boolean value that represents the whether or not the left value is less than the right value
*/
bool comp_less(const cmp_object &lhs, const cmp_object &rhs){
    if(lhs.type == STRING){
        return lhs.param_string < rhs.param_string;
    }
    else if(lhs.type == CHAR){
        return lhs.param_char < rhs.param_char;
    }
    else if(lhs.type == BOOL){
        std::cout << "Why are we comparing less than for bools boi?\n";
    }
    else if(lhs.type == INT){
        return lhs.param_int < rhs.param_int;
    }
    else{
        exit_with_error(UNKNOWN_TYPE, std::to_string(lhs.type));
    }
    return false;
}

/*
Function to run the '>' or "GREATER" comparator

Arguments:
    - lhs: represents the left hand side value of the expression
    - rhs: represents the right hand side value of the expression

Return:
    - returns a boolean value that represents the whether or not the left value is greater than the right value
*/
bool comp_greater(const cmp_object &lhs, const cmp_object &rhs){
    if(lhs.type == STRING){
        return lhs.param_string > rhs.param_string;
    }
    else if(lhs.type == CHAR){
        return lhs.param_char > rhs.param_char;
    }
    else if(lhs.type == BOOL){
        std::cout << "Why are we comparing greater than for bools boi?\n";
    }
    else if(lhs.type == INT){
        return lhs.param_int > rhs.param_int;
    }
    else{
        exit_with_error(UNKNOWN_TYPE, std::to_string(lhs.type));
    }
    return false;
}

/*
Function to run the '<=' or "LESS_THAN_OR_EQUAL_TO" comparator

Arguments:
    - lhs: represents the left hand side value of the expression
    - rhs: represents the right hand side value of the expression

Return:
    - returns a boolean value that represents the whether or not the left value is less than or equal to the right value
*/
bool comp_less_than_or_equal_to(const cmp_object &lhs, const cmp_object &rhs){
    if(lhs.type == STRING){
        return lhs.param_string <= rhs.param_string;
    }
    else if(lhs.type == CHAR){
        return lhs.param_char <= rhs.param_char;
    }
    else if(lhs.type == BOOL){
        std::cout << "Why are we comparing less than or equal for bools boi?\n";
    }
    else if(lhs.type == INT){
        return lhs.param_int <= rhs.param_int;
    }
    else{
        exit_with_error(UNKNOWN_TYPE, std::to_string(lhs.type));
    }
    return false;
}

/*
Function to run the '>=' or "GREATER_THAN_OR_EQUAL" comparator

Arguments:
    - lhs: represents the left hand side value of the expression
    - rhs: represents the right hand side value of the expression

Return:
    - returns a boolean value that represents the whether or not the left value is greater than or equal to the right value
*/
bool comp_greater_than_or_equal_to(const cmp_object &lhs, const cmp_object &rhs){
    if(lhs.type == STRING){
        return lhs.param_string >= rhs.param_string;
    }
    else if(lhs.type == CHAR){
        return lhs.param_char >= rhs.param_char;
    }
    else if(lhs.type == BOOL){
        std::cout << "Why are we comparing greater than or equal for bools boi?\n";
    }
    else if(lhs.type == INT){
        return lhs.param_int >= rhs.param_int;
    }
    else{
        exit_with_error(UNKNOWN_TYPE, std::to_string(lhs.type));
    }
    return false;
}

/*
Function to run the '!=' or "NOT_EQUAL" comparator

Arguments:
    - lhs: represents the left hand side value of the expression
    - rhs: represents the right hand side value of the expression

Return:
    - returns a boolean value that represents the whether or not the two values are not equal to each other
*/
bool comp_not_equal(const cmp_object &lhs, const cmp_object &rhs){
    if(lhs.type == STRING){
        return lhs.param_string != rhs.param_string;
    }
    else if(lhs.type == CHAR){
        return lhs.param_char != rhs.param_char;
    }
    else if(lhs.type == BOOL){
        return lhs.param_bool != rhs.param_bool;
    }
    else if(lhs.type == INT){
        return lhs.param_int != rhs.param_int;
    }
    else{
        exit_with_error(UNKNOWN_TYPE, std::to_string(lhs.type));
    }
    return false;
}