/*
Implementation for the ADDCOL function
Arguments:
    - arguments: contains tbl_name, column name, and the column type to be inserted
*/
void add_col_qmt(const cmd_args &arguments){
    std::cout << "Running addcol implementation, can fill out semantics later\n";

    // Check to make sure that all the names and paths are valid
    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.create.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.add_cols.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.add_cols.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.add_cols.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.create.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    // Create a schema if there doesn't exist one already
    if(!fs::exists(schema_path)){
        std::ofstream schema(schema_path);
    }

    // construct the comma-delimited value string to be inserted into the database schema
    std::string line;
    std::ifstream schema(schema_path);
    while(std::getline(schema, line));
    schema.close();

    line += arguments.add_cols.column_name + "_" + arguments.add_cols.type + ",";

    std::ofstream out_schema(schema_path);
    out_schema << line;

    out_schema.close();

    executing_line_num++; // update the execution line number
    return;

}