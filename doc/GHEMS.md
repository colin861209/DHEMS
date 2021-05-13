# GHEMS

* determine_realTimeOrOneDayMode_andGetSOC
    * Return real_time '1'
    * Case:
    1. real_time = 0 
        * truncate tables (contorl_status, real_status, cost)
        * SOC = 0.7
        * simulate time block = 0
        * Update real time = 1

    2. real_time = 1
        * truncate table (real_status)
        * Get previous SOC value to be current SOC

* getOrUpdate_SolarInfo_ThroughSampleTime
    * Input `'big_sunny' 'sunny' 'cloudy'` determine by table `solar_data`
    * Case:
    1. sample_time = 0
        * Get data from `solar_data` And Update to `solar_day`

    2. sample_time = 1
        * Get data from `solar_day`

* calculateCostInfo
    * Calculate cost information then input to `updateTableCost`

* updateTableCost
    * Input cost information 
    * Table `cost`
        1. `Load comsuption` in each time block
        1. `Load comsuption * electric price` in each time block
        1. `Grid buy * electric price` in each time block
        1. `Grid sell * electric price` in each time block
        1. `Total fuel cell power * price` in each time block
        1. `Grams of Hydrogen cost from total fuel cell power` in each time block
    * Table `BaseParameter`
        1. Sum `Load comsuption` 
        1. Sum `Load comsuption * electric price` 
        1. Sum `Grid buy * electric price` 
        1. Sum `Grid sell * electric price` 
        1. Sum `Total fuel cell power * price` 
        1. Sum `Grams of Hydrogen cost from total fuel cell power` 
        
    * Case:
    1. sample_time = 0
        * `Insert` to table `cost`
        * `Update` to `BaseParameter` corressponse column
    1. sample_time = 1
        * `Update` to table `cost`
        * `Update` to `BaseParameter` corressponse column

* insert_GHEMS_variable
    * Record CEMS enerage limit, due to `BaseParameter` is for HEMS
    
* getPrevious_battery_dischargeSOC
    * Calculate `ESS discharge SOC` amount until sample time
    * Calculate `Total discharge SOC` amount for all day SOC, it's for preview, can be ignore