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
    std::string cmp_type;
    std::string comparator;
    cmp_object lhs_val;
    cmp_object rhs_val;

    // for now just assuming that the left-hand-side will represent the column name, no other operations for now
    if(constraint.where.type == MATH){
        double expression_result = 0;
        std::vector<int> variable_idx;
        int curr_variable_idx = 0;
        for(size_t i = 0; i < constraint.where.left_math.expression_pieces.size(); ++i){
            if(constraint.where.left_math.expression_pieces[i] == "?"){
                variable_idx.push_back(i);
            }
        }

        assert(variable_idx.size() == constraint.where.left_math.variables.size());

        cmp_object_type lhs_type;
        if(constraint.where.left_math.variables.size() > 0){
            cmp_object curr_variable;
            for(size_t i = 0; i < constraint.where.left_math.variables.size(); ++i){
                std::string variable_name = constraint.where.left_math.variables[i];
                curr_variable = expression_variables[variable_name];
                lhs_type = curr_variable.type;

                if(lhs_type == INT){
                    constraint.where.left_math.expression_pieces[curr_variable_idx] = std::to_string(curr_variable.param_int);
                }
                else{
                    constraint.where.left_math.expression_pieces[curr_variable_idx] = std::to_string(curr_variable.param_double);
                }
                curr_variable_idx++;
            }

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
        else{
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
        bool lhs_is_variable = true;
        if(expression_variables.find(constraint.where.lhs_expression) == expression_variables.end()){
            lhs_is_variable = false;
        }

        bool rhs_is_variable = true;
        if(expression_variables.find(constraint.where.rhs_expression) == expression_variables.end()){
            rhs_is_variable = false;
        }

        if(lhs_is_variable){
            lhs_val = expression_variables[constraint.where.lhs_expression];
        }
        else{
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

        if(lhs_val.type != rhs_val.type){
            std::cout << "Values for the left hand side and the right hand side are not comparable" << std::endl;
            exit(67);
        }
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

    if(good_to_push){
        return TRUE;
    }
    else{
        return FALSE;
    }

    return FALSE;
}