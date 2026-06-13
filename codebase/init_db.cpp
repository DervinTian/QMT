#include <unistd.h>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "globals.h"

// compile command g++ -std=c++20 init_db.cpp globals.cpp -o init_db
// usage command ./init_db
// example path: /Users/dervint/Desktop/For_Fun/QMT/

// Script in order to initialize the database on disk

namespace fs = std::filesystem;

int main(int argc, char* argv[]){
    std::string qmt_db = "qmt_db";
    std::string input_path;
    // prompt the user for a database name
    std::cout << "Enter full path for the database to be stored: ";
    std::cin >> input_path;
    std::string full_path;

    // create the path for the database to be stored in
    if(input_path.back() == '/'){
        full_path = input_path + qmt_db;
    }  
    else{
        full_path = input_path + "/" + qmt_db;
    }
    
    // try to create the folder for the database
    try {
        fs::create_directory(full_path);
    }
    catch (const fs::filesystem_error& e) {
        std::cout << e.what() << '\n';
        exit(1);
    }

    std::ofstream file("db_path");
    file << full_path;
}