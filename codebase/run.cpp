#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include "interpreter.h"

// compile command g++ run.cpp interpreter.cpp -o run
// usage cmd ./run [INSERT QMT FILENAME]

int main(int argc, char* argv[]){
    if(argc < 2){
        std::cout << "No QMT script provided to run!\n";
        exit(1);
    }

    std::string filename = argv[1];

    std::ifstream file(filename);
    if(!file){
        std::cout << "Input file failed to open!\n";
        exit(1);
    }

    std::string line; // hold each QMT line
    std::vector<std::string> command; // hold a chunk of QMT lines that represents a command, ends with ";"
    int line_num = 1;
    while(std::getline(file, line)){
        command.push_back(line);
        if(!end_statement(line)){
            line_num++;
        }
        else{

        }
    }


}