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
cmp_return_type where_qmt(std::string curr_col, std::string curr_col_type, std::string table_val, const select_additional_args &constraint){
    bool good_to_push = false;
    std::string cmp_type;
    std::string comparator;
    cmp_object lhs_val;
    cmp_object rhs_val;

    // for now just assuming that the left-hand-side will represent the column name, no other operations for now
    if(curr_col == constraint.where.lhs_expression){ // check to make sure that we are looking at the right column
        if(check_string(constraint.where.rhs_expression)){
            cmp_type = "string";
            rhs_val.param_string = constraint.where.rhs_expression.substr(1, constraint.where.rhs_expression.size() - 2);
            rhs_val.type = STRING;
        }
        else if(check_char(constraint.where.rhs_expression)){
            cmp_type = "char";
            rhs_val.param_char = constraint.where.rhs_expression[1];
            rhs_val.type = CHAR;
        }
        else if(check_bool(constraint.where.rhs_expression)){
            cmp_type = "bool";
            if(constraint.where.rhs_expression == "true"){
                rhs_val.param_bool = true;
            }
            else{
                rhs_val.param_bool = false;
            }
            rhs_val.type = BOOL;
        }
        else if(check_int(constraint.where.rhs_expression)){
            cmp_type = "int";
            rhs_val.param_int = std::stoi(constraint.where.rhs_expression);
            rhs_val.type = INT;
        }
        else{
            std::cout << "Unknown type error: " << constraint.where.rhs_expression << " does not fall under any of the accepted types!\n";
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

            good_to_push = comparators[comparator](lhs_val, rhs_val);
        }
        else{
            return UNKNOWN;
        }
    }
    else{
        return UNKNOWN;
    }

    if(good_to_push){
        return TRUE;
    }
    else{
        return FALSE;
    }

    return FALSE;
}