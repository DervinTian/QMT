# JOIN # 

## Purpose ##
The purpose of the ON keyword is used in the JOIN command to specify the condition on which we want to join.

## Usage ##
```cpp
ON (left_expression, comparator, right_expression);
```
- **join_type:** represents the join that we want to perform. The list of currently implemented join types are:
    - JOIN INNER: Only records in both tables that are good for the matching condition are kept, others are discarded.
- **table_name:** the second table that we want to be joining on. The first table is inheretied from the FROM statement above the JOIN statement.
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
JOIN INNER (test_tbl1)
ON (desc, =, column3);
```
