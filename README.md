# DHEMS

###### tags: `DHEMS`

* [Mathematical Formula Link](https://hackmd.io/pvujnbJeQf6bXQqIibQXXQ)
* Function introduction in `doc` file

### STEP :
1. Should modify for loop j in 1 to households' amount in `main.sh`

2. LHEMS calculate the `loads deployment` in purpose, and update total loads consumption.

3. GHEMS calculate the `optimized enerage supply` in purpose, and calculate the price cost in whole day.

---

## 2021/02/24

+ Commit link [87c3513](https://github.com/colin861209/DHEMS/commit/87c3513ec9aef53bef73158f49a007f15179f212)

* Merge branch 'develop'
  * Content: 
    1. Psell in GLPK
    2. Set weather & Hydro_Price from shell script main

---
## 2021/03/02

+ Commit link [e012dda](https://github.com/colin861209/DHEMS/commit/e012dda857c830e031a4a65dde8bc8a9f27f7d63)

* Content:
  * LHEMS:
    1. Uncontrollable load generate
  * GHEMS:
    1. Insert variable to table

---
## 2021/03/30

+ Commit link [8c84b57](https://github.com/colin861209/DHEMS/commit/8c84b5726aa7aab361bf1744c760aa54ba3717a0)

* Content:
  * SQLFunction:
    1. ADD function `demand response info`
  * GHEMS:
    1. DR info
    2. constraint
    3. Pgrid_max_array
  * LHEMS:
    1. DR info
    2. household's weighting

+ Commit link [38e7cc9](https://github.com/colin861209/DHEMS/commit/38e7cc98b8071f0a6d90658cb234b24ffc31861e)

* Content:
  * SQLFunction:
	1. `-404` no found data
	2. `-999` data is NULL, not number etc...

---
## 2021/04/15

+ Commit link [57ebaf5](https://github.com/colin861209/DHEMS/commit/57ebaf539e656267a0b4b14565142209170e54d4)

* Content:
  * SQLFunction:
    1. ADD function `flag_receive` & `value_receive`
  * GHEMS:
    1. Import function `flag_receive` & `value_receive`
    2. Some work determine by flag
  * LHEMS:
    1. Import function `flag_receive` & `value_receive`
    2. Some work determine by flag

---
## 2021/05/13

+ Commit link [7cbd8f1](https://github.com/colin861209/DHEMS/commit/7cbd8f1a155745b9f882687581a71219b8c2f863)
+ Commit link [a09559e](https://github.com/colin861209/DHEMS/commit/a09559eef95d9d282c79429b102453cef4ac622b)

* Content:
  * scheduling_parameter:
    1. Function `display_coefAndBnds_rowNum`
    2. Display `coef_row_num` and `bnd_row_num`
    3. Note: `coef_row_num range` substractd from `bnd_row_num range` should equal to `1` 
  * GHEMS:
    1. Remove GLPK coefficient replace `(time_block-sample_time) * {number}` with `coef_row_num`
    2. Remove GLPK set_row_bnds replace `(time_block-sample_time) * {number} + 1` with `bnd_row_num`
    3. Add for loop execute number after for loop `ex: coef_row_num += 96`

---
## 2021/05/31

+ Commit link [b521f06](https://github.com/colin861209/DHEMS/commit/b521f06e8e32a59f50202050b66bbd372768a575)
+ Commit link [4932911](https://github.com/colin861209/DHEMS/commit/49329115b37a529a1cb51d6cb6d538b30c7d6a29)

* Content: 50 household 
	* 10 code parallel process run 5 time in one time step
	* Table `distributed_group` record each group flag individual and update to table `BaseParameter` if condition is established
