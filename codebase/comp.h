#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

#include "globals.h"

bool comp_equal(const cmp_object &lhs, const cmp_object &rhs);

bool comp_less(const cmp_object &lhs, const cmp_object &rhs);

bool comp_greater(const cmp_object &lhs, const cmp_object &rhs);

bool comp_less_than_or_equal_to(const cmp_object &lhs, const cmp_object &rhss);

bool comp_greater_than_or_equal_to(const cmp_object &lhs, const cmp_object &rhs);

bool comp_not_equal(const cmp_object &lhs, const cmp_object &rhs);