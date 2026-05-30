void delete_qmt(cmd_args arguments){
    std::cout << "Running delete implementation, can fill out semantics later\n";

    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.deleted.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.deleted.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.deleted.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.deleted.tbl_name  << " doesn't exist!\n";
        exit(3);
    }

    fs::remove(table_path);
}