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
    
    size_t pos = table_path.rfind('/');
    std::string tbl_name;
    if(pos != std::string::npos){
        tbl_name = table_path.substr(pos + 1);

        std::string file_extension;
        if(tbl_name.size() > 4){
            for(size_t idx = tbl_name.size() - 4; idx < tbl_name.size(); ++idx){
                file_extension += tbl_name[idx];
            }

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