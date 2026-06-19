#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "interpreter.h"

// Functions used to check the type of the value passed as a string

bool check_string(const std::string &value);
bool check_int(const std::string &value);
bool check_double(const std::string &value);
bool check_bool(const std::string &value);
bool check_char(const std::string &value);

// Functions below to fill in the arguments for the major keyword commands

void fill_select_args(const std::vector<std::string> &command, cmd_args &args);
void fill_insert_args(const std::vector<std::string> &command, cmd_args &args);
void fill_create_args(const std::vector<std::string> &command, cmd_args &args);
void fill_add_col_args(const std::vector<std::string> &command, cmd_args &args);
void fill_update_args(const std::vector<std::string> &command, cmd_args &args);
void fill_alter_args(const std::vector<std::string> &command, cmd_args &args);
void fill_delete_args(const std::vector<std::string> &command, cmd_args &args);
void fill_copy_args(const std::vector<std::string> &command, cmd_args &args);

void fill_where_args(const std::vector<std::string> &command, select_additional_args &args);
void fill_from_args(const std::vector<std::string> &command, select_additional_args &args);
void fill_join_args(const std::vector<std::string> &command, select_additional_args &args);
void fill_order_args(const std::vector<std::string> &command, select_additional_args &args);