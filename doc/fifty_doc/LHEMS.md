# LHEMS
### 10 LHEMS to `parallel process`
### `for loop` execute times is `5` in shell script `main_fifty`

* Amount of LHEMS code should same as the parameter `householdDistributed` in table `BaseParameter`
* Change the `for loop` execute times in shell script `main_fifty` if execute times not the same as `householdAmount/householdDistributed`
* `household_id` & `householdTotal` is household real number, for currnt is like `1~5`, `6~10` etc...
* `distributed_household_id` & `distributed_householdTotal` is household range number, for currnt range is like `1~5`, calculate by parameter `householdAmount/householdDistributed`
* Every supply power is `origin power * 5` for current
* Update own information to table `distributed_group`
  1. real_time
  2. household_id
  3. next_simulate_timeblock
  4. now_SOC
  5. total_load_flag
  6. uncontrollable_load_flag

* Will update `next_simulate_timeblock` to table `BaseParameter` when all groups were updated to the same value in table `distributed_group`