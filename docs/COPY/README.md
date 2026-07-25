# COPY # 

## Purpose ##
The purpose of the COPY keyword is to copy a table across to another table. If the new table that we want to copy to does not exist, the COPY keyword will create one automatically.

## Usage ##
```cpp
COPY (orig_table, copy_table);
...
```
- **orig_name:** represents the name of the table that we want to copy.
- **copy_name:** represents the name of the table that we want the table to be copied to.
