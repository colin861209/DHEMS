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
      1. ADD function 'demand response info'
	* GHEMS:
      1. DR info
      2. constraint
      3. Pgrid_max_array
	* LHEMS:
	   1. DR info
	   2. household's weighting

---
