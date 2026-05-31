# SELECT # 

## Purpose ##
The purpose of the SELECT keyword is to select certain attributes from a table in a database.

## Usage ##
```cpp
SELECT (table_name, column_name1, column_name2, ...);
```
- **table_name:** represents the name of the table that we want to select column(s) (attribute) from.
- **column_nameXX:** there may be a variable number of column value entries, depending on the schema of the table to be inserted into. Represents the columns to be selected from the table. 
  - To select all of the column names at once, use the * character.
  - The output of select comes in the order in which the column names are entered into the select statement. 
