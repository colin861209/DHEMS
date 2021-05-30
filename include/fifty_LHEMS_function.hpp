#ifndef fifty_LHEMS_function_H
#define fifty_LHEMS_function_H

#include <vector>
#include <string>
using namespace std;

extern int h, i, j, k, m, n;
extern int time_block, sample_time, divide, interrupt_num, uninterrupt_num, varying_num, variable, app_count;
extern float Cbat, Vsys, SOC_ini, SOC_min, SOC_max, SOC_thres, Pbat_min, Pbat_max, Pgrid_max, Psell_max, delta_T;
extern int distributed_household_id, household_id, distributed_householdTotal, householdTotal;

int connect_mysql(string DB_name);
int determine_realTimeOrOneDayMode_andGetSOC(int real_time, vector<string> variable_name, int distributed_group_num);
void countUninterruptAndVaryingLoads_Flag(int *uninterrupt_flag, int *varying_flag, int household_id);
void countLoads_AlreadyOpenedTimes(int *buff, int household_id);
void count_interruptLoads_RemainOperateTime(int interrupt_num, int *interrupt_ot, int *interrupt_reot, int *buff);
void count_uninterruptAndVaryingLoads_RemainOperateTime(int group_id, int loads_total, int *total_operateTime, int *remain_operateTime, int *end_time, int *flag, int *buff);
void init_VaryingLoads_OperateTimeAndPower(int **varying_t_d, float **varying_p_d, int *varying_ot);
void putValues_VaryingLoads_OperateTimeAndPower(int **varying_t_d, float **varying_p_d, int **varying_t_pow, float **varying_p_pow, int *varying_start, int *varying_end, float *varying_p_max);
void optimization(vector<string> variable_name, int household_id, int *interrupt_start, int *interrupt_end, int *interrupt_ot, int *interrupt_reot, float *interrupt_p, int *uninterrupt_start, int *uninterrupt_end, int *uninterrupt_ot, int *uninterrupt_reot, float *uninterrupt_p, int *uninterrupt_flag, int *varying_start, int *varying_end, int *varying_ot, int *varying_reot, int *varying_flag, int **varying_t_pow, float **varying_p_pow, int app_count, float *price, float *uncontrollable_load);
void update_loadModel(float *interrupt_p, float *uninterrupt_p, int household_id, int distributed_group_num);
float *rand_operationTime(int distributed_group_num);
float *household_weighting();
int truncate_table_flag();
int get_distributed_group(string target, string condition_col = "", int condition_num = -1);
void update_distributed_group(string target, int target_value, string condition_col, int condition_num);
void init_totalLoad_flag_and_table(int distributed_group_num);

#endif
