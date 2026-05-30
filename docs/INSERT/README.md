# INSERT # 

## Purpose ##
The purpose of the INSERT keyword is to insert a new entry into the database table.

## Usage ##
```cpp
INSERT (table_name, column_value1, column_value2, ...);
```
- **table_name:** represents the name of the table that we want to add column (attribute) to.
- **column_valueXX:** there may be a variable number of column value entries, depending on the schema of the table to be inserted into. Represents the values to be inserted into the table, require one value for each attribute in order of the schema to be inserted.
