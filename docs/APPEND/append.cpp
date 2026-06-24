/*
Implementation for the APPEND function
Arguments:
    - arguments: contains the table that we want to move, and the table where want it to be moved to
*/
void append_qmt(const cmd_args &arguments){
    std::cout << "Running append implementation, can fill out semantics later\n";
    executing_line_num++; // update the execution line number

    // Do error checking to ensure that the database and the tables are valid paths
    if(!valid_pathname(db_path)){
        exit_with_error(INVALID_DATABASE_NAME, "");
    }

    if(!valid_table(arguments.append.dest_table)){
        exit_with_error(INVALID_TABLENAME, arguments.append.dest_table);
    }

    std::string dest_table_path = db_path + "/" + arguments.append.dest_table;
    std::string dest_schema_path = db_path + "/schemas/" + arguments.append.dest_table;

    if(!fs::exists(dest_table_path)){
        exit_with_error(NULL_TABLE, arguments.move.dest_table);
    }

    std::vector<std::string> dest_schema_types = read_schema(dest_schema_path)[1];

    cmd_args inner_select_args;
    inner_select_args.select = arguments.append.select;
    select_qmt(inner_select_args);
    std::cout << "Here\n";
    std::cout << "#: " << dest_schema_types.size() << std::endl;
    std::cout << "$: " << intermediate_results.buffer_table_schema[1].size() << std::endl;
    check_compatible_schemas(dest_schema_types, intermediate_results.buffer_table_schema[1]);

    std::vector<std::vector<std::string>> &table = intermediate_results.buffer_table;
    for (size_t col = 0; col < table[0].size(); col++) {
        cmd_args insert_args;
        insert_args.insert.tbl_name = arguments.append.dest_table;
        for (size_t row = 0; row < table.size(); row++) {
            std::cout << table[row][col] << std::endl;
            insert_args.insert.values.push_back(table[row][col]);
        }

        insert_qmt(insert_args);

        std::cout << "\n";
    }
}