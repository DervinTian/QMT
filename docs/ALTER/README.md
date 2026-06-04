# ALTER # 

## Purpose ##
The purpose of the ALTER feature is to modify the selected table, such as renaming a column, or changing the type of a column.

## Usage ##
```cpp
ALTER (table_name);
...
```
- **table_name:** represents the name of the table that we want to alter.

Following the ALTER statement can be a list of different keywords. Currently supported keywords include:

```cpp
ALTER (table_name)
RENAME (old_column_name, new_column_name);
```
- **old_column_name:** represents the existing column within the table that we would like to rename.
- **new_column_name:** represents the new column name that we would like to change the existing one to.

```cpp
ALTER (table_name)
MODIFY (column_name, new_column_type);
```
- **old_column_name:** represents the existing column within the table that we would like to change the type of.
- **new_column_name:** represents the type that we would like to change the existing column to.
