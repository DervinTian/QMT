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

    executing_line_num++; // update the execution line number
    return;

}