# fifty_LHEMS_function

> `connect_mysql(DB_name)`
  * Input `DB name`, can use in any code
  * Output `1` = `Success`, `-1` = `Error`

> `determine_realTimeOrOneDayMode_andGetSOC(int real_time, vector<string> variable_name, int distributed_group_num)`
  * When `real_time` is 1
    * When function `truncate_table_flag` is 1
      * Truncate table `LHEMS_real_status`
    * When `Pess_flag` is 1
      * Get previous SOC
  * When `real_time` is not 1
    * When function `truncate_table_flag` is 1
      * Truncate table
        1. LHEMS_control_status
        2. LHEMS_real_status
      * Update value = 0 in table `distributed_group`
        1. real_time
        2. next_simulate_timeblock
    * When `Pess_flag` is 1
      * Get ini_SOC
    * When `distributed_household_id` = `distributed_householdTotal`
      * `real_time` = 1
      * Update `real_time` to 1 in table `distributed_group`
      * When `COUNT(group_id) = SUM(real_time)`
        * Update `real_time` to 1 in table `BaseParameter`

> `countUninterruptAndVaryingLoads_Flag(int *uninterrupt_flag, int *varying_flag, int household_id)`
> `countLoads_AlreadyOpenedTimes(int *buff, int household_id)`
> `count_interruptLoads_RemainOperateTime(int interrupt_num, int *interrupt_ot, int *interrupt_reot, int *buff)`
> `count_uninterruptAndVaryingLoads_RemainOperateTime(int group_id, int loads_total, int *total_operateTime, int *remain_operateTime, int *end_time, int *flag, int *buff)`
> `init_VaryingLoads_OperateTimeAndPower(int **varying_t_d, float **varying_p_d, int *varying_ot)`
> `putValues_VaryingLoads_OperateTimeAndPower(int **varying_t_d, float **varying_p_d, int **varying_t_pow, float **varying_p_pow, int *varying_start, int *varying_end, float *varying_p_max)`
  * Nothing change, same as the five household version

> `update_loadModel(*interrupt_p, *uninterrupt_p, household_id, distributed_group_num)`
  * Almost same as the five household LHEMS version
  * When `distributed_household_id` = `distributed_householdTotal`
    * Update `total_load_flag` to 1 in table `distributed_group`
    * When `COUNT(group_id) = SUM(total_load_flag)`
      * Update `totalLoad` in table `totalLoad_model`

> `rand_operationTime(distributed_group_num)`
  * When `uncontrollable_load_flag` in table `BaseParameter` is 0
    * Update `uncontrollable_load_flag` to 0 in table `distributed_group`
    * Update `household{real number}` in `LHEMS_uncontrollable_load`
    * When `distributed_household_id` = `distributed_householdTotal`
      * Update `totalLoad` to 0 in table `LHEMS_uncontrollable_load`
    * Output `array size = 96, value = 0`
  * When `sample_time` is 0
    * Generate uncontrollable load, same as five household LHEMS version
    * When `distributed_household_id` = `distributed_householdTotal`
      * Update `uncontrollable_load_flag` to 1 in table `distributed_group`
      * When `COUNT(group_id)` = `SUM(uncontrollable_load_flag)`
        * Update `totalLoad` in table `LHEMS_uncontrollable_load`
  * `sample_time` is not 0
    * Get old uncontrollable load data
  * Output array `size = 96, value = {uncontrollable load}`

> `household_weighting()`
  * No change yet, same as the five household LHEMS version

> `truncate_table_flag()`
  * Verify `COUNT(group_id) = SUM(household_id)`
  * Every LHEMS has own flag but don't truncate common table every time, and `household_id` will start at 1, so truncate table when all `household_id` is 1
  * Output `return int` 1 is TRUNCATE

> `get_distributed_group(target, condition_col = "", condition_num = -1)`
  * Input `target`, Between `SELECT` and `FROM`
  * Input `condition_col` can be empty if don't need, Between `WHERE` and `=`
  * Input `condition_num` can be empty, default is -1 (due to all value in table `distribted_group` won't be -1), After `{condition_col} = `
  * Output `return int`, if need return `float type` should function `Overloading`

> `update_distributed_group(target, target_value, condition_col, condition_num)`
  * Input `target`, should be exist in table `distributed_group` column, Between `SET` and `=`
  * Input `target_value`, Between `{target} = ` and `WHERE`
  * Input `condition_col`, Between `WHERE` and `=`
  * Input `condition_num`, After `{condition_col} = `

> `init_totalLoad_flag_and_table(distributed_group_num)`
  * Input `distributed_group_num`
  * When `distributed_household_id` is 1 
    * Update `total_load_flag` to 0 in table `distributed_group`
    * When `sample_time` is 0
      * Update `household{real number}` to 0 in table `totalLoad_model`
      * Update `totalLoad` to 0 in table `totalLoad_model`