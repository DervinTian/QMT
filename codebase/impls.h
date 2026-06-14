#pragma once
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "globals.h"

// All the implementations of the keyword commands so far

void select_qmt(const cmd_args &arguments);
void insert_qmt(const cmd_args &arguments);
void create_qmt(const cmd_args &arguments);
void update_qmt(const cmd_args &arguments);
void alter_qmt(const cmd_args &arguments);
void add_col_qmt(const cmd_args &arguments);
void delete_qmt(const cmd_args &arguments);