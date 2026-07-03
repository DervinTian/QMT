#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>

#include "globals_disk.h"

// Create our own simple disk to use for the database: Each table will have an inode, and then each table will have it's own disk blocks
// It's essentially a filesystem without directories

/*
Inode Layout:
size:
owner? Could be useful cuz I was thinking that we could somehow like save an image of the database and then it could be transported and read in
child_disk_blocks:
*/

// compile cmd: g++ -g -std=c++20 initialize_disk.cpp -o initialize_disk

namespace fs = std::filesystem;

int main() {
    const size_t size = DISK_SIZE; // 1 MB

    if(fs::exists("vm_disk.bin")){
        std::cout << "Virtual disk already exists!\n";
        exit(100);
    }

    std::ofstream file("vm_disk.bin", std::ios::binary);
    std::vector<char> buffer(size, 0);

    file.write(buffer.data(), buffer.size());
    file.seekp(0);

    char starting_type = 'i';
    file.write(reinterpret_cast<const char*>(&starting_type), sizeof(starting_type));
}