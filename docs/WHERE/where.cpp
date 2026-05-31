void where_qmt(){
    (...)
    {
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
    
    (...)
    }