# SQLFunction

* `fetch_row_value`
    * ```if ((mysql_row = mysql_fetch_row(mysql_result)) != NULL)``` determine there is no data in table or not
      * Have data `error = 0`
      * No data `error = -1`
    1. After ```SELECT``` SQL sentence finish, should use `turn_int` or `turn_float` to transfor data type

* `sent_query`

* `turn_int`
    * Purpose : Return Int Type
    * Data is NULL `return -999`
    1. Turn data type to int

* `turn_float`
    * Purpose : Return Float Type
    * Data is NULL `return -999`
    1. Turn data type to float

* `turn_value_to_float`
    * Purpose : Return Float Type
    * No data `return -404`
    1. Combine `fetch_row_value` and `turn_float`

* `turn_value_to_int`
    * Purpose : Return Int Type
    * No data `return -404`
    1. Combine `fetch_row_value` and `turn_int`

* messagePrint
    1. Input `__LINE__`
    1. Input ` ex: "Pgrid max = "`
    1. Input `'I', 'F', 'S'`
    1. Input `{variable_name}` 
    1. Input `'Y', 'N'`

* functionPrint
    1. Input `__func__`

* find_variableName_position
    * Purpose : Return array position
    1. Input your scheduling variable array. ex: `variable_name` in `GHEMS`
    2. Input scheduling variable array target. ex: `"Pgrid"` in `variable_name` in `GHEMS`

* demand_response_info
    * Get demand response
    1. start time
    2. end time 
    3. min decrease power
    4. min feedback price
    5. customer base line

* flag_recevie
    * Use for receive `GHEMS_flag` & `LHEMS_flag` variable `flag` status
    1. Input `table name`
    2. Input `table column name`

* value_receive
    * Use for receive value status
    1. Input `table name`
    2. Input `table column name`
    3. Input `table column text name` or `table column number`
    4. Input `'F'` to return float type