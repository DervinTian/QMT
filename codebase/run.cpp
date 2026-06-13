#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "interpreter.h"
#include "globals.h"

// compile command g++ -g -std=c++20 run.cpp interpreter.cpp globals.cpp impls.cpp fill_args.cpp comp.cpp -o run
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

    std::ifstream database_name("db_path");
    database_name >> db_path;

    init_function_map();

    std::string line; // hold each QMT line
    std::vector<std::string> command; // hold a chunk of QMT lines that represents a command, ends with ";"

    while(std::getline(file, line)){
        in_memory_script.push_back(line);
    }

    int next_starting_line_num = 0;
    int prev_starting_line_num = 0;
    for(size_t i = 0; i < in_memory_script.size(); ++i){
        
        // if(i != executing_line_num){
        //     i = executing_line_num;
        // }

        std::string script_line = in_memory_script[i];
        if(script_line[0] == '#'){
            continue;
        }
        if(end_statement(script_line)){
            next_starting_line_num = i;
            if(!run_interpreter(prev_starting_line_num, i)){
                std::cout << "Error in command ending at line " << i << std::endl;
                exit(1);
            }

            prev_starting_line_num = next_starting_line_num + 1;
 
        }

        if(prev_starting_line_num >= in_memory_script.size()){
            break;
        }

    }

}