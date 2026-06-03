# FROM # 

## Purpose ##
The purpose of the FROM keyword is to specify the name of a data source where we want to get the data from. It can either be in the form of a QMT table name, or a csv file path.

## Usage ##
```cpp
FROM (data_source) HEADER;
```
- **data_source:** represents the name of either the QMT table or the path to the csv file.
- **HEADER:** Can be either HEADER or NO_HEADER. It represents an additional optional argument specifying whether or not the column headers are at the top of the csv file. If not specified, the default behavior is to use NO_HEADER.

## Example ##
```cpp
FROM (table.csv) HEADER;
```

_Note: The table name is derived from the select statement above._
    
