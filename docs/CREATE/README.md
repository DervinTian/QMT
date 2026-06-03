# CREATE # 

## Purpose ##
The purpose of the CREATE keyword is to create a new blank table within the database.

## Usage ##
```cpp
CREATE (table_name);
```
- **table_name:** represents the name of the table that we want to create within our database.

Additionally, the CREATE keyword can also be used to create a QMT table from an input .csv file
using the FROM keyword to specify where to read the data in from.

```cpp
CREATE (table_name)
FROM (path_to_csv) HEADER;
```

- **table_name:** represents the name of the table that we want to create within our database.
- **path_to_csv:** represents the path to the csv file that we want to read in from.
- **HEADER:** Can be either HEADER or NO_HEADER. It represents an additional optional argument specifying whether or not the column headers are at the top of the csv file. If not specified, the default behavior is to use NO_HEADER.

