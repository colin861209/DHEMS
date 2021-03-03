# SQLFunction

* `fetch_row_value`
    1. After ```SELECT``` SQL sentence finish, should use `turn_int` or `turn_float` to transfor data type

* `sent_query`

* `turn_int`
    * Purpose : Return Int Type
    1. Turn data type to int

* `turn_float`
    * Purpose : Return Float Type
    1. Turn data type to float

* `turn_value_to_float`
    * Purpose : Return Float Type
    1. Combine `fetch_row_value` and `turn_float`

* `turn_value_to_int`
    * Purpose : Return Int Type
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