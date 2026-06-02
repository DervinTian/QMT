#pragma once
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "globals.h"

void select_qmt(const cmd_args &arguments);

void insert_qmt(const cmd_args &arguments);

void create_qmt(const cmd_args &arguments);

void add_col_qmt(const cmd_args &arguments);

void delete_qmt(const cmd_args &arguments);