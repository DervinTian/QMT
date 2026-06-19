/*
Implementation for the COPY function
Arguments:
    - arguments: contains the table that we want to copy, and the table where want it to be copied to
*/
void copy_qmt(const cmd_args &arguments){
    std::cout << "Running delete implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number

    // Do error checking to ensure that the database and the tables are valid paths
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }

    if(!valid_table(arguments.copy.orig_table)){
        exit_with_error(INVALID_TABLENAME, arguments.copy.orig_table);
    }

    if(!valid_table(arguments.copy.copy_table)){
        exit_with_error(INVALID_TABLENAME, arguments.copy.orig_table);
    }

    std::string orig_table_path = db_path + "/" + arguments.copy.orig_table;
    std::string orig_schema_path = db_path + "/schemas/" + arguments.copy.orig_table;

    std::string copy_table_path = db_path + "/" + arguments.copy.orig_table;
    std::string copy_schema_path = db_path + "/schemas/" + arguments.copy.orig_table;

    if(!fs::exists(orig_table_path)){
        exit_with_error(NULL_TABLE, arguments.copy.orig_table);
    }

    if(!fs::exists(copy_table_path)){
        exit_with_error(NULL_TABLE, arguments.copy.orig_table);
    }

    // read in schema, so that we can compare and make sure that we can actually copy the tables over
    std::vector<std::string> original_tbl_schema_types = read_schema(orig_schema_path)[1];
    std::vector<std::string> copy_tbl_schema_types = read_schema(copy_schema_path)[1];

    if(original_tbl_schema_types.size() != copy_tbl_schema_types.size()){
        exit_with_error(DIFF_SCHEMAS, "");
    }

    // Go through the types and if there is a type conversion that cannot be made, then error out
    // Also copy values completely later, so that we can avoid having partially copied over tables and stuff
    for(size_t i = 0; i < original_tbl_schema_types.size(); ++i){
        if(copy_tbl_schema_types[i] == "string"){
            continue; // Anything can convert into a string
        }
        else if(copy_tbl_schema_types[i] == "char"){
            if(original_tbl_schema_types[i] != "char"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else if(copy_tbl_schema_types[i] == "bool"){
            if(original_tbl_schema_types[i] != "bool"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else if(copy_tbl_schema_types[i] == "int" || copy_tbl_schema_types[i] == "double"){
            if(original_tbl_schema_types[i] != "int" && original_tbl_schema_types[i] != "double"){
                exit_with_error(DIFF_SCHEMAS, "");
            }
        }
        else{
            exit_with_error(UNKNOWN_TYPE, copy_tbl_schema_types[i]);
        }
    }

    // empty constraints for now, but actually could be a good idea to have some constraints, like only copy select columns over
    std::vector<std::vector<std::string>> original_table = from_qmt(orig_table_path, std::vector<select_additional_args>{});

    write_table_to_disk(original_table, arguments.copy.copy_table);
    return;
}