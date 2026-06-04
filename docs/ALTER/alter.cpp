/*
Implementation for the ALTER function. Allows the user to modify the table.
Arguments:
    - arguments: contains tbl_name, column name, and the column type to be inserted
        Supported keywords following alter include:
            - RENAME: rename the column
            - MODIFY: change the type of the column
*/
void alter_qmt(const cmd_args &arguments){
    std::cout << "Running alter implementation, can fill out semantics later\n";

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.create.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.alter.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.alter.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.alter.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.alter.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    if(arguments.alter.rename == 1){
        std::cout << "Column to be modified is: " << arguments.alter.column_name << " and modify it to: " << arguments.alter.new_column_name << std::endl;
        // Create a schema if there doesn't exist one already
        if(!fs::exists(schema_path)){
            std::cout << "Schema for Table " << arguments.alter.tbl_name << " doesn't exist!\n";
            exit(4);
        }

        std::ifstream schema_file(schema_path);
        std::string schema_line;

        std::vector<std::string> column_names;
        std::vector<std::string> column_types;

        std::getline(schema_file, schema_line);
        schema_file.close();

        std::stringstream comma_separated_values(schema_line);
        std::string name_type;
        while(std::getline(comma_separated_values, name_type, ',')){
            std::stringstream underscore_separated_values(name_type);
            std::string attr;

            int name_mode = 1;
            while(std::getline(underscore_separated_values, attr, '_')){
                if(name_mode){
                    // attr represents the name of the column
                    if(attr == arguments.alter.column_name){
                        column_names.push_back(arguments.alter.new_column_name);
                    }
                    else{
                        column_names.push_back(attr);
                    }
                }
                else{
                    column_types.push_back(attr);
                }
                name_mode = (name_mode + 1) % 2;
            }
        }

        std::ofstream schema_output_file(schema_path);
        for(size_t i = 0; i < column_names.size(); ++i){
            schema_output_file << column_names[i] + "_" + column_types[i] << ",";
        }

        executing_line_num++; // update the execution line number
    }
    else if(arguments.alter.modify == 1){
        std::cout << "Column to be modified is: " << arguments.alter.column_name << " and modify it to: " << arguments.alter.new_column_type << std::endl;
         // Create a schema if there doesn't exist one already
        if(!fs::exists(schema_path)){
            std::cout << "Schema for Table " << arguments.alter.tbl_name << " doesn't exist!\n";
            exit(4);
        }

        std::ifstream schema_file(schema_path);
        std::string schema_line;

        std::vector<std::string> column_names;
        std::vector<std::string> column_types;

        std::getline(schema_file, schema_line);
        schema_file.close();

        std::stringstream comma_separated_values(schema_line);
        std::string name_type;
        while(std::getline(comma_separated_values, name_type, ',')){
            std::stringstream underscore_separated_values(name_type);
            std::string attr;

            int type_mode = 0;
            bool right_column = false;
            while(std::getline(underscore_separated_values, attr, '_')){
                if(type_mode){
                    // attr represents the name of the column
                    if(right_column){
                        column_types.push_back(arguments.alter.new_column_type);
                        right_column = false;
                    }
                    else{
                        column_types.push_back(attr);
                    }
                }
                else{
                    if(attr == arguments.alter.column_name){
                        right_column = true;
                    }
                    column_names.push_back(attr);
                }
                type_mode = (type_mode + 1) % 2;
            }
        }

        std::ofstream schema_output_file(schema_path);
        for(size_t i = 0; i < column_names.size(); ++i){
            schema_output_file << column_names[i] + "_" + column_types[i] << ",";
        }

        executing_line_num++; // update the execution line number
    }

    executing_line_num++; // update the execution line number
    return;
}