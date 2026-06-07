# INSERT # 

## Purpose ##
The purpose of the INSERT keyword is to add in a new entry/record into the table.

## Usage ##
```cpp
VALUES (column_value1, column_value2, ...);
```

- **column_value:** represents the values to be inserted into the table. Note that for the specified table, the VALUES keyword must provide a value for each of the columns in the table.

## Example ##
For a table with name, age, and description as the columns, the following insert would be:
```cpp
INSERT ("Jimmy", 26, "YouTuber");
```


_Note: The table name is derived from the INSERT statement above the VALUES statment(s)._
    
