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

    if(!valid_pathname(schema_path)){
        return result;
    }

    size_t pos = schema_path.rfind('/');
    std::string tbl_name = schema_path.substr(pos + 1);
    if(!fs::exists(schema_path)){
        std::cout << "Schema for " << tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::ifstream schema_file(schema_path);

    std::string schema_line;
    while(std::getline(schema_file, schema_line));
    schema_file.close();

    std::stringstream schema_ss(schema_line);
    std::string token;
    
    while(std::getline(schema_ss, token, ',')){
        if(token.size() > 0){
            schema.push_back(token);
        }
    }
    
    for(size_t k = 0; k < schema.size(); ++k){
        std::stringstream temp_ss(schema[k]);

        std::string first, second;

        std::getline(temp_ss, first, '_');
        std::getline(temp_ss, second, '_');

        schema_names.push_back(first);
        schema_types.push_back(second);
    }

    result.push_back(schema_names);
    result.push_back(schema_types);

    return result;
}

/*
Function to load the table into memory with the given where constraints.
Arguments:
    - table_path: the path of the table to read in
    - constraints: the constraint(s) to be placed on the table

Return:
    - 2D vector containing the table structure
    - Is a vector that contains vectors of column values
*/
std::vector<std::vector<std::string>> read_in_table(const std::string &table_path, const std::vector<select_additional_args> &constraints){

    // Structure of the in-memory table will be defined here
    // Made up of a vector of vectors
    // Outer vectors represent a type
    // Inner vectors hold the values for that type
    // Values align across all the vectors
    std::vector<std::vector<std::string>> result;

    size_t pos = table_path.rfind('/');
    std::string tbl_name = table_path.substr(pos + 1);

    if(!fs::exists(table_path)){
        std::cout << "Table " << tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::ifstream table_file(table_path);

    std::string schema_path = db_path + "/schemas/" + tbl_name;

    std::vector<std::vector<std::string>> schema = read_schema(schema_path);
    std::vector<std::string>& column_names = schema[0];
    std::vector<std::string>& column_types = schema[1];
    int num_cols = (int)column_names.size();

    for(int i = 0; i < num_cols; ++i){
        std::vector<std::string> temp;
        result.push_back(temp);
    }

    int curr_attribute_counter = 0;
    int curr_col_idx = 0;
    std::string table_line;
    while(std::getline(table_file, table_line)){

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

        while(std::getline(table_ss, table_val, ',')){

            curr_col = column_names[curr_attribute_counter];
            curr_col_type = column_types[curr_attribute_counter];
            buffer.push_back(table_val);
            // result[curr_attribute_counter].push_back(table_val);
            curr_attribute_counter = (curr_attribute_counter + 1) % num_cols;

            for(size_t x = 0; x < constraints.size(); ++x){
                select_additional_args constraint = constraints[x]; // update to deal with all the constraints, not just one at a time

                bool where_constraint = false;

                if(constraint.where.tbl_name.size() > 0){
                    where_constraint = true;
                }
                
                if(where_constraint){
                    good_to_push = where_qmt(curr_col, curr_col_type, table_val, constraint);
                    
                    if(good_to_push == FALSE){
                        push = false;
                    }
                }
            }

        }

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

    // Going to use AI to help me do some pretty printing so just beware, I am not a cout wizard
    std::cout << std::left;
    int total_width = 0;
    for(size_t i = 0; i < schema[0].size(); ++i){
        std::string column_header = schema[0][i] + " (" + schema[1][i] + ")";
        int width = column_header.size() + 4;
        column_widths.push_back(width);
        total_width += width;
        std::cout << std::setw(width) << column_header;
    }
    std::cout << std::endl;

    std::cout << std::string(total_width + 5, '-') << "\n";

    for (size_t col = 0; col < table[0].size(); col++) {
        std::cout << std::left;
        for (size_t row = 0; row < table.size(); row++) {
            std::cout << std::setw(column_widths[row]) << table[row][col];
        }
        std::cout << "\n";
    }

}

/*
Implementation for the SELECT function
Arguments:
    - arguments: contains tbl_name, selection columns, and additional data structures for additional constraints
*/
void select_qmt(const cmd_args &arguments){
    std::cout << "Running select implementation, can fill out semantics later\n";

    std::vector<select_additional_args> additional_args;
    select_additional_args add_args;

    // For any additional lines after the select statement, such as WHERE
    for(int i = 0; i < arguments.select.additionals.size(); ++i){
        std::stringstream ss(arguments.select.additionals[i]);
        std::string cmd_type;
        ss >> cmd_type;
        
        for (char& c : cmd_type) {
            c = std::tolower(static_cast<unsigned char>(c));
        }

        // Fill in the arguments for the specified keyword
        if(cmd_type == "where"){
            add_args.where.tbl_name = arguments.select.tbl_name;
        }

        fill_in_additional_cmds[cmd_type](arguments.select.additionals[i], add_args);
        additional_args.push_back(add_args);
        executing_line_num++;
    }

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.select.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.select.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.select.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.select.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.select.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::vector<std::vector<std::string>> schema = read_schema(schema_path);

    // Create a mapping to find which attribute goes to which column index
    std::unordered_map<std::string, int> attr_to_idx_mapping;
    for(size_t i = 0; i < schema[0].size(); ++i){
        attr_to_idx_mapping[schema[0][i]] = i;
    }

    // Read in the table into memory with the given constraints, to try and reduce on the memory load
    std::vector<std::vector<std::string>> table = read_in_table(table_path, additional_args);

    std::vector<std::vector<std::string>> result_table;
    std::vector<std::vector<std::string>> result_schema;

    result_schema.push_back(std::vector<std::string>{});
    result_schema.push_back(std::vector<std::string>{});

    // If they select all columns, then just display everything
    if(arguments.select.sel_columns.size() == 1 && arguments.select.sel_columns[0] == "*"){
        display_in_memory_table(table, schema);
        return;
    }

    // Otherwise, just show the columns that were specified
    for(size_t i = 0; i < arguments.select.sel_columns.size(); ++i){
        result_table.push_back(table[attr_to_idx_mapping[arguments.select.sel_columns[i]]]);
        result_schema[0].push_back(schema[0][attr_to_idx_mapping[arguments.select.sel_columns[i]]]);
        result_schema[1].push_back(schema[1][attr_to_idx_mapping[arguments.select.sel_columns[i]]]);;
    }

    // Print it out
    display_in_memory_table(result_table, result_schema);

    executing_line_num++; // update the execution line number
    return;

}