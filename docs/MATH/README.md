# MATH # 

## Purpose ##
The purpose of the MATH keyword is to allow the use of mathematical expressions when comparing values, such as in a WHERE clause.

## Usage ##
```cpp
MATH (math_expression, variable1, ...);
```
- **math_expression:** represents the math expressions that we want to run, with ? denoting a variable place holder, to be specified later in the variables argument.
- **variableX:** represents the variable (column value) that we want to fill in into the expression at the location specified by the ? symbol.

## Example ##
```cpp
MATH (? - 5, age);
```

The placement of the ? goes hand-in-hand with the order of the variables specified. That is to say, the first ? will go with the first variable specified, the second ? with the second variable and so on.

_Note: The usage of the MATH operator is very similar to printf in C._
    
