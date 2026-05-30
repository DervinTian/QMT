#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "interpreter.h"

bool check_string(std::string &value);

bool check_int(std::string &value);

bool check_bool(std::string &value);

bool check_char(std::string &value);

void fill_select_args(const std::vector<std::string> &command, cmd_args &args);

void fill_insert_args(const std::vector<std::string> &command, cmd_args &args);

void fill_create_args(const std::vector<std::string> &command, cmd_args &args);

void fill_add_col_args(const std::vector<std::string> &command, cmd_args &args);

void fill_delete_args(const std::vector<std::string> &command, cmd_args &args);