/*
Implementation for the CREATE function
Arguments:
    - arguments: contains tbl_name to be created
*/
void create_qmt(const cmd_args &arguments){
    std::cout << "Running create implementation, can fill out semantics later\n";

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.create.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.create.tbl_name << ") detected.\n";
        exit(2);
    }
    
    std::string table_path = db_path + "/" + arguments.create.tbl_name;

    if(fs::exists(table_path)){
        std::cout << "Table " << arguments.create.tbl_name << " already exists!\n";
        exit(3);
    }

    // Create a blank table in the database with the given table name
    std::ofstream tbl(table_path);

    if(arguments.create.additionals.size() > 0){
        std::vector<select_additional_args> constraints;
        for(size_t x = 0; x < arguments.create.additionals.size(); ++x){
            std::stringstream ss(arguments.create.additionals[x]);
            std::string cmd_type;
            ss >> cmd_type;
            
            for (char& c : cmd_type) {
                c = std::tolower(static_cast<unsigned char>(c));
            }
            select_additional_args add_args;

            for(size_t x = 0; x < arguments.create.additionals.size(); ++x){
                fill_in_additional_cmds[cmd_type](arguments.create.additionals[x], add_args);
            }

            if(add_args.curr_type == FROM){
                std::vector<std::vector<std::string>> in_memory_table;
                std::string csv_tbl_name;
                size_t pos = add_args.from.data_source.rfind('/');
                for(size_t q = pos + 1; q < add_args.from.data_source.size() - 4; ++q){
                    csv_tbl_name += add_args.from.data_source[q];
                }

                std::string schema_path = db_path + "/schemas/" + csv_tbl_name;

                if(!fs::exists(schema_path)){
                    std::vector<std::string> column_names;
                    std::ifstream csv_file(add_args.from.data_source);
                    std::string header;

                    std::getline(csv_file, header);
                    std::stringstream header_ss(header);
                    std::string attr;
                    if(add_args.from.header){
                        while(std::getline(header_ss, attr, ',')){
                            column_names.push_back(attr);
                        }

                    }
                    else{
                        std::string tmp_col_name = "column";
                        int counter = 1;
                        while(std::getline(header_ss, attr, ',')){
                            column_names.push_back(tmp_col_name + std::to_string(counter));
                            counter++;
                        }
                    }
                    std::string schema_line;
                    for(size_t col = 0; col < column_names.size(); ++col){
                        schema_line += column_names[col] + "_string,";
                    }

                    std::ofstream schema_file(schema_path);
                    schema_file << schema_line << '\n';
                    schema_file.close();
                }
                in_memory_table = from_qmt(add_args.from.data_source, constraints);

                if(in_memory_table.size() > 0){
                    for (size_t col = 0; col < in_memory_table[0].size(); col++) {
                        for (size_t row = 0; row < in_memory_table.size(); row++) {
                            tbl << in_memory_table[row][col] << ',';
                        }
                        tbl << "\n";
                    }
                }
            }

            executing_line_num++;
        }

    }

    executing_line_num++; // update the execution line number
    return;

}