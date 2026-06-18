#pragma once
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cctype>

// All the different keyword types
enum cmd_type{
    SELECT,
    INSERT,
    CREATE,
    ADD_COL,
    UPDATE,
    ALTER,
    DELETE,
    WHERE,
    FROM,
    MATH,
    JOIN,
    ORDER,
    NONE
};

enum math_modes{
    SUBTRACT,
    ADD,
    DIVIDE,
    MULTIPLY,
};

// The compare object types, used in comparisons
enum cmp_object_type{
    STRING,
    INT,
    DOUBLE,
    BOOL,
    CHAR
};

// cmp_return_type, used as a return value for the cmp_object_type
enum cmp_return_type{
    TRUE,
    FALSE, 
    UNKNOWN
};

enum join_types{
    LEFT,
    RIGHT,
    INNER,
    OUTER,
    CROSS
};

enum order_by_directions{
    ASC, 
    DESC
};

// object to be used in the comparisons
struct cmp_object{
    std::string param_string;
    int param_int;
    double param_double;
    bool param_bool;
    char param_char;

    cmp_object_type type;
};

struct column_entry{
    std::string column_name;
    cmp_object_type column_type;
    cmp_object column_value;
};

struct math_args{
    std::vector<std::string> expression_pieces;
    std::vector<std::string> variables;
};

// Data structure to hold arguments for the WHERE command
struct where_args{
    std::string tbl_name;
    std::string lhs_expression;
    std::string rhs_expression;
    std::string comparator;

    math_args left_math;
    math_args right_math;

    cmd_type type;
};

struct from_args{
    std::string data_source;
    int header = 0;
};

struct join_args{
    std::string left_tbl;
    std::string right_tbl;

    where_args on;
    join_types join_type;
};

struct order_args{
    std::string col;
    order_by_directions sort_direction;
};

// Data structure to hold additional arguments as constraints
struct select_additional_args{
    where_args where;
    from_args from;
    join_args join;
    order_args order;

    cmd_type curr_type;
};

// Data structure to hold arguments for the SELECT command
struct select_args{
    std::string tbl_name;
    std::vector<std::string> sel_columns;
    std::vector<std::string> additionals;

    select_additional_args additional_args;
};

// Data structure to hold arguments for the INSERT command
struct insert_args{
    std::string tbl_name;
    std::vector<std::string> values;
};

// Data structure to hold arguments for the CREATE command
struct create_args{
    std::string tbl_name;
    std::vector<std::string> additionals;

    select_additional_args additional_args;
};

// Data structure to hold arguments for the ADDCOL command
struct add_col_args{
    std::string tbl_name;
    std::string column_name;
    std::string type;
};

// Data structure to hold arguments for the UPDATE command
struct update_args{
    std::string tbl_name;

    std::vector<std::pair<std::string, std::string>> set_values;
    
    std::vector<std::string> additionals;
    select_additional_args additional_args;
};

struct alter_args{
    std::string tbl_name;
    std::string column_name;

    // there are a bunch more keywords to follow alter, start with just modifying the schema things, so like modify and rename
    int modify = 0;
    std::string new_column_type;

    int rename = 0;
    std::string new_column_name;
};

// Data structure to hold arguments for the DELETE command
struct delete_args{
    std::string tbl_name;
};

// Data structure to hold all the arguments to be passed into the implementations
struct cmd_args{
    cmd_type cmd;

    select_args select;
    insert_args insert;
    create_args create;
    add_col_args add_cols;
    delete_args deleted;
    update_args update;
    alter_args alter;
};

// Global variables to be used
extern std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, cmd_args&)>> fill_in_cmd;
extern std::unordered_map<std::string, std::function<void(const std::vector<std::string>&, select_additional_args&)>> fill_in_additional_cmds;
extern std::unordered_map<std::string, std::function<void(const cmd_args&)>> cmd_impls;
extern std::unordered_map<std::string, std::function<void(const select_additional_args&)>> additional_cmd_impls;
extern std::unordered_map<std::string, std::function<bool(const std::string&)>> check_value_against_type;
extern std::unordered_map<std::string, std::function<bool(const cmp_object&, const cmp_object&)>> comparators;
extern std::unordered_map<std::string, std::string> alias_to_attr_mapping;
extern std::string db_path;
extern std::vector<std::string> in_memory_script;
extern int executing_line_num;
extern int int_default_value;
extern double double_default_value;
extern std::string str_default_value;
extern char char_default_value;
extern bool bool_default_value;


// Additional keywords that can be used in multiple other operations, WHERE, VALUES, ...
cmp_return_type where_qmt(const select_additional_args &constraint);
std::vector<std::vector<std::string>> from_qmt(const std::string &table_path, const std::vector<select_additional_args> &constraints);
double math_qmt(const std::vector<std::string> &expression_pieces);
std::vector<std::vector<std::string>> join_qmt(const std::vector<select_additional_args> &constraints, std::vector<std::vector<std::string>> &left_tbl, std::vector<std::vector<std::string>> &left_tbl_schema, std::string &join_result_schema);
std::vector<std::vector<std::string>> order_qmt(const std::vector<select_additional_args> &constraints, const std::vector<std::vector<std::string>> &tbl, const std::vector<std::vector<std::string>> &tbl_schema);

// Additional functions to be used
bool valid_table(std::string table);
bool valid_pathname(std::string pathname);
std::string trim_string(std::string value);
std::vector<std::vector<std::string>> read_schema(const std::string &schema_path);
std::vector<std::vector<std::string>> vectorize_schema(const std::string &schema_string);
std::vector<std::vector<std::string>> vectorize_csv(const std::vector<std::string> &csv_format_table);
void display_in_memory_table(const std::vector<std::vector<std::string>> &table, const std::vector<std::vector<std::string>> &schema);
void write_table_to_disk(const std::vector<std::vector<std::string>> &table, std::string tbl_name);