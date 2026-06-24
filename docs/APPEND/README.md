# APPEND # 

## Purpose ##
The purpose of the APPEND keyword is to add additional records on to the end of an existing table in the from of the result of a SELECT query, instead of inserting in individual values with the INSERT keyword.

## Usage ##
```cpp
APPEND (dest_table)
SELECT (...)
...;
...
```
- **dest_table:** represents the name of the table that we want to append to.

After the APPEND keyword should be a SELECT query that should have the same matching schema as the dest_table schema. Whatever is returned from the SELECT query is going to be appended into the table specified by dest_table.

## Example ##

```cpp
APPEND (tbl2)
SELECT (tbl1, *);
```
