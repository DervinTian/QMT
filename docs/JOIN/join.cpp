/*
Implementation for the JOIN function
Arguments:
    - arguments: contains left tbl_name, right tbl_name, the type of join, and the constraint on which to join

Returns:
    - an in-memory table that represents the result of the join
*/
std::vector<std::vector<std::string>> join_qmt(const std::vector<select_additional_args> &constraints, std::vector<std::vector<std::string>> &left_tbl, std::vector<std::vector<std::string>> &left_tbl_schema, std::string &join_result_schema){
    std::cout << "Running join implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number

    std::vector<std::vector<std::string>> join_result;

    for(size_t i = 0; i < constraints.size(); ++i){
        select_additional_args constraint = constraints[i];
        std::cout << constraint.join.left_tbl << " joined on " << constraint.join.right_tbl << std::endl;
        
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
        std::vector<std::vector<std::string>> right_tbl = from_qmt(right_tbl_path, std::vector<select_additional_args>{});
        
        // In the future, if the select statement only wants like some from the right table, we can update that, for now assume the second set is SELECT (*), like select all columns
        std::vector<std::vector<std::string>> right_tbl_schema = read_schema(right_tbl_schema_path);
        
        attr_to_idx_mapping.clear();
        for(size_t j = 0; j < right_tbl_schema[0].size(); ++j){
            join_result_schema += right_tbl_schema[0][j] + "_" + right_tbl_schema[1][j] + ",";
            attr_to_idx_mapping[right_tbl_schema[0][j]] = j;
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

        num_attributes = 0;
        if(csv_format_join_results.size() > 0){
            std::stringstream ss(csv_format_join_results[0]);
            std::string token;
            while(std::getline(ss, token, ',')){
                num_attributes++;
            }
        }
        else{
            return join_result;
        }

        for(int j = 0; j < num_attributes; ++j){
            join_result.push_back(std::vector<std::string>{});
        }

        int attr_idx = 0;
        for(size_t j = 0; j < csv_format_join_results.size(); ++j){
            std::stringstream ss(csv_format_join_results[j]);
            std::string token;
            while(std::getline(ss, token, ',')){
                join_result[attr_idx].push_back(token);
                attr_idx = (attr_idx + 1) % num_attributes;
            }
        }
    }

    return join_result;
}