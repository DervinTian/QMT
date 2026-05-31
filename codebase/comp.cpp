#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "globals.h"

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
        std::cout << "Unknown type in the comparator? " << lhs.type << std::endl;
        exit(69);
    }
    return false;
}

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
        std::cout << "Unknown type in the comparator? " << lhs.type << std::endl;
        exit(69);
    }
    return false;
}

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
        std::cout << "Unknown type in the comparator? " << lhs.type << std::endl;
        exit(69);
    }
    return false;
}

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
        std::cout << "Unknown type in the comparator? " << lhs.type << std::endl;
        exit(69);
    }
    return false;
}

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
        std::cout << "Unknown type in the comparator? " << lhs.type << std::endl;
        exit(69);
    }
    return false;
}

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
        std::cout << "Unknown type in the comparator? " << lhs.type << std::endl;
        exit(69);
    }
    return false;
}