#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "interpreter.h"
#include "globals.h"

// compile command g++ -std=c++20 run.cpp interpreter.cpp globals.cpp impls.cpp fill_args.cpp -o run
// usage cmd ./run [INSERT QMT FILENAME]

int main(int argc, char* argv[]){
    if(argc < 2){
        std::cout << "No QMT script provided to run!\n";
        exit(1);
    }

    std::string filename = argv[1];

    std::ifstream file(filename);
    if(!file){
        std::cout << "Input file (path: " << filename << ") failed to open!\n";
        exit(1);
    }

    std::ifstream database_name("database_name.txt");
    database_name >> db_path;

    init_function_map();
    init_impl_map();
    init_check_map();

    std::string line; // hold each QMT line
    std::vector<std::string> command; // hold a chunk of QMT lines that represents a command, ends with ";"
    int line_num = 1;
    
    while(std::getline(file, line)){
        if(line[0] == '#'){
            continue;
        }
        command.push_back(line);
        if(end_statement(line)){
            if(!run_interpreter(command)){
                std::cout << "Error in command ending at line " << line_num << std::endl;
                    exit(1);
                }
            command.clear();
        }
        line_num++;
    }

}