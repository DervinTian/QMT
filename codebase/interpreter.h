#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

enum cmd_type{
    SELECT,
    INSERT,
    DELETE,
};

struct cmd_args{
    cmd_type cmd;

    std::vector<std::string> sel_columns;
    std::string data_dest;
};

bool end_statement(std::string line);

bool run_interpreter(std::vector<std::string> command);

void select_qmt(cmd_args arguments);

void insert_qmt(cmd_args arguments);

void delete_qmt(cmd_args arguments);