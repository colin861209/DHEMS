# LHEMS

* determine_realTimeOrOneDayMode_andGetSOC
    * Return real_time '1'
    * NOTE: variable `same_day` isn't use in simulation
    * Case:
    1. real_time = 0 
        1. household_id = 1
            * truncate tables (contorl_status, real_status)
            * simulate time block = 0
        2. household_id = householdTotal
            * real_time = 1

    2. real_time = 1
        * truncate table (real_status)

* getOrUpdate_SolarInfo_ThroughSampleTime
    * Input `'big_sunny' 'sunny' 'cloudy'` determine by table `solar_data`
    * Case:
    1. sample_time = 0
        * Get data from `solar_data` And Update to `solar_day`

    2. sample_time = 1
        * Get data from `solar_day`

* countUninterruptAndVaryingLoads_Flag
    * Calculate flag is 0 or 1 with variable `delta in uninterrupt and varying` 

* countLoads_AlreadyOpenedTimes
    * Count each load operated times

* count_interruptLoads_RemainOperateTime
* count_uninterruptAndVaryingLoads_RemainOperateTime
    * Count each load remain times

* putValues_VaryingLoads_OperateTimeAndPower
    * Set varying load operate time and power model

* update_loadModel
    * Update all household loads' comsuption in each time block

* rand_operationTime
    * Uncontrollable load generate
    * Input starting time block 
    * random target 
        1. operation time
        2. power