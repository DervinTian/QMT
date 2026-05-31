# WHERE # 

## Purpose ##
The purpose of the WHERE keyword is to act as an extension of the SELECT keyword to select columns, given certain conditions specified by the WHERE keyword.

## Usage ##
```cpp
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
WHERE (name, EQUAL, "John");
```

_Note: The table name is derived from the select statement above._
    
