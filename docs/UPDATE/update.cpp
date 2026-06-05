/*
Implementation for the UPDATE function. Allows the user to modify the table.
Arguments:
    - arguments: contains tbl_name, the set values that we want to set, and any constraints for the table
*/
void update_qmt(const cmd_args &arguments){
    std::cout << "Running update implementation, can fill out semantics later\n";

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.update.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.update.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.update.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.update.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.update.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::vector<select_additional_args> additional_args;
    select_additional_args add_args;

    // For any additional lines after the select statement, such as WHERE
    for(int i = 0; i < arguments.update.additionals.size(); ++i){
        std::stringstream ss(arguments.update.additionals[i]);
        std::string cmd_type;
        ss >> cmd_type;
        
        for (char& c : cmd_type) {
            c = std::tolower(static_cast<unsigned char>(c));
        }

        // Fill in the arguments for the specified keyword
        if(cmd_type == "where"){
            add_args.where.tbl_name = arguments.update.tbl_name;
        }

        fill_in_additional_cmds[cmd_type](arguments.update.additionals[i], add_args);
        additional_args.push_back(add_args);
        executing_line_num++;
    }

    std::vector<std::vector<std::string>> schema = read_schema(schema_path);

    // Create a mapping to find which attribute goes to which column index
    std::unordered_map<std::string, int> attr_to_idx_mapping;
    for(size_t i = 0; i < schema[0].size(); ++i){
        attr_to_idx_mapping[schema[0][i]] = i;
    }

    // Read in the table into memory with the given constraints, to try and reduce on the memory load
    std::vector<std::vector<std::string>> filetered_table = from_qmt(table_path, additional_args);
    std::unordered_set<std::string> filtered_results;
    std::unordered_set<size_t> columns_in_there;

    std::vector<std::vector<std::string>> whole_table = from_qmt(table_path, std::vector<select_additional_args>{});
    size_t num_cols = whole_table.size();

    if(filetered_table.size() > 0){
        std::string search_key;
        for (size_t col = 0; col < filetered_table[0].size(); col++) {
            for (size_t row = 0; row < filetered_table.size(); row++) {
                search_key += filetered_table[row][col] + '/';
            }
            filtered_results.insert(search_key);
        }
    }

    if(whole_table.size() == 0){
        std::cout << "Empty table, cannot update records on an empty table!\n";
        exit(10);
    }

    columns_in_there.insert(0);

    for (size_t col = 0; col < whole_table[0].size(); col++) {
        std::string table_search_key;
        for (size_t row = 0; row < whole_table.size(); row++) {
            table_search_key += whole_table[row][col] + '/';
        }

        if(filtered_results.find(table_search_key) != filtered_results.end()){
            columns_in_there.insert(col);
        }
    }

    for(int i = 0; i < arguments.update.set_values.size(); ++i){
        std::string col_name = arguments.update.set_values[i].first;
        std::string new_val = arguments.update.set_values[i].second;

        int attr_idx = attr_to_idx_mapping[col_name];

        std::string actual_type = schema[1][attr_idx];

        bool types_match = check_value_against_type[actual_type](new_val);

        if(types_match){
            if(check_string(new_val) || check_char(new_val)){
                new_val = new_val.substr(1, new_val.size() - 2);
            }
        }
        else{
            std::cout << "Value " << new_val << " does not match the type for column " << col_name << std::endl;
        }

        for(size_t x = 0; x < whole_table[attr_idx].size(); ++x){
            auto it = columns_in_there.find(x);
            if(it != columns_in_there.end()){
                std::cout << "Here\n";
                std::cout << new_val << std::endl;
                whole_table[attr_idx][x] = new_val;
            }
        }

        executing_line_num++;
    }

    write_table_to_disk(whole_table, arguments.update.tbl_name);

    executing_line_num++;
    return;
}