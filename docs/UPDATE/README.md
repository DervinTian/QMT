# UPDATE # 

## Purpose ##
The purpose of the Update feature is to modify values within the selected table.

## Usage ##
```cpp
UPDATE (table_name)
SET (col_name, new_value);
...
```
- **table_name:** represents the name of the table that we want to alter.
- **col_name:** represents the name of the column that we want to update the value for.
- **new_value:** represents the new value that we want to insert into the new column.

Following the SET statement(s) can be a constraint using the WHERE keyword.

```cpp
UPDATE (table_name)
SET (col_name, new_value);
...
WHERE (left_expression, comparator, right_expression, ...);
```
- **left_expression:** represents the left hand side expression to be evaluated, similar to how regular SQL works.
- **comparator:** represents the comparison to be performed between the left and the right expressions:
  - Accepted comparators include:
    - EQUAL or =
    - NOT_EQUAL or !=
    - LESS or <
    - LESS_THAN_OR_EQUAL_TO or <=
    - GREATER or >
    - GREATER_THAN_OR_EQUAL_TO or >=
- **right_expression:** represents the right hand side expression to be evaluated, similar to how regular SQL works.

## Example ##
```cpp
UPDATE (test_tbl)
SET (name, "JOHN")
SET (age, 25)
WHERE (name, EQUAL, "John");
```
