/*
Implementation for the DELETE function
Arguments:
    - arguments: contains tbl_name to be deleted
*/
void delete_qmt(const cmd_args &arguments){
    std::cout << "Running delete implementation, can fill out semantics later\n";

    // Do error checking to ensure that the database and the tables are valid paths
    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.deleted.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.deleted.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.deleted.tbl_name;
    std::string schema_path = db_path + "/schemas/" + arguments.deleted.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.deleted.tbl_name  << " doesn't exist!\n";
        exit(3);
    }

    // If everything is valid, remove the schema and the data table from the database
    fs::remove(table_path);

    if(fs::exists(schema_path)){
        fs::remove(schema_path);
    }

    executing_line_num++; // update the execution line number
    return;
}