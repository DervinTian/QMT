#pragma once
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

#include "globals.h"

void init_function_map();

bool end_statement(std::string line);

bool run_interpreter(int starting_line, int end_line);

