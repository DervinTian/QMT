#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "interpreter.h"

void fill_select_args(const std::vector<std::string> &command, cmd_args &args);

void fill_insert_args(const std::vector<std::string> &command, cmd_args &args);

void fill_create_args(const std::vector<std::string> &command, cmd_args &args);

void fill_add_col_args(const std::vector<std::string> &command, cmd_args &args);

void fill_delete_args(const std::vector<std::string> &command, cmd_args &args);