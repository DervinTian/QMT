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

    for(size_t i = 0; i < expression.size(); ++i){
        if(expression[i] == "("){
            int count_extra_parens = 0;
            int start_of_expression = i;
            int end_of_expression = i;
            bool end_expression = false;
            for(int j = i + 1; j < expression.size(); ++j){
                if(expression[j] == "("){
                    count_extra_parens++;
                }
                if(expression[j] == ")"){
                    if(count_extra_parens == 0){
                        end_expression = true;
                        end_of_expression = j;
                        break;
                    }
                    count_extra_parens--;
                }
            }

            if(end_expression){
                std::vector<std::string> nested_expression;
                for(int k = start_of_expression + 1; k < end_of_expression; ++k){
                    nested_expression.push_back(expression[k]);
                }

                recursive_evaluate(nested_expression);
                
                std::vector<std::string> expression_copy;
                for(int q = 0; q < expression.size(); ++q){
                    if(q == start_of_expression){
                        expression_copy.push_back(nested_expression[0]);
                        q = end_of_expression + 1;
                        if(q >= expression.size()){
                            break;
                        }
                    }
                    expression_copy.push_back(expression[q]);
                }
                expression = expression_copy;
            }
            else{
                std::cout << "Wut\n";
            }

        }
    }

    std::unordered_map<int, operation> operation_positions;
    for(size_t i = 0; i < expression.size(); ++i){
        if(expression[i] == "+"){
            operation op;
            op.op_symbol = "+";
            op.position = i;
            op.priority = SUB_ADD;
            operation_positions[i] = op;
        }
        else if(expression[i] == "-"){
            operation op;
            op.op_symbol = "-";
            op.position = i;
            op.priority = SUB_ADD;
            operation_positions[i] = op;
        }
        else if(expression[i] == "*"){
            operation op;
            op.op_symbol = "*";
            op.position = i;
            op.priority = MULT_DIVIDE;
            operation_positions[i] = op;
        }
        else if(expression[i] == "/"){
            operation op;
            op.op_symbol = "/";
            op.position = i;
            op.priority = MULT_DIVIDE;
            operation_positions[i] = op;
        }
    }

    std::priority_queue<operation, std::vector<operation>, operation_comparator> operation_pq;

    for(auto &pair : operation_positions){
        int curr_operator_position = pair.first;
        operation curr_operation = pair.second;

        operation_pq.push(curr_operation);
    }

    while(operation_pq.size() > 0){
        operation top_operation = operation_pq.top();
        operation_pq.pop();

        std::vector<std::string> expression_copy;
        std::priority_queue<operation, std::vector<operation>, operation_comparator> operation_pq_copy;
        
        int left_pos = top_operation.position - 1;
        int right_pos = top_operation.position + 1;

        double op_result = 0;
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

        for(size_t i = 0; i < expression.size(); ++i){
            if(i == left_pos){
                expression_copy.push_back(std::to_string(op_result));
                i = right_pos;
            }
            else{
                expression_copy.push_back(expression[i]);
            }
        }

        expression = expression_copy;

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

        operation_pq = operation_pq_copy;
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
double math_qmt(const select_additional_args &constraint, std::string table_val){
    std::cout << "Running math implementation\n";

    select_additional_args constraint_copy = constraint;

    for(size_t i = 0; i < constraint_copy.where.math.expression_pieces.size(); ++i){
        if(constraint_copy.where.math.expression_pieces[i] == "?"){
            constraint_copy.where.math.expression_pieces[i] = table_val;
        }
    }

    recursive_evaluate(constraint_copy.where.math.expression_pieces);
    double result = std::stod(constraint_copy.where.math.expression_pieces[0]);
    std::cout << "Result is: " << result << std::endl;

    return result;
}