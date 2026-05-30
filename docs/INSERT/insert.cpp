void insert_qmt(cmd_args arguments){
    std::cout << "Running insert implementation, can fill out semantics later\n";

    if(!valid_pathname(db_path)){
        std::cout << "Invalid database (pathname is: " << db_path << ") detected. Did you forget to run init_db?\n";
        exit(1);
    }

    if(!valid_table(arguments.insert.tbl_name)){
        std::cout << "Invalid tablename (tablename is: " << arguments.insert.tbl_name << ") detected.\n";
        exit(2);
    }

    std::string table_path = db_path + "/" + arguments.insert.tbl_name;

    if(!fs::exists(table_path)){
        std::cout << "Table " << arguments.insert.tbl_name << " doesn't exist!\n";
        exit(3);
    }

    std::ofstream tbl(table_path, std::ios::app);

    for(size_t i = 0; i < arguments.insert.values.size(); ++i){
        tbl << arguments.insert.values[i];
        if(i != arguments.insert.values.size() - 1){
            tbl << ',';
        }
    }

    tbl << '\n';
    tbl.close();
}