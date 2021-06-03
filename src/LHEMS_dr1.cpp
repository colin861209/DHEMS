#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> //162 165 76~86 579~581�Ȯɵ��ѱ�
#include <glpk.h> //435~438�h�gsame_day
#include <math.h>
#include <mysql.h>
#include <iostream> //504~513 sample_time ��b�iGLPK�e�A�o�˹L�@�Ѥ~���|�n���s��
#include "SQLFunction.hpp"
// use function 'find_variableName_position' needs
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
using namespace std;
int h, i, j, k, m, n = 0;
double z = 0;
vector<string> variable_name;
// base parameter
int time_block = 0, variable = 0, divide = 0, sample_time = 0, householdTotal = 0, interrupt_num, uninterrupt_num, varying_num, app_count, household_id;
int Pgrid_flag, Pess_flag, Pfc_flag, interruptLoad_flag, uninterruptLoad_flag, varyingLoad_flag;
int dr_mode, dr_startTime, dr_endTime, dr_minDecrease_power, dr_feedback_price, dr_customer_baseLine;
float delta_T = 0.0;
float Cbat = 0.0, Vsys = 0.0, SOC_ini = 0.0, SOC_min = 0.0, SOC_max = 0.0, SOC_thres = 0.0, Pbat_min = 0.0, Pbat_max = 0.0, Pgrid_max = 0.0, Psell_max;

float step1_bill = 0.0, step1_sell = 0.0, step1_PESS = 0.0; //?�[?B?J?@?p??q?O

char column[400] = "A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,A15,A16,A17,A18,A19,A20,A21,A22,A23,A24,A25,A26,A27,A28,A29,A30,A31,A32,A33,A34,A35,A36,A37,A38,A39,A40,A41,A42,A43,A44,A45,A46,A47,A48,A49,A50,A51,A52,A53,A54,A55,A56,A57,A58,A59,A60,A61,A62,A63,A64,A65,A66,A67,A68,A69,A70,A71,A72,A73,A74,A75,A76,A77,A78,A79,A80,A81,A82,A83,A84,A85,A86,A87,A88,A89,A90,A91,A92,A93,A94,A95";
int determine_realTimeOrOneDayMode_andGetSOC(int real_time, vector<string> variable_name);
void getOrUpdate_SolarInfo_ThroughSampleTime(const char *weather, float *solar2);
void countUninterruptAndVaryingLoads_Flag(int *, int *, int);
void countLoads_AlreadyOpenedTimes(int *, int);
void count_interruptLoads_RemainOperateTime(int, int *, int *, int *);
void count_uninterruptAndVaryingLoads_RemainOperateTime(int, int, int *, int *, int *, int *, int *);
void init_VaryingLoads_OperateTimeAndPower(int **, float **, int *);
void putValues_VaryingLoads_OperateTimeAndPower(int **varying_t_d, float **varying_p_d, int **varying_t_pow, float **varying_p_pow, int *varying_start, int *varying_end, float *varying_p_max);
void updateTableCost(float *now_grid, float *varying_grid, float *cost, float *FC_cost, float *Hydrogen_com, float *FC_every_cost, float now_power_result, float var_grid_result, float opt_cost_result, float opt_FC_cost_result, float opt_Hydrogen_result, float price_sum_now_power);
void optimization(vector<string> variable_name, int, int *, int *, int *, int *, float *, int *, int *, int *, int *, float *, int *, int *, int *, int *, int *, int *, int **, float **, int, float *, float *);
void update_loadModel(float *, float *, int);
float *rand_operationTime();
float *household_weighting();

int main(void)
{
	time_t t = time(NULL);
	struct tm now_time = *localtime(&t);
	int real_time = 0;

	if ((mysql_real_connect(mysql_con, "140.124.42.65", "root", "fuzzy314", "DHEMS_dr1", 3306, NULL, 0)) == NULL)
	{
		printf("Failed to connect to Mysql!\n");
		system("pause");
		return 0;
	}
	printf("Connect to Mysql sucess!!\n");
	mysql_set_character_set(mysql_con, "utf8");

	// =-=-=-=-=-=-=- get BaseParameter values -=-=-=-=-=-=-= //
	float *base_par = new float[3 + 7];
	char *s_time = new char[3];
	for (i = 1; i <= 3; i++)
		base_par[i - 1] = value_receive("BaseParameter", "parameter_id", i);

	for (i = 8; i <= 14; i++)
		base_par[i - 5] = value_receive("BaseParameter", "parameter_id", i, 'F');

	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value from BaseParameter where parameter_name = 'lastTime_execute' ");
	fetch_row_value();
	s_time = mysql_row[0];

	char *token = strtok(s_time, "-");
	vector<int> time_tmp;
	while (token != NULL)
	{
		time_tmp.push_back(atoi(token));
		token = strtok(NULL, "-");
	}

	// not the same day real_time = 0
	if ((time_tmp[0] != (now_time.tm_year + 1900)) || (time_tmp[1] != (now_time.tm_mon + 1)) || (time_tmp[2] != (now_time.tm_mday)))
		real_time = 0;
	time_tmp.clear();

	time_block = base_par[0];
	householdTotal = base_par[1];
	variable = base_par[2];
	Vsys = base_par[3];
	Cbat = base_par[4];
	SOC_min = base_par[5];
	SOC_max = base_par[6];
	SOC_thres = base_par[7];
	Pbat_min = base_par[8];
	Pbat_max = base_par[9];
	divide = (time_block / 24);
	delta_T = 1.0 / (float)divide;

	Pgrid_max = value_receive("BaseParameter", "parameter_name", "Pgridmax", 'F');
	real_time = value_receive("BaseParameter", "parameter_name", "real_time");
	household_id = value_receive("BaseParameter", "parameter_name", "household_id");

	// =-=-=-=-=-=-=- get load_list loads category's amount -=-=-=-=-=-=-= //
	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT COUNT(*) FROM load_list WHERE group_id = 1");
	interrupt_num = turn_value_to_int(0);
	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT COUNT(*) FROM load_list WHERE group_id = 2");
	uninterrupt_num = turn_value_to_int(0);
	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT COUNT(*) FROM load_list WHERE group_id = 3");
	varying_num = turn_value_to_int(0);
	app_count = interrupt_num + uninterrupt_num + varying_num;

	// =-=-=-=-=-=-=- get demand response -=-=-=-=-=-=-= //
	dr_mode = value_receive("BaseParameter", "parameter_name", "dr_mode");
	messagePrint(__LINE__, "dr mode: ", 'I', dr_mode);
	if (dr_mode != 0)
	{
		int *dr_info = demand_response_info(dr_mode);
		dr_startTime = dr_info[0];
		dr_endTime = dr_info[1];
		dr_minDecrease_power = dr_info[2];
		dr_feedback_price = dr_info[3];
		dr_customer_baseLine = dr_info[4];
	}
	interruptLoad_flag = flag_receive("LHEMS_flag", "interrupt");
	uninterruptLoad_flag = flag_receive("LHEMS_flag", "uninterrupt");
	varyingLoad_flag = flag_receive("LHEMS_flag", "varying");
	Pgrid_flag = flag_receive("LHEMS_flag", "Pgrid");
	Pess_flag = flag_receive("LHEMS_flag", "Pess");
	// =-=-=-=-=-=-=- Define variable name and use in GLPK -=-=-=-=-=-=-= //
	// Most important thing, helping in GLPK big matrix setting
	if (interruptLoad_flag == 1)
	{
		for (int i = 0; i < interrupt_num; i++)
			variable_name.push_back("interrupt" + to_string(i + 1));
	}
	if (uninterruptLoad_flag == 1)
	{
		for (int i = 0; i < uninterrupt_num; i++)
			variable_name.push_back("uninterrupt" + to_string(i + 1));
	}
	if (varyingLoad_flag == 1)
	{
		for (int i = 0; i < varying_num; i++)
			variable_name.push_back("varying" + to_string(i + 1));
	}
	if (Pgrid_flag == 1)
		variable_name.push_back("Pgrid");
	if (Pess_flag == 1)
	{
		variable_name.push_back("Pess");
		variable_name.push_back("Pcharge");
		variable_name.push_back("Pdischarge");
		variable_name.push_back("SOC");
		variable_name.push_back("Z");
	}
	if (dr_mode != 0)
		variable_name.push_back("dr_alpha");
	if (uninterruptLoad_flag == 1)
	{
		for (int i = 0; i < uninterrupt_num; i++)
			variable_name.push_back("uninterDelta" + to_string(i + 1));
	}
	if (varyingLoad_flag == 1)
	{
		for (int i = 0; i < varying_num; i++)
			variable_name.push_back("varyingDelta" + to_string(i + 1));
		for (int i = 0; i < varying_num; i++)
			variable_name.push_back("varyingPsi" + to_string(i + 1));
	}
	variable = variable_name.size();

	snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `BaseParameter` SET value = %d WHERE `BaseParameter`.`parameter_name` = 'local_variable_num' ", variable);
	sent_query();

	// =-=-=-=-=-=-=- Get simulation time slot -=-=-=-=-=-=-= //
	sample_time = value_receive("BaseParameter", "parameter_name", "next_simulate_timeblock");

	// =-=-=-=-=-=-=- get electric price data -=-=-=-=-=-=-= //
	string simulate_price;
	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value FROM BaseParameter WHERE parameter_name = 'simulate_price' ");
	if (fetch_row_value() != -1)
		simulate_price = mysql_row[0];
	float *price = new float[time_block];
	for (i = 0; i < time_block; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM price WHERE price_period = %d", simulate_price.c_str(), i);
		price[i] = turn_value_to_float(0);
	}

	// =-=-=-=-=-=-=- determine which mode and get SOC if in need -=-=-=-=-=-=-= //
	real_time = determine_realTimeOrOneDayMode_andGetSOC(real_time, variable_name);
	if ((sample_time + 1) == 97)
	{
		messagePrint(__LINE__, "Time block to the end !!");
		exit(0);
	}

	// =-=-=-=-=-=-=- Update date is necesssary in real experience -=-=-=-=-=-=-= //
	snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = '%d-%02d-%02d' WHERE parameter_name = 'lastTime_execute' ", now_time.tm_year + 1900, now_time.tm_mon + 1, now_time.tm_mday);
	sent_query();

	// =-=-=-=-=-=-=- the time slot which send to GLPK -=-=-=-=-=-=-= //
	sample_time = value_receive("BaseParameter", "parameter_name", "next_simulate_timeblock");
	messagePrint(__LINE__, "sample time from database = ", 'I', sample_time);

	float *uncontrollable_load = rand_operationTime();
	// =-=-=-=-=-=-=- initial total load table -=-=-=-=-=-=-= //
	if (sample_time == 0 && household_id == 1)
	{
		for (int i = 1; i <= householdTotal; i++)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `totalLoad_model` SET `household%d` = '0' ", i);
			sent_query();
		}
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `totalLoad_model` SET `totalLoad` = '0' ");
		sent_query();
	}

	// =-=-=-=-=-=-=- becuase have five household, so will run GLPK five times -=-=-=-=-=-=-= //

	// =-=-=-=-=-=-=- get each hosueholds' loads info -=-=-=-=-=-=-= //
	int *interrupt_start = new int[interrupt_num];
	int *interrupt_end = new int[interrupt_num];
	int *interrupt_ot = new int[interrupt_num];
	int *interrupt_reot = new int[interrupt_num];
	float *interrupt_p = new float[interrupt_num];

	int *uninterrupt_start = new int[uninterrupt_num];
	int *uninterrupt_end = new int[uninterrupt_num];
	int *uninterrupt_ot = new int[uninterrupt_num];
	int *uninterrupt_reot = new int[uninterrupt_num];
	float *uninterrupt_p = new float[uninterrupt_num];
	int *uninterrupt_flag = new int[uninterrupt_num];

	int *varying_start = new int[varying_num];
	int *varying_end = new int[varying_num];
	int *varying_ot = new int[varying_num];
	int *varying_reot = new int[varying_num];
	int **varying_t_pow = NEW2D(varying_num, 3, int);
	float **varying_p_pow = NEW2D(varying_num, 3, float);
	int *varying_flag = new int[varying_num];
	for (int i = 0; i < interrupt_num; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT household%d_startEndOperationTime FROM load_list WHERE group_id = 1 and number = %d", household_id, i + 1);
		fetch_row_value();
		char *seo_time = mysql_row[0];
		token = strtok(seo_time, "~");
		while (token != NULL)
		{
			time_tmp.push_back(atoi(token));
			token = strtok(NULL, "~");
		}
		interrupt_start[i] = (int)time_tmp[0];
		interrupt_end[i] = (int)time_tmp[1] - 1;
		interrupt_ot[i] = (int)time_tmp[2];
		interrupt_reot[i] = 0;
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT power1 FROM load_list WHERE group_id = 1 and number = %d", i + 1);
		interrupt_p[i] = turn_value_to_float(0);
		time_tmp.clear();
	}
	for (int i = 0; i < uninterrupt_num; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT household%d_startEndOperationTime FROM load_list WHERE group_id = 2 and number = %d", household_id, i + 1 + interrupt_num);
		fetch_row_value();
		char *seo_time = mysql_row[0];
		token = strtok(seo_time, "~");
		while (token != NULL)
		{
			time_tmp.push_back(atoi(token));
			token = strtok(NULL, "~");
		}
		uninterrupt_start[i] = (int)time_tmp[0];
		uninterrupt_end[i] = (int)time_tmp[1] - 1;
		uninterrupt_ot[i] = (int)time_tmp[2];
		uninterrupt_reot[i] = 0;
		uninterrupt_flag[i] = 0;
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT power1 FROM load_list WHERE group_id = 2 and number = %d", i + 1 + interrupt_num);
		uninterrupt_p[i] = turn_value_to_float(0);
		time_tmp.clear();
	}
	for (int i = 0; i < varying_num; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT household%d_startEndOperationTime FROM load_list WHERE group_id = 3 and number = %d", household_id, i + 1 + interrupt_num + uninterrupt_num);
		fetch_row_value();
		char *seo_time = mysql_row[0];
		token = strtok(seo_time, "~");
		while (token != NULL)
		{
			time_tmp.push_back(atoi(token));
			token = strtok(NULL, "~");
		}
		varying_start[i] = (int)time_tmp[0];
		varying_end[i] = (int)time_tmp[1] - 1;
		varying_ot[i] = (int)time_tmp[2];
		varying_reot[i] = 0;
		varying_flag[i] = 0;
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT power1, power2, power3, block1, block2, block3 FROM load_list WHERE group_id = 3 and number = %d", i + 1 + interrupt_num + uninterrupt_num);
		fetch_row_value();
		for (int z = 0; z < 3; z++)
			varying_p_pow[i][z] = turn_float(z);
		for (int z = 0; z < 3; z++)
			varying_t_pow[i][z] = int(turn_float(z + 3) * divide);
		time_tmp.clear();

		optimization(variable_name, household_id, interrupt_start, interrupt_end, interrupt_ot, interrupt_reot, interrupt_p, uninterrupt_start, uninterrupt_end, uninterrupt_ot, uninterrupt_reot, uninterrupt_p, uninterrupt_flag, varying_start, varying_end, varying_ot, varying_reot, varying_flag, varying_t_pow, varying_p_pow, app_count, price, uncontrollable_load);

		update_loadModel(interrupt_p, uninterrupt_p, household_id);
	}

	printf("LINE %d: sample_time = %d\n", __LINE__, sample_time);
	if (household_id == householdTotal)
	{
		printf("LINE %d: next sample_time = %d\n\n", __LINE__, sample_time + 1);
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = '%d' WHERE  parameter_name = 'next_simulate_timeblock'", sample_time + 1);
		sent_query();
	}

	if (household_id < householdTotal)
		household_id++;
	else
		household_id = 1;

	printf("LINE %d: next household_id = %d\n", __LINE__, household_id);
	snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = '%d' WHERE  parameter_name = 'household_id'", household_id);
	sent_query();

	mysql_close(mysql_con);
	return 0;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------- GLPK ---------------------------------------------------------------------------------------------------------------------------------------------------- //

void optimization(vector<string> variable_name, int household_id, int *interrupt_start, int *interrupt_end, int *interrupt_ot, int *interrupt_reot, float *interrupt_p, int *uninterrupt_start, int *uninterrupt_end, int *uninterrupt_ot, int *uninterrupt_reot, float *uninterrupt_p, int *uninterrupt_flag, int *varying_start, int *varying_end, int *varying_ot, int *varying_reot, int *varying_flag, int **varying_t_pow, float **varying_p_pow, int app_count, float *price, float *uncontrollable_load)
{
	functionPrint(__func__);
	time_t t = time(NULL);
	struct tm now_time = *localtime(&t);

	// 'noo' is timeblock which determine to control each loads opened or closed.
	//get now time that can used in the real experiment
	int noo;
	if (((now_time.tm_min) % (60 / divide)) != 0)
		noo = (now_time.tm_hour) * divide + (int)((now_time.tm_min) / (60 / divide));
	else
		noo = (now_time.tm_hour) * divide + (int)((now_time.tm_min) / (60 / divide));

	messagePrint(__LINE__, "Now time block : ", 'I', noo, 'Y');

	// =-=-=-=-=-=-=- choose column 'big_sunny' 'sunny' 'cloudy' in table solar_data -=-=-=-=-=-=-= //
	float *solar2 = new float[time_block];
	getOrUpdate_SolarInfo_ThroughSampleTime("cloudy", solar2);

	countUninterruptAndVaryingLoads_Flag(uninterrupt_flag, varying_flag, household_id);

	/*====================(Get the number of times the device was executed before sample_time)=======================*/
	int *buff = new int[app_count]; //(The number of remaining executions)
	for (i = 0; i < app_count; i++)
		buff[i] = 0;
	countLoads_AlreadyOpenedTimes(buff, household_id);

	/*============================(Calculate the remaining execution time of appliances)==============================*/
	count_interruptLoads_RemainOperateTime(interrupt_num, interrupt_ot, interrupt_reot, buff);
	count_uninterruptAndVaryingLoads_RemainOperateTime(2, uninterrupt_num, uninterrupt_ot, uninterrupt_reot, uninterrupt_end, uninterrupt_flag, buff);
	for (int i = 0; i < uninterrupt_num; i++)
		printf("LINE %d: uninterrupt load %d : reot = %d\n\t", __LINE__, i, uninterrupt_reot[i]);
	count_uninterruptAndVaryingLoads_RemainOperateTime(3, varying_num, varying_ot, varying_reot, varying_end, varying_flag, buff);
	for (int i = 0; i < varying_num; i++)
		printf("LINE %d: varying load %d : reot = %d\n", __LINE__, i, varying_reot[i]);

	/*=========================(Structure initialization of varying load)=============================*/
	// int **varying_t_d;
	// float **varying_p_d;
	int **varying_t_d = NEW2D(varying_num, (time_block - sample_time), int);
	float **varying_p_d = NEW2D(varying_num, varying_ot[i], float);
	float *varying_p_max = new float[varying_num];
	init_VaryingLoads_OperateTimeAndPower(varying_t_d, varying_p_d, varying_ot);
	putValues_VaryingLoads_OperateTimeAndPower(varying_t_d, varying_p_d, varying_t_pow, varying_p_pow, varying_start, varying_end, varying_p_max);

	float *weighting_array;
	if (dr_mode != 0)
		weighting_array = household_weighting();

	printf("\n------ Starting GLPK Part ------\n");

	/*============================(GLPK matrix row & col definition)==================================*/
	int rowTotal = (time_block - sample_time) * 200 + 1;
	int colTotal = variable * (time_block - sample_time);

	/*=============================(GLPK variable definition)=====================================*/
	glp_prob *mip;
	mip = glp_create_prob();
	glp_set_prob_name(mip, "LHEMS_dr1");
	glp_set_obj_dir(mip, GLP_MIN); //�̤p�ƥιq��O
	glp_add_rows(mip, rowTotal);
	glp_add_cols(mip, colTotal);

	// column setting
	for (i = 0; i < (time_block - sample_time); i++)
	{
		if (interruptLoad_flag == 1)
		{
			for (int j = 1; j <= interrupt_num; j++)
			{
				glp_set_col_bnds(mip, (find_variableName_position(variable_name, "interrupt" + to_string(j)) + 1 + i * variable), GLP_DB, 0.0, 1.0);
				glp_set_col_kind(mip, (find_variableName_position(variable_name, "interrupt" + to_string(j)) + 1 + i * variable), GLP_BV);
			}
		}
		if (uninterruptLoad_flag == 1)
		{
			for (int j = 1; j <= uninterrupt_num; j++)
			{
				glp_set_col_bnds(mip, (find_variableName_position(variable_name, "uninterrupt" + to_string(j)) + 1 + i * variable), GLP_DB, 0.0, 1.0);
				glp_set_col_kind(mip, (find_variableName_position(variable_name, "uninterrupt" + to_string(j)) + 1 + i * variable), GLP_BV);
			}
		}
		if (varyingLoad_flag == 1)
		{
			for (int j = 1; j <= varying_num; j++)
			{
				glp_set_col_bnds(mip, (find_variableName_position(variable_name, "varying" + to_string(j)) + 1 + i * variable), GLP_DB, 0.0, 1.0);
				glp_set_col_kind(mip, (find_variableName_position(variable_name, "varying" + to_string(j)) + 1 + i * variable), GLP_BV);
			}
		}
		if (Pgrid_flag == 1)
		{
			glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pgrid") + 1 + i * variable), GLP_DB, 0.0, Pgrid_max);
			glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pgrid") + 1 + i * variable), GLP_CV);
		}
		if (Pess_flag == 1)
		{
			glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pess") + 1 + i * variable), GLP_DB, -Pbat_min, Pbat_max);
			glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pess") + 1 + i * variable), GLP_CV);
			glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pcharge") + 1 + i * variable), GLP_FR, 0.0, Pbat_max);
			glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pcharge") + 1 + i * variable), GLP_CV);
			glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pdischarge") + 1 + i * variable), GLP_FR, 0.0, Pbat_min);
			glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pdischarge") + 1 + i * variable), GLP_CV);
			glp_set_col_bnds(mip, (find_variableName_position(variable_name, "SOC") + 1 + i * variable), GLP_DB, SOC_min, SOC_max);
			glp_set_col_kind(mip, (find_variableName_position(variable_name, "SOC") + 1 + i * variable), GLP_CV);
			glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Z") + 1 + i * variable), GLP_DB, 0.0, 1.0);
			glp_set_col_kind(mip, (find_variableName_position(variable_name, "Z") + 1 + i * variable), GLP_BV);
		}
		if (dr_mode != 0)
		{
			glp_set_col_bnds(mip, (find_variableName_position(variable_name, "dr_alpha") + 1 + i * variable), GLP_DB, 0.0, 1.0);
			glp_set_col_kind(mip, (find_variableName_position(variable_name, "dr_alpha") + 1 + i * variable), GLP_CV);
		}
		if (uninterruptLoad_flag == 1)
		{
			for (int j = 1; j <= uninterrupt_num; j++)
			{
				glp_set_col_bnds(mip, (find_variableName_position(variable_name, "uninterDelta" + to_string(j)) + 1 + i * variable), GLP_DB, 0.0, 1.0);
				glp_set_col_kind(mip, (find_variableName_position(variable_name, "uninterDelta" + to_string(j)) + 1 + i * variable), GLP_BV);
			}
		}
		if (varyingLoad_flag == 1)
		{
			for (int j = 1; j <= varying_num; j++)
			{
				glp_set_col_bnds(mip, (find_variableName_position(variable_name, "varyingDelta" + to_string(j)) + 1 + i * variable), GLP_DB, 0.0, 1.0);
				glp_set_col_kind(mip, (find_variableName_position(variable_name, "varyingDelta" + to_string(j)) + 1 + i * variable), GLP_BV);
			}
			for (int j = 1; j <= varying_num; j++)
			{
				glp_set_col_bnds(mip, (find_variableName_position(variable_name, "varyingPsi" + to_string(j)) + 1 + i * variable), GLP_DB, 0.0, varying_p_max[j - 1]);
				glp_set_col_kind(mip, (find_variableName_position(variable_name, "varyingPsi" + to_string(j)) + 1 + i * variable), GLP_CV);
			}
		}
	}

	/*=============================(initial the matrix)======================================*/
	float **coefficient = NEW2D(rowTotal, colTotal, float);
	for (m = 0; m < rowTotal; m++)
	{
		for (n = 0; n < colTotal; n++)
			coefficient[m][n] = 0.0;
	}
	// int column_tmp = 0;
	// int row_tmp = 0;
	for (h = 0; h < interrupt_num; h++)
	{
		if ((interrupt_end[h] - sample_time) >= 0)
		{
			if ((interrupt_start[h] - sample_time) >= 0)
			{
				for (i = (interrupt_start[h] - sample_time); i <= (interrupt_end[h] - sample_time); i++)
				{
					coefficient[h][i * variable + find_variableName_position(variable_name, "interrupt" + to_string(h + 1))] = 1.0;
				}
			}
			else if ((interrupt_start[h] - sample_time) < 0)
			{
				for (i = 0; i <= (interrupt_end[h] - sample_time); i++)
				{
					coefficient[h][i * variable + find_variableName_position(variable_name, "interrupt" + to_string(h + 1))] = 1.0;
				}
			}
		}
	}
	for (i = 1; i <= interrupt_num; i++)
	{
		glp_set_row_name(mip, i, "");
		glp_set_row_bnds(mip, i, GLP_LO, ((float)interrupt_reot[i - 1]), 0.0);
	}
	// for (h = 0; h < uninterrupt_num; h++)
	// {
	// 	if (uninterrupt_flag[h] == 0)
	// 	{
	// 		if (((uninterrupt_end[h] - sample_time) >= 0) && (uninterrupt_reot[h] > 0))
	// 		{
	// 			if ((uninterrupt_start[h] - sample_time) >= 0)
	// 			{
	// 				for (i = (uninterrupt_start[h] - sample_time); i <= (uninterrupt_end[h] - sample_time); i++)
	// 				{
	// 					coefficient[h + interrupt_num][i * variable + find_variableName_position(variable_name, "uninterrupt" + to_string(h + 1))] = 1.0;
	// 				}
	// 			}
	// 			else if ((uninterrupt_start[h] - sample_time) < 0)
	// 			{
	// 				for (i = 0; i <= (uninterrupt_end[h] - sample_time); i++)
	// 				{
	// 					coefficient[h + interrupt_num][i * variable + find_variableName_position(variable_name, "uninterrupt" + to_string(h + 1))] = 1.0;
	// 				}
	// 			}
	// 		}
	// 	}
	// }
	// for (i = 1; i <= uninterrupt_num; i++)
	// {
	// 	glp_set_row_name(mip, interrupt_num + i, "");
	// 	glp_set_row_bnds(mip, interrupt_num + i, GLP_LO, ((float)uninterrupt_reot[i - 1]), 0.0);
	// }
	// for (h = 0; h < varying_num; h++)
	// {
	// 	if (varying_flag[h] == 0)
	// 	{
	// 		if (((varying_end[h] - sample_time) >= 0) && (varying_reot[h] > 0))
	// 		{
	// 			if ((varying_start[h] - sample_time) >= 0)
	// 			{
	// 				for (i = (varying_start[h] - sample_time); i <= (varying_end[h] - sample_time); i++)
	// 				{
	// 					coefficient[h + interrupt_num + uninterrupt_num][i * variable + find_variableName_position(variable_name, "varying" + to_string(h + 1))] = 1.0;
	// 				}
	// 			}
	// 			else if ((varying_start[h] - sample_time) < 0)
	// 			{
	// 				for (i = 0; i <= (varying_end[h] - sample_time); i++)
	// 				{
	// 					coefficient[h + interrupt_num + uninterrupt_num][i * variable + find_variableName_position(variable_name, "varying" + to_string(h + 1))] = 1.0;
	// 				}
	// 			}
	// 		}
	// 	}
	// }
	// for (i = 1; i <= varying_num; i++)
	// {
	// 	glp_set_row_name(mip, interrupt_num + uninterrupt_num + i, "");
	// 	glp_set_row_bnds(mip, interrupt_num + uninterrupt_num + i, GLP_LO, ((float)varying_reot[i - 1]), 0.0);
	// }
	// 0 < Pgrid j < Pgrid max
	if (dr_mode != 0)
	{
		for (i = 0; i < (time_block - sample_time); i++)
		{
			coefficient[app_count + i][i * variable + find_variableName_position(variable_name, "Pgrid")] = 1.0;
			coefficient[app_count + i][i * variable + find_variableName_position(variable_name, "dr_alpha")] = -Pgrid_max;
		}
		for (i = 1; i <= (time_block - sample_time); i++)
		{
			glp_set_row_name(mip, app_count + i, "");
			glp_set_row_bnds(mip, app_count + i, GLP_UP, 0.0, 0.0);
		}
	}
	//(Balanced function) Pgrid j + Pess j = sum(Pa j)
	for (h = 0; h < interrupt_num; h++)
	{
		if ((interrupt_end[h] - sample_time) >= 0)
		{
			if ((interrupt_start[h] - sample_time) >= 0)
			{
				for (i = (interrupt_start[h] - sample_time); i <= (interrupt_end[h] - sample_time); i++)
				{
					coefficient[(time_block - sample_time) + app_count + i][i * variable + find_variableName_position(variable_name, "interrupt" + to_string(h + 1))] = interrupt_p[h];
				}
			}
			else if ((interrupt_start[h] - sample_time) < 0)
			{
				for (i = 0; i <= (interrupt_end[h] - sample_time); i++)
				{
					coefficient[(time_block - sample_time) + app_count + i][i * variable + find_variableName_position(variable_name, "interrupt" + to_string(h + 1))] = interrupt_p[h];
				}
			}
		}
	}
	for (h = 0; h < uninterrupt_num; h++) //(uninterrupt load)
	{
		if ((uninterrupt_end[h] - sample_time) >= 0)
		{
			if ((uninterrupt_start[h] - sample_time) >= 0)
			{
				for (i = (uninterrupt_start[h] - sample_time); i <= (uninterrupt_end[h] - sample_time); i++)
				{
					coefficient[(time_block - sample_time) + app_count + i][i * variable + find_variableName_position(variable_name, "uninterrupt" + to_string(h + 1))] = uninterrupt_p[h];
				}
			}
			else if ((uninterrupt_start[h] - sample_time) < 0)
			{
				for (i = 0; i <= (uninterrupt_end[h] - sample_time); i++)
				{
					coefficient[(time_block - sample_time) + app_count + i][i * variable + find_variableName_position(variable_name, "uninterrupt" + to_string(h + 1))] = uninterrupt_p[h];
				}
			}
		}
	}
	for (h = 0; h < varying_num; h++) //(varying load)
	{
		if ((varying_end[h] - sample_time) >= 0)
		{
			if ((varying_start[h] - sample_time) >= 0)
			{
				for (i = (varying_start[h] - sample_time); i <= (varying_end[h] - sample_time); i++)
				{
					coefficient[(time_block - sample_time) + app_count + i][i * variable + find_variableName_position(variable_name, "varyingPsi" + to_string(h + 1))] = 1.0;
				}
			}
			else if ((varying_start[h] - sample_time) < 0)
			{
				for (i = 0; i <= (varying_end[h] - sample_time); i++)
				{
					coefficient[(time_block - sample_time) + app_count + i][i * variable + find_variableName_position(variable_name, "varyingPsi" + to_string(h + 1))] = 1.0;
				}
			}
		}
	}
	// Pgrid j
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) + app_count + i][i * variable + find_variableName_position(variable_name, "Pgrid")] = -1.0;
		if (Pess_flag == 1)
			coefficient[(time_block - sample_time) + app_count + i][i * variable + find_variableName_position(variable_name, "Pess")] = 1.0;
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) + app_count + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) + app_count + i), GLP_FX, -uncontrollable_load[i - 1 + sample_time], -uncontrollable_load[i - 1 + sample_time]);
	}

	if (Pess_flag == 1)
	{ // SOC j - 1 + sum((Pess * Ts) / (Cess * Vess)) >= SOC threshold, only one constranit formula
		for (i = 0; i < (time_block - sample_time); i++)
		{
			coefficient[(time_block - sample_time) * 2 + app_count][i * variable + find_variableName_position(variable_name, "Pess")] = 1.0;
		}
		glp_set_row_name(mip, ((time_block - sample_time) * 2 + app_count + 1), "");
		if (sample_time == 0)
		{
			glp_set_row_bnds(mip, ((time_block - sample_time) * 2 + app_count + 1), GLP_LO, ((SOC_thres - SOC_ini) * Cbat * Vsys) / delta_T, 0.0);
		}
		else
		{
			// avoid the row max is bigger than SOC max
			glp_set_row_bnds(mip, ((time_block - sample_time) * 2 + app_count + 1), GLP_DB, ((SOC_thres - SOC_ini) * Cbat * Vsys) / delta_T, ((0.89 - SOC_ini) * Cbat * Vsys) / delta_T);
		}

		// next SOC
		// SOC j = SOC j - 1 + (Pess j * Ts) / (Cess * Vess)
		for (i = 0; i < (time_block - sample_time); i++)
		{
			for (j = 0; j <= i; j++)
			{
				coefficient[(time_block - sample_time) * 2 + app_count + 1 + i][j * variable + find_variableName_position(variable_name, "Pess")] = -1.0; // Pess
			}
			coefficient[(time_block - sample_time) * 2 + app_count + 1 + i][i * variable + find_variableName_position(variable_name, "SOC")] = Cbat * Vsys / delta_T; //SOC
		}
		for (i = 1; i <= (time_block - sample_time); i++)
		{
			glp_set_row_name(mip, ((time_block - sample_time) * 2 + app_count + 1 + i), "");
			glp_set_row_bnds(mip, ((time_block - sample_time) * 2 + app_count + 1 + i), GLP_FX, (SOC_ini * Cbat * Vsys / delta_T), (SOC_ini * Cbat * Vsys / delta_T));
		}

		//(Charge limit) Pess + <= z * Pcharge max
		for (i = 0; i < (time_block - sample_time); i++)
		{
			coefficient[(time_block - sample_time) * 3 + app_count + 1 + i][i * variable + find_variableName_position(variable_name, "Pcharge")] = 1.0; //Pess +
			coefficient[(time_block - sample_time) * 3 + app_count + 1 + i][i * variable + find_variableName_position(variable_name, "Z")] = -Pbat_max; //Z
		}
		for (i = 1; i <= (time_block - sample_time); i++)
		{
			glp_set_row_name(mip, ((time_block - sample_time) * 3 + app_count + 1 + i), "");
			glp_set_row_bnds(mip, ((time_block - sample_time) * 3 + app_count + 1 + i), GLP_UP, 0.0, 0.0);
		}

		// (Discharge limit) Pess - <= (1 - z) * (-Pdischarge max)
		for (i = 0; i < (time_block - sample_time); i++)
		{
			coefficient[(time_block - sample_time) * 4 + app_count + 1 + i][i * variable + find_variableName_position(variable_name, "Pdischarge")] = 1.0; //Pess -
			coefficient[(time_block - sample_time) * 4 + app_count + 1 + i][i * variable + find_variableName_position(variable_name, "Z")] = Pbat_min;	   //Z
		}
		for (i = 1; i <= (time_block - sample_time); i++)
		{
			glp_set_row_name(mip, ((time_block - sample_time) * 4 + app_count + 1 + i), "");
			glp_set_row_bnds(mip, ((time_block - sample_time) * 4 + app_count + 1 + i), GLP_UP, 0.0, Pbat_min);
		}

		// (Battery power) Pdischarge max <= Pess j <= Pcharge max
		for (i = 0; i < (time_block - sample_time); i++)
		{
			coefficient[(time_block - sample_time) * 5 + app_count + 1 + i][i * variable + find_variableName_position(variable_name, "Pess")] = 1.0;	   //Pess
			coefficient[(time_block - sample_time) * 5 + app_count + 1 + i][i * variable + find_variableName_position(variable_name, "Pcharge")] = -1.0;   //Pess +
			coefficient[(time_block - sample_time) * 5 + app_count + 1 + i][i * variable + find_variableName_position(variable_name, "Pdischarge")] = 1.0; //Pess -
		}
		for (i = 1; i <= (time_block - sample_time); i++)
		{
			glp_set_row_name(mip, ((time_block - sample_time) * 5 + app_count + 1 + i), "");
			glp_set_row_bnds(mip, ((time_block - sample_time) * 5 + app_count + 1 + i), GLP_FX, 0.0, 0.0);
		}
	}

	//(Uninterrupted load of auxiliary variables), sum = 1
	// sum(δa j) = 1 (uninterrupt loads)
	int counter = 0;
	for (h = 0; h < uninterrupt_num; h++)
	{
		if (uninterrupt_flag[h] == 0)					 //(When the uninterruptible load is not started, use the original way to fill the matrix)
		{												 //?Y?w��?�X??N��j??��N��??{�X_?????? ��q "start????" ?? "start???? + ???U?�P�X???????"
			if ((uninterrupt_end[h] - sample_time) >= 0) //If it is already started, it will force the schedule to start from "start time" to "start time + left to be executed"
			{
				if ((uninterrupt_start[h] - sample_time) >= 0)
				{
					for (i = (uninterrupt_start[h] - sample_time); i <= ((uninterrupt_end[h] - uninterrupt_reot[h] + 1) - sample_time); i++)
					{
						coefficient[(time_block - sample_time) * 7 + app_count + 1 + counter][i * variable + find_variableName_position(variable_name, "uninterDelta" + to_string(h + 1))] = 1.0;
					}
				}
				else if ((uninterrupt_start[h] - sample_time) < 0)
				{
					for (i = 0; i <= ((uninterrupt_end[h] - uninterrupt_reot[h] + 1) - sample_time); i++)
					{
						coefficient[(time_block - sample_time) * 7 + app_count + 1 + counter][i * variable + find_variableName_position(variable_name, "uninterDelta" + to_string(h + 1))] = 1.0;
					}
				}
			}
			counter += 1;
		}
	}
	//(Varying load of auxiliary variables), sum = 1
	// sum(δa j) = 1 (varying loads)
	for (h = 0; h < varying_num; h++)
	{
		if (varying_flag[h] == 0)
		{
			if ((varying_end[h] - sample_time) >= 0)
			{
				if ((varying_start[h] - sample_time) >= 0)
				{
					for (i = (varying_start[h] - sample_time); i <= ((varying_end[h] - varying_reot[h] + 1) - sample_time); i++)
					{
						coefficient[(time_block - sample_time) * 7 + app_count + 1 + counter][i * variable + find_variableName_position(variable_name, "varyingDelta" + to_string(h + 1))] = 1.0;
					}
				}
				else if ((varying_start[h] - sample_time) < 0)
				{
					for (i = 0; i <= ((varying_end[h] - varying_reot[h] + 1) - sample_time); i++)
					{
						coefficient[(time_block - sample_time) * 7 + app_count + 1 + counter][i * variable + find_variableName_position(variable_name, "varyingDelta" + to_string(h + 1))] = 1.0;
					}
				}
			}
			counter += 1;
		}
	}

	// ra j+n >= δa j (uninterrupt loads)
	n = 0;
	for (h = 0; h < uninterrupt_num; h++)
	{
		if (uninterrupt_flag[h] == 0)
		{
			for (k = (7 + n), m = 0; k < (7 + n) + uninterrupt_reot[h], m < uninterrupt_reot[h]; k++, m++)
			{
				if ((uninterrupt_end[h] - sample_time) >= 0)
				{
					if ((uninterrupt_start[h] - sample_time) >= 0)
					{
						for (i = (uninterrupt_start[h] - sample_time); i <= ((uninterrupt_end[h] - uninterrupt_reot[h] + 1) - sample_time); i++)
						{
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][(i + m) * variable + find_variableName_position(variable_name, "uninterrupt" + to_string(h + 1))] = 1.0;
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][i * variable + find_variableName_position(variable_name, "uninterDelta" + to_string(h + 1))] = -1.0;
						}
					}
					else if ((uninterrupt_start[h] - sample_time) < 0)
					{
						for (i = 0; i <= ((uninterrupt_end[h] - uninterrupt_reot[h] + 1) - sample_time); i++)
						{
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][(i + m) * variable + find_variableName_position(variable_name, "uninterrupt" + to_string(h + 1))] = 1.0;
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][i * variable + find_variableName_position(variable_name, "uninterDelta" + to_string(h + 1))] = -1.0;
						}
					}
				}
			}
			n += uninterrupt_reot[h];
		}
		if (uninterrupt_flag[h] == 1)
		{
			if ((uninterrupt_end[h] - sample_time) >= 0)
			{
				if ((uninterrupt_start[h] - sample_time) <= 0)
				{
					for (i = 0; i <= (uninterrupt_end[h] - sample_time); i++)
					{
						coefficient[(time_block - sample_time) * (7 + n) + app_count + 1 + counter + i][i * variable + find_variableName_position(variable_name, "uninterrupt" + to_string(h + 1))] = 1.0;
					}
				}
				n += 1;
			}
		}
	}
	// ra j+n >= δa j (varying loads)
	for (h = 0; h < varying_num; h++)
	{
		if (varying_flag[h] == 0) //?b??�X??t????��?�X???
		{
			for (k = (7 + n), m = 0; k < (7 + n) + varying_reot[h], m < varying_reot[h]; k++, m++)
			{
				if ((varying_end[h] - sample_time) >= 0)
				{
					if ((varying_start[h] - sample_time) >= 0)
					{
						for (i = (varying_start[h] - sample_time); i <= ((varying_end[h] - varying_reot[h] + 1) - sample_time); i++)
						{
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][(i + m) * variable + find_variableName_position(variable_name, "varying" + to_string(h + 1))] = 1.0;
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][i * variable + find_variableName_position(variable_name, "varyingDelta" + to_string(h + 1))] = -1.0;
						}
					}
					else if ((varying_start[h] - sample_time) < 0)
					{
						for (i = 0; i <= ((varying_end[h] - varying_reot[h] + 1) - sample_time); i++)
						{
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][(i + m) * variable + find_variableName_position(variable_name, "varying" + to_string(h + 1))] = 1.0;
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][i * variable + find_variableName_position(variable_name, "varyingDelta" + to_string(h + 1))] = -1.0;
						}
					}
				}
			}
			n += varying_reot[h];
		}
		if (varying_flag[h] == 1)
		{
			if ((varying_end[h] - sample_time) >= 0)
			{
				if ((varying_start[h] - sample_time) <= 0)
				{
					for (i = 0; i <= (varying_end[h] - sample_time); i++)
					{
						coefficient[(time_block - sample_time) * (7 + n) + app_count + 1 + counter + i][i * variable + find_variableName_position(variable_name, "varying" + to_string(h + 1))] = 1.0;
					}
				}
				n += 1;
			}
		}
	}

	// ψa j+n  >= δa j * σa n
	for (h = 0; h < varying_num; h++)
	{
		if (varying_flag[h] == 0)
		{
			for (k = (7 + n), m = 0; k < (7 + n) + varying_reot[h], m < varying_reot[h]; k++, m++)
			{
				if ((varying_end[h] - sample_time) >= 0)
				{
					if ((varying_start[h] - sample_time) >= 0)
					{
						for (i = (varying_start[h] - sample_time); i <= ((varying_end[h] - varying_reot[h] + 1) - sample_time); i++)
						{
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][(i * variable) + find_variableName_position(variable_name, "varyingDelta" + to_string(h + 1))] = -1.0 * (((float)varying_t_d[h][i]) * (varying_p_d[h][m]));
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][((i + m) * variable) + find_variableName_position(variable_name, "varyingPsi" + to_string(h + 1))] = 1.0; // ψa j+n
						}
					}
					else if ((varying_start[h] - sample_time) < 0)
					{
						for (i = 0; i <= ((varying_end[h] - varying_reot[h] + 1) - sample_time); i++)
						{
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][(i * variable) + find_variableName_position(variable_name, "varyingDelta" + to_string(h + 1))] = -1.0 * (((float)varying_t_d[h][i]) * (varying_p_d[h][m]));
							coefficient[(time_block - sample_time) * k + app_count + 1 + counter + i][((i + m) * variable) + find_variableName_position(variable_name, "varyingPsi" + to_string(h + 1))] = 1.0; // ψa j+n
						}
					}
				}
			}
			n += varying_reot[h];
		}
		if (varying_flag[h] == 1)
		{
			//h?X?W
			if ((varying_end[h] - sample_time) >= 0)
			{
				if ((varying_start[h] - sample_time) >= 0)
				{
					for (i = (varying_start[h] - sample_time); i <= (varying_end[h] - sample_time); i++)
					{
						coefficient[(time_block - sample_time) * (7 + n) + app_count + 1 + counter + i][(i * variable) + find_variableName_position(variable_name, "varying" + to_string(h + 1))] = -1.0 * ((float)(varying_t_d[h][i]) * (varying_p_d[h][i + buff[h + interrupt_num + uninterrupt_num]]));
						coefficient[(time_block - sample_time) * (7 + n) + app_count + 1 + counter + i][(i * variable) + find_variableName_position(variable_name, "varyingPsi" + to_string(h + 1))] = 1.0; // ψa j+n
					}
				}
				else if ((varying_start[h] - sample_time) < 0)
				{
					for (i = 0; i <= (varying_end[h] - sample_time); i++)
					{
						coefficient[(time_block - sample_time) * (7 + n) + app_count + 1 + counter + i][(i * variable) + find_variableName_position(variable_name, "varying" + to_string(h + 1))] = -1.0 * ((float)(varying_t_d[h][i]) * (varying_p_d[h][i + buff[h + interrupt_num + uninterrupt_num]]));
						coefficient[(time_block - sample_time) * (7 + n) + app_count + 1 + counter + i][(i * variable) + find_variableName_position(variable_name, "varyingPsi" + to_string(h + 1))] = 1.0; // ψa j+n
					}
				}
			}
			n += 1;
		}
	}
	// sum(δa j) = 1 (uninterrupt loads)
	counter = 1;
	for (h = 0; h < uninterrupt_num; h++)
	{
		if (uninterrupt_flag[h] == 0)
		{
			glp_set_row_name(mip, ((time_block - sample_time) * 7 + app_count + 1 + counter), "");
			glp_set_row_bnds(mip, ((time_block - sample_time) * 7 + app_count + 1 + counter), GLP_FX, 1.0, 1.0);

			counter += 1;
		}
	}
	// sum(δa j) = 1 (varying loads)
	for (h = 0; h < varying_num; h++)
	{
		if (varying_flag[h] == 0)
		{
			glp_set_row_name(mip, ((time_block - sample_time) * 7 + app_count + 1 + counter), "");
			glp_set_row_bnds(mip, ((time_block - sample_time) * 7 + app_count + 1 + counter), GLP_FX, 1.0, 1.0);

			counter += 1;
		}
	}
	// ra j+n >= δa j (uninterrupt loads)
	n = 0;
	for (h = 0; h < uninterrupt_num; h++)
	{
		if (uninterrupt_flag[h] == 0)
		{
			for (k = (7 + n); k < (7 + n) + uninterrupt_reot[h]; k++)
			{
				for (i = ((time_block - sample_time) * k + app_count + 1 + counter); i < ((time_block - sample_time) * (k + 1) + app_count + 1 + counter); i++)
				{
					glp_set_row_name(mip, i, "");
					glp_set_row_bnds(mip, i, GLP_LO, 0.0, 0.0);
				}
			}
			n += uninterrupt_reot[h];
		}
		if (uninterrupt_flag[h] == 1)
		{
			if ((uninterrupt_end[h] - sample_time) >= 0)
			{
				for (i = ((time_block - sample_time) * (7 + n) + app_count + 1 + counter); i < ((time_block - sample_time) * (7 + n) + app_count + 1 + counter + uninterrupt_reot[h]); i++)
				{
					glp_set_row_name(mip, i, "");
					glp_set_row_bnds(mip, i, GLP_LO, 1.0, 1.0);
				}
				for (i = ((time_block - sample_time) * (7 + n) + app_count + 1 + counter + uninterrupt_reot[h]); i < ((time_block - sample_time) * ((7 + n) + 1) + app_count + 1 + counter); i++)
				{
					glp_set_row_name(mip, i, "");
					glp_set_row_bnds(mip, i, GLP_LO, 0.0, 0.0);
				}
				n += 1;
			}
		}
	}
	// ra j+n >= δa j (varying loads)
	for (h = 0; h < varying_num; h++)
	{
		if (varying_flag[h] == 0)
		{

			for (k = (7 + n); k < (7 + n) + varying_reot[h]; k++)
			{
				for (i = ((time_block - sample_time) * k + app_count + 1 + counter); i < ((time_block - sample_time) * (k + 1) + app_count + 1 + counter); i++)
				{
					glp_set_row_name(mip, i, "");
					glp_set_row_bnds(mip, i, GLP_LO, 0.0, 0.0);
				}
			}
			n += varying_reot[h];
		}
		if (varying_flag[h] == 1)
		{
			for (i = ((time_block - sample_time) * (7 + n) + app_count + 1 + counter); i < (((time_block - sample_time) * (7 + n) + app_count + 1 + counter) + varying_reot[h]); i++)
			{
				glp_set_row_name(mip, i, "");
				glp_set_row_bnds(mip, i, GLP_LO, 1.0, 1.0);
			}
			for (i = (((time_block - sample_time) * (7 + n) + app_count + 1 + counter) + varying_reot[h]); i < ((time_block - sample_time) * ((7 + n) + 1) + app_count + 1 + counter); i++)
			{
				glp_set_row_name(mip, i, "");
				glp_set_row_bnds(mip, i, GLP_LO, 0.0, 0.0);
			}
			n += 1;
		}
	}

	// ψa j+n >= δa j * σa n
	for (h = 0; h < varying_num; h++)
	{
		if (varying_flag[h] == 0)
		{
			for (k = (7 + n); k < (7 + n) + varying_reot[h]; k++)
			{
				for (i = ((time_block - sample_time) * k + app_count + 1 + counter); i < ((time_block - sample_time) * (k + 1) + app_count + 1 + counter); i++)
				{
					glp_set_row_name(mip, i, "");
					glp_set_row_bnds(mip, i, GLP_LO, 0.0, 0.0);
				}
			}
			n += varying_reot[h];
		}
		if (varying_flag[h] == 1)
		{
			for (i = ((time_block - sample_time) * (7 + n) + app_count + 1 + counter); i < ((time_block - sample_time) * ((7 + n) + 1) + app_count + 1 + counter); i++)
			{
				glp_set_row_name(mip, i, "");
				glp_set_row_bnds(mip, i, GLP_LO, 0.0, 0.0);
			}
			n += 1;
		}
	}

	for (j = 0; j < (time_block - sample_time); j++)
	{
		glp_set_obj_coef(mip, (find_variableName_position(variable_name, "Pgrid") + 1 + j * variable), price[j + sample_time] * delta_T);
	}
	if (dr_mode != 0)
	{
		if (sample_time - dr_startTime >= 0)
		{
			for (j = 0; j < dr_endTime - sample_time; j++)
			{
				glp_set_obj_coef(mip, (find_variableName_position(variable_name, "Pgrid") + 1 + j * variable), dr_feedback_price * delta_T);
			}
		}
		else if (sample_time - dr_startTime < 0)
		{
			for (j = dr_startTime - sample_time; j < dr_endTime - sample_time; j++)
			{
				glp_set_obj_coef(mip, (find_variableName_position(variable_name, "Pgrid") + 1 + j * variable), dr_feedback_price * delta_T);
			}
		}
	}

	/*==============================GLPK?g?J��x�X}(ia,ja,ar)===============================*/
	int *ia = new int[rowTotal * colTotal + 1];		  //Row
	int *ja = new int[rowTotal * colTotal + 1];		  //Column
	double *ar = new double[rowTotal * colTotal + 1]; //structural variable
	for (i = 0; i < rowTotal; i++)
	{
		for (j = 0; j < colTotal; j++)
		{
			ia[i * ((time_block - sample_time) * variable) + j + 1] = i + 1;
			ja[i * ((time_block - sample_time) * variable) + j + 1] = j + 1;
			ar[i * ((time_block - sample_time) * variable) + j + 1] = coefficient[i][j];
		}
	}
	/*==============================GLPK????????��x�X}====================================*/
	glp_load_matrix(mip, rowTotal * colTotal, ia, ja, ar);

	glp_iocp parm;
	glp_init_iocp(&parm);

	if (sample_time == 0)
		parm.tm_lim = 120000;
	else
		parm.tm_lim = 60000;

	parm.presolve = GLP_ON;
	//not cloudy
	// parm.ps_heur = GLP_ON;
	// parm.bt_tech = GLP_BT_BPH;
	// parm.br_tech = GLP_BR_PCH;

	//cloud
	parm.gmi_cuts = GLP_ON;
	parm.ps_heur = GLP_ON;
	parm.bt_tech = GLP_BT_BFS;
	parm.br_tech = GLP_BR_PCH;

	//no fc+ no sell
	//fc+no sell
	// parm.gmi_cuts = GLP_ON;
	// parm.bt_tech = GLP_BT_BPH;
	// parm.br_tech = GLP_BR_PCH;

	//FC+sell
	//parm.fp_heur = GLP_ON;
	// parm.bt_tech = GLP_BT_BPH;
	//parm.br_tech = GLP_BR_PCH;

	int err = glp_intopt(mip, &parm);

	z = glp_mip_obj_val(mip);
	printf("\n");
	printf("LINE %d: timeblock %d household id %d sol = %f; \n", __LINE__, sample_time, household_id, z);

	if (z == 0.0 && glp_mip_col_val(mip, find_variableName_position(variable_name, "SOC") + 1) == 0.0)
	{
		printf("Error > sol is 0, No Solution, give up the solution\n");
		printf("%.2f\n", glp_mip_col_val(mip, find_variableName_position(variable_name, "Pgrid") + 1));
		system("pause");
		exit(1);
	}

	/*==============================��N?M?????????G???X==================================*/
	int l = 0;
	float *s = new float[time_block];

	//get now time that can used in the real experiment
	if (((now_time.tm_min) % (60 / divide)) != 0)
		noo = (now_time.tm_hour) * divide + (int)((now_time.tm_min) / (60 / divide));
	else
		noo = (now_time.tm_hour) * divide + (int)((now_time.tm_min) / (60 / divide));

	for (i = 1; i <= variable; i++)
	{
		h = i;
		l = variable - (app_count - i); // get interrupt & varying ra j
		if (sample_time == 0)
		{
			for (j = 0; j < time_block; j++)
			{
				s[j] = glp_mip_col_val(mip, h);
				if ((i > interrupt_num + uninterrupt_num) && (i <= app_count)) //sometimes varying load will have weird, use power model instead of varying load
				{
					s[j] = glp_mip_col_val(mip, l);
					if (s[j] == varying_p_pow[0][0] || s[j] == varying_p_pow[0][1] || s[j] == varying_p_pow[0][2])
						s[j] = 1.0;
				}
				h = (h + variable);
				l = (l + variable);
			}
			// =-=-=-=-=-=-=-=-=-=- update each variables's A0 ~ A95 in each for loop -=-=-=-=-=-=-=-=-=-= //
			snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO LHEMS_control_status (%s, equip_name, household_id) VALUES('%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%s', '%d');", column, s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15], s[16], s[17], s[18], s[19], s[20], s[21], s[22], s[23], s[24], s[25], s[26], s[27], s[28], s[29], s[30], s[31], s[32], s[33], s[34], s[35], s[36], s[37], s[38], s[39], s[40], s[41], s[42], s[43], s[44], s[45], s[46], s[47], s[48], s[49], s[50], s[51], s[52], s[53], s[54], s[55], s[56], s[57], s[58], s[59], s[60], s[61], s[62], s[63], s[64], s[65], s[66], s[67], s[68], s[69], s[70], s[71], s[72], s[73], s[74], s[75], s[76], s[77], s[78], s[79], s[80], s[81], s[82], s[83], s[84], s[85], s[86], s[87], s[88], s[89], s[90], s[91], s[92], s[93], s[94], s[95], variable_name[i - 1].c_str(), household_id);
			sent_query();
		}

		if (sample_time != 0)
		{
			// =-=-=-=-=-=-=-=-=-=- history about the control status from each control id -=-=-=-=-=-=-=-=-=-= //
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM LHEMS_control_status WHERE equip_name = '%s' and household_id = %d", column, variable_name[i - 1].c_str(), household_id);
			fetch_row_value();
			for (int k = 0; k < sample_time; k++)
			{
				s[k] = turn_float(k);
			}

			// =-=-=-=-=-=-=-=-=-=- change new result after the sample time -=-=-=-=-=-=-=-=-=-= //
			for (j = 0; j < (time_block - sample_time); j++)
			{
				s[j + sample_time] = glp_mip_col_val(mip, h);
				if ((i > interrupt_num + uninterrupt_num) && (i <= app_count)) //sometimes varying load will have weird, use power model instead of varying load
				{
					s[j + sample_time] = glp_mip_col_val(mip, l);
					if (s[j + sample_time] == varying_p_pow[0][0] || s[j + sample_time] == varying_p_pow[0][1] || s[j + sample_time] == varying_p_pow[0][2])
						s[j + sample_time] = 1.0;
				}
				h = (h + variable);
				l = (l + variable);
			}
			// =-=-=-=-=-=-=-=-=-=- full result update -=-=-=-=-=-=-=-=-=-= //
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE LHEMS_control_status set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f' WHERE equip_name = '%s' and household_id = %d;", s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15], s[16], s[17], s[18], s[19], s[20], s[21], s[22], s[23], s[24], s[25], s[26], s[27], s[28], s[29], s[30], s[31], s[32], s[33], s[34], s[35], s[36], s[37], s[38], s[39], s[40], s[41], s[42], s[43], s[44], s[45], s[46], s[47], s[48], s[49], s[50], s[51], s[52], s[53], s[54], s[55], s[56], s[57], s[58], s[59], s[60], s[61], s[62], s[63], s[64], s[65], s[66], s[67], s[68], s[69], s[70], s[71], s[72], s[73], s[74], s[75], s[76], s[77], s[78], s[79], s[80], s[81], s[82], s[83], s[84], s[85], s[86], s[87], s[88], s[89], s[90], s[91], s[92], s[93], s[94], s[95], variable_name[i - 1].c_str(), household_id);
			sent_query();

			// =-=-=-=-=-=-=-=-=-=- result update from the sample time until end timeblock (96) -=-=-=-=-=-=-=-=-=-= //
			for (j = 0; j < sample_time; j++)
			{
				s[j] = 0;
			}
			snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO LHEMS_real_status (%s, equip_name, household_id) VALUES('%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%s', '%d');", column, s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15], s[16], s[17], s[18], s[19], s[20], s[21], s[22], s[23], s[24], s[25], s[26], s[27], s[28], s[29], s[30], s[31], s[32], s[33], s[34], s[35], s[36], s[37], s[38], s[39], s[40], s[41], s[42], s[43], s[44], s[45], s[46], s[47], s[48], s[49], s[50], s[51], s[52], s[53], s[54], s[55], s[56], s[57], s[58], s[59], s[60], s[61], s[62], s[63], s[64], s[65], s[66], s[67], s[68], s[69], s[70], s[71], s[72], s[73], s[74], s[75], s[76], s[77], s[78], s[79], s[80], s[81], s[82], s[83], s[84], s[85], s[86], s[87], s[88], s[89], s[90], s[91], s[92], s[93], s[94], s[95], variable_name[i - 1].c_str(), household_id);
			sent_query();
		}
	}

	float now_grid[time_block] = {0.0}, varying_grid[time_block] = {0.0}, cost[time_block] = {0.0};
	float now_power_result = 0.0, var_grid_result = 0.0, opt_cost_result = 0.0, price_sum_now_power = 0.0;
	// float sell[time_block] = {0.0}, opt_sell_result = 0.0 ;
	float FC_cost[time_block] = {0.0}, Hydrogen_com[time_block] = {0.0}, FC_every_cost[time_block] = {0.0};
	float opt_FC_cost_result = 0.0, opt_Hydrogen_result = 0.0;

	// the total loads power(now_grid) and cost by grid(varying_grid) from 'sample time' to the 'end time (96)'
	// sum all now_grid(now_power_result) and sum all varying_grid(var_grid_result) from 'sample time' to the 'end time (96)'
	//get all_grid cost(use the time varing price)
	for (j = 0; j < (time_block - sample_time); j++)
	{
		// for (i = 1; i < (app_count + 1); i++)
		// {
		// 	if (i < (interrupt_num + 1))
		// 	{
		// 		h = i + variable * j;
		// 		s[j] = glp_mip_col_val(mip, h);
		// 		now_grid[j + sample_time] += (s[j]) * interrupt_p[i - 1] * delta_T;
		// 		varying_grid[j + sample_time] += (s[j]) * interrupt_p[i - 1] * price[j + sample_time] * delta_T;
		// 	}
		// 	else if (i >= (interrupt_num + 1) && i < (interrupt_num + uninterrupt_num + 1))
		// 	{
		// 		h = i + variable * j;
		// 		s[j] = glp_mip_col_val(mip, h);
		// 		now_grid[j + sample_time] += (s[j]) * uninterrupt_p[i - 1 - interrupt_num] * delta_T;
		// 		varying_grid[j + sample_time] += (s[j]) * uninterrupt_p[i - 1 - interrupt_num] * price[j + sample_time] * delta_T;
		// 	}
		// 	else if (i >= (interrupt_num + uninterrupt_num + 1) && i < (interrupt_num + uninterrupt_num + varying_num + 1))
		// 	{
		// 		k = variable;
		// 		s[j] = glp_mip_col_val(mip, k);
		// 		now_grid[j + sample_time] += (float)(glp_mip_col_val(mip, i + variable * j)) * s[j] * delta_T;
		// 		varying_grid[j + sample_time] += (float)(glp_mip_col_val(mip, i + variable * j)) * s[j] * price[j + sample_time] * delta_T;
		// 	}
		// }

		now_power_result += now_grid[j + sample_time];	  //now_power_result¡÷ just all the comsumption energy(for no varying price and coculate leter).
		var_grid_result += varying_grid[j + sample_time]; //var_power_result¡÷ comsumption energy*price.
	}

	// buy how much grid (cost) from 'sample time' to the 'end time (96)'
	// sum all cost(opt_cost_result) from 'sample time' to the 'end time (96)'
	h = find_variableName_position(variable_name, "Pgrid") + 1; //cost
	for (j = 0; j < (time_block - sample_time); j++)
	{
		s[j] = glp_mip_col_val(mip, h);
		cost[j + sample_time] = s[j] * price[j + sample_time] * delta_T;
		opt_cost_result += cost[j + sample_time];
		h = (h + variable);
	}
	// updateTableCost(now_grid, varying_grid, cost, FC_cost, Hydrogen_com, FC_every_cost, now_power_result, var_grid_result, opt_cost_result, opt_FC_cost_result, opt_Hydrogen_result, price_sum_now_power);

	glp_delete_prob(mip);
	delete[] ia, ja, ar, s;
	delete[] coefficient;
	return;
}

int determine_realTimeOrOneDayMode_andGetSOC(int real_time, vector<string> variable_name)
{
	// 'Realtime mode' if same day & real time = 1;
	// 'One day mode' =>
	// 		1. SOC = 0.7 if real_time = 0,
	// 		2. Use Previous SOC if real_time = 1.
	functionPrint(__func__);
	if (real_time == 1)
	{
		messagePrint(__LINE__, "Real Time Mode...", 'S', 0, 'Y');

		if (household_id == 1)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "TRUNCATE TABLE LHEMS_real_status"); //clean LHEMS_real_status;
			sent_query();
		}

		// get previous SOC value
		if (Pess_flag)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM LHEMS_control_status WHERE equip_name = '%s' and household_id = %d", sample_time - 1, "SOC", household_id);
			SOC_ini = turn_value_to_float(0);
			messagePrint(__LINE__, "SOC = ", 'F', SOC_ini, 'Y');
		}
	}
	else
	{
		if (household_id == 1)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "TRUNCATE TABLE LHEMS_control_status");
			sent_query();
			snprintf(sql_buffer, sizeof(sql_buffer), "TRUNCATE TABLE LHEMS_real_status");
			sent_query();
			sample_time = 0;
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %d WHERE parameter_name = 'next_simulate_timeblock' ", sample_time);
			sent_query();
		}

		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value FROM BaseParameter WHERE parameter_name = 'ini_SOC'"); //get ini_SOC
		SOC_ini = turn_value_to_float(0);
		messagePrint(__LINE__, "ini_SOC : ", 'F', SOC_ini, 'Y');

		if (household_id == householdTotal)
		{
			real_time = 1; //if you don't want do real_time,please commend it.
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %d WHERE parameter_name = 'real_time' ", real_time);
			sent_query();
		}
	}

	return real_time;
}

void getOrUpdate_SolarInfo_ThroughSampleTime(const char *weather, float *solar2)
{
	functionPrint(__func__);
	printf("\tWeather : %s\n", weather);
	if (sample_time == 0)
	{
		for (i = 0; i < time_block; i++)
		{

			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM solar_data WHERE time_block = %d", weather, i);
			solar2[i] = turn_value_to_float(0);

			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE solar_day SET value =%.3f WHERE time_block = %d", solar2[i], i);
			sent_query();
		}
	}
	else
	{
		for (i = 0; i < time_block; i++)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value FROM solar_day WHERE time_block = %d", i);
			solar2[i] = turn_value_to_float(0);
		}
	}
}

void updateTableCost(float *now_grid, float *varying_grid, float *cost, float *FC_cost, float *Hydrogen_com, float *FC_every_cost, float now_power_result, float var_grid_result, float opt_cost_result, float opt_FC_cost_result, float opt_Hydrogen_result, float price_sum_now_power)
{
	functionPrint(__func__);
	// NOTE:
	// the total loads power(now_grid) in each time block, sum all now_grid(now_power_result)
	// cost by grid(varying_grid) in each time block, sum all varying_grid(var_grid_result)
	// buy how much grid (cost) in each time block, sum all cost(opt_cost_result)
	// sell how much grid (sell) in each time block, sum all sell(opt_sell_result)
	// cost how much price to generate power by hydrogen (FC_cost) in each time block, sum all FC_cost(opt_FC_cost_result)
	// cost how much hydrogen gram to generate power (Hydrogen_com) in each time block, sum all Hydrogen_com (opt_Hydrogen_result)
	// cost by FC divide by real power generate by FC (FC_cost / Pfc) (FC_every_cost)

	// insert cost info when sample time = 0, and update cost info from sample time to end to 'cost' table
	if (sample_time == 0)
	{
		printf("LINE %d: sample_time : %d\n\t", __LINE__, sample_time);
		//now grid
		printf("LINE %d: Insert cost_name : %s\n\t", __LINE__, "total_load_power");
		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "total_load_power", now_grid[0], now_grid[1], now_grid[2], now_grid[3], now_grid[4], now_grid[5], now_grid[6], now_grid[7], now_grid[8], now_grid[9], now_grid[10], now_grid[11], now_grid[12], now_grid[13], now_grid[14], now_grid[15], now_grid[16], now_grid[17], now_grid[18], now_grid[19], now_grid[20], now_grid[21], now_grid[22], now_grid[23], now_grid[24], now_grid[25], now_grid[26], now_grid[27], now_grid[28], now_grid[29], now_grid[30], now_grid[31], now_grid[32], now_grid[33], now_grid[34], now_grid[35], now_grid[36], now_grid[37], now_grid[38], now_grid[39], now_grid[40], now_grid[41], now_grid[42], now_grid[43], now_grid[44], now_grid[45], now_grid[46], now_grid[47], now_grid[48], now_grid[49], now_grid[50], now_grid[51], now_grid[52], now_grid[53], now_grid[54], now_grid[55], now_grid[56], now_grid[57], now_grid[58], now_grid[59], now_grid[60], now_grid[61], now_grid[62], now_grid[63], now_grid[64], now_grid[65], now_grid[66], now_grid[67], now_grid[68], now_grid[69], now_grid[70], now_grid[71], now_grid[72], now_grid[73], now_grid[74], now_grid[75], now_grid[76], now_grid[77], now_grid[78], now_grid[79], now_grid[80], now_grid[81], now_grid[82], now_grid[83], now_grid[84], now_grid[85], now_grid[86], now_grid[87], now_grid[88], now_grid[89], now_grid[90], now_grid[91], now_grid[92], now_grid[93], now_grid[94], now_grid[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 22 ", now_power_result);
		sent_query();

		//variing grid
		printf("LINE %d: Insert cost_name : %s\n\t", __LINE__, "total_load_price");
		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "total_load_price", varying_grid[0], varying_grid[1], varying_grid[2], varying_grid[3], varying_grid[4], varying_grid[5], varying_grid[6], varying_grid[7], varying_grid[8], varying_grid[9], varying_grid[10], varying_grid[11], varying_grid[12], varying_grid[13], varying_grid[14], varying_grid[15], varying_grid[16], varying_grid[17], varying_grid[18], varying_grid[19], varying_grid[20], varying_grid[21], varying_grid[22], varying_grid[23], varying_grid[24], varying_grid[25], varying_grid[26], varying_grid[27], varying_grid[28], varying_grid[29], varying_grid[30], varying_grid[31], varying_grid[32], varying_grid[33], varying_grid[34], varying_grid[35], varying_grid[36], varying_grid[37], varying_grid[38], varying_grid[39], varying_grid[40], varying_grid[41], varying_grid[42], varying_grid[43], varying_grid[44], varying_grid[45], varying_grid[46], varying_grid[47], varying_grid[48], varying_grid[49], varying_grid[50], varying_grid[51], varying_grid[52], varying_grid[53], varying_grid[54], varying_grid[55], varying_grid[56], varying_grid[57], varying_grid[58], varying_grid[59], varying_grid[60], varying_grid[61], varying_grid[62], varying_grid[63], varying_grid[64], varying_grid[65], varying_grid[66], varying_grid[67], varying_grid[68], varying_grid[69], varying_grid[70], varying_grid[71], varying_grid[72], varying_grid[73], varying_grid[74], varying_grid[75], varying_grid[76], varying_grid[77], varying_grid[78], varying_grid[79], varying_grid[80], varying_grid[81], varying_grid[82], varying_grid[83], varying_grid[84], varying_grid[85], varying_grid[86], varying_grid[87], varying_grid[88], varying_grid[89], varying_grid[90], varying_grid[91], varying_grid[92], varying_grid[93], varying_grid[94], varying_grid[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 23 ", var_grid_result);
		sent_query();

		////minimum cost
		printf("LINE %d: Insert cost_name : %s\n\t", __LINE__, "real_buy_grid_price");
		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "real_buy_grid_price", cost[0], cost[1], cost[2], cost[3], cost[4], cost[5], cost[6], cost[7], cost[8], cost[9], cost[10], cost[11], cost[12], cost[13], cost[14], cost[15], cost[16], cost[17], cost[18], cost[19], cost[20], cost[21], cost[22], cost[23], cost[24], cost[25], cost[26], cost[27], cost[28], cost[29], cost[30], cost[31], cost[32], cost[33], cost[34], cost[35], cost[36], cost[37], cost[38], cost[39], cost[40], cost[41], cost[42], cost[43], cost[44], cost[45], cost[46], cost[47], cost[48], cost[49], cost[50], cost[51], cost[52], cost[53], cost[54], cost[55], cost[56], cost[57], cost[58], cost[59], cost[60], cost[61], cost[62], cost[63], cost[64], cost[65], cost[66], cost[67], cost[68], cost[69], cost[70], cost[71], cost[72], cost[73], cost[74], cost[75], cost[76], cost[77], cost[78], cost[79], cost[80], cost[81], cost[82], cost[83], cost[84], cost[85], cost[86], cost[87], cost[88], cost[89], cost[90], cost[91], cost[92], cost[93], cost[94], cost[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 24 ", opt_cost_result);
		sent_query();
		/*
		//maximum sell
		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name,%s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "real_sell_grid_price", sell[0], sell[1], sell[2], sell[3], sell[4], sell[5], sell[6], sell[7], sell[8], sell[9], sell[10], sell[11], sell[12], sell[13], sell[14], sell[15], sell[16], sell[17], sell[18], sell[19], sell[20], sell[21], sell[22], sell[23], sell[24], sell[25], sell[26], sell[27], sell[28], sell[29], sell[30], sell[31], sell[32], sell[33], sell[34], sell[35], sell[36], sell[37], sell[38], sell[39], sell[40], sell[41], sell[42], sell[43], sell[44], sell[45], sell[46], sell[47], sell[48], sell[49], sell[50], sell[51], sell[52], sell[53], sell[54], sell[55], sell[56], sell[57], sell[58], sell[59], sell[60], sell[61], sell[62], sell[63], sell[64], sell[65], sell[66], sell[67], sell[68], sell[69], sell[70], sell[71], sell[72], sell[73], sell[74], sell[75], sell[76], sell[77], sell[78], sell[79], sell[80], sell[81], sell[82], sell[83], sell[84], sell[85], sell[86], sell[87], sell[88], sell[89], sell[90], sell[91], sell[92], sell[93], sell[94], sell[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 25 ", opt_sell_result);
		sent_query();
		*/
		//minimum FC cost
		printf("LINE %d: Insert cost_name : %s\n\t", __LINE__, "FC_price");
		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "FC_price", FC_cost[0], FC_cost[1], FC_cost[2], FC_cost[3], FC_cost[4], FC_cost[5], FC_cost[6], FC_cost[7], FC_cost[8], FC_cost[9], FC_cost[10], FC_cost[11], FC_cost[12], FC_cost[13], FC_cost[14], FC_cost[15], FC_cost[16], FC_cost[17], FC_cost[18], FC_cost[19], FC_cost[20], FC_cost[21], FC_cost[22], FC_cost[23], FC_cost[24], FC_cost[25], FC_cost[26], FC_cost[27], FC_cost[28], FC_cost[29], FC_cost[30], FC_cost[31], FC_cost[32], FC_cost[33], FC_cost[34], FC_cost[35], FC_cost[36], FC_cost[37], FC_cost[38], FC_cost[39], FC_cost[40], FC_cost[41], FC_cost[42], FC_cost[43], FC_cost[44], FC_cost[45], FC_cost[46], FC_cost[47], FC_cost[48], FC_cost[49], FC_cost[50], FC_cost[51], FC_cost[52], FC_cost[53], FC_cost[54], FC_cost[55], FC_cost[56], FC_cost[57], FC_cost[58], FC_cost[59], FC_cost[60], FC_cost[61], FC_cost[62], FC_cost[63], FC_cost[64], FC_cost[65], FC_cost[66], FC_cost[67], FC_cost[68], FC_cost[69], FC_cost[70], FC_cost[71], FC_cost[72], FC_cost[73], FC_cost[74], FC_cost[75], FC_cost[76], FC_cost[77], FC_cost[78], FC_cost[79], FC_cost[80], FC_cost[81], FC_cost[82], FC_cost[83], FC_cost[84], FC_cost[85], FC_cost[86], FC_cost[87], FC_cost[88], FC_cost[89], FC_cost[90], FC_cost[91], FC_cost[92], FC_cost[93], FC_cost[94], FC_cost[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 26 ", opt_FC_cost_result);
		sent_query();

		//minimum hydrogen consumption
		printf("LINE %d: Insert cost_name : %s\n\t", __LINE__, "hydrogen_consumption");
		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "hydrogen_consumption", Hydrogen_com[0], Hydrogen_com[1], Hydrogen_com[2], Hydrogen_com[3], Hydrogen_com[4], Hydrogen_com[5], Hydrogen_com[6], Hydrogen_com[7], Hydrogen_com[8], Hydrogen_com[9], Hydrogen_com[10], Hydrogen_com[11], Hydrogen_com[12], Hydrogen_com[13], Hydrogen_com[14], Hydrogen_com[15], Hydrogen_com[16], Hydrogen_com[17], Hydrogen_com[18], Hydrogen_com[19], Hydrogen_com[20], Hydrogen_com[21], Hydrogen_com[22], Hydrogen_com[23], Hydrogen_com[24], Hydrogen_com[25], Hydrogen_com[26], Hydrogen_com[27], Hydrogen_com[28], Hydrogen_com[29], Hydrogen_com[30], Hydrogen_com[31], Hydrogen_com[32], Hydrogen_com[33], Hydrogen_com[34], Hydrogen_com[35], Hydrogen_com[36], Hydrogen_com[37], Hydrogen_com[38], Hydrogen_com[39], Hydrogen_com[40], Hydrogen_com[41], Hydrogen_com[42], Hydrogen_com[43], Hydrogen_com[44], Hydrogen_com[45], Hydrogen_com[46], Hydrogen_com[47], Hydrogen_com[48], Hydrogen_com[49], Hydrogen_com[50], Hydrogen_com[51], Hydrogen_com[52], Hydrogen_com[53], Hydrogen_com[54], Hydrogen_com[55], Hydrogen_com[56], Hydrogen_com[57], Hydrogen_com[58], Hydrogen_com[59], Hydrogen_com[60], Hydrogen_com[61], Hydrogen_com[62], Hydrogen_com[63], Hydrogen_com[64], Hydrogen_com[65], Hydrogen_com[66], Hydrogen_com[67], Hydrogen_com[68], Hydrogen_com[69], Hydrogen_com[70], Hydrogen_com[71], Hydrogen_com[72], Hydrogen_com[73], Hydrogen_com[74], Hydrogen_com[75], Hydrogen_com[76], Hydrogen_com[77], Hydrogen_com[78], Hydrogen_com[79], Hydrogen_com[80], Hydrogen_com[81], Hydrogen_com[82], Hydrogen_com[83], Hydrogen_com[84], Hydrogen_com[85], Hydrogen_com[86], Hydrogen_com[87], Hydrogen_com[88], Hydrogen_com[89], Hydrogen_com[90], Hydrogen_com[91], Hydrogen_com[92], Hydrogen_com[93], Hydrogen_com[94], Hydrogen_com[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 27 ", opt_Hydrogen_result);
		sent_query();

		//FC cost every kW
		printf("LINE %d: Insert cost_name : %s\n\t", __LINE__, "FC_price_every_kW");
		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "FC_price_every_kW", FC_every_cost[0], FC_every_cost[1], FC_every_cost[2], FC_every_cost[3], FC_every_cost[4], FC_every_cost[5], FC_every_cost[6], FC_every_cost[7], FC_every_cost[8], FC_every_cost[9], FC_every_cost[10], FC_every_cost[11], FC_every_cost[12], FC_every_cost[13], FC_every_cost[14], FC_every_cost[15], FC_every_cost[16], FC_every_cost[17], FC_every_cost[18], FC_every_cost[19], FC_every_cost[20], FC_every_cost[21], FC_every_cost[22], FC_every_cost[23], FC_every_cost[24], FC_every_cost[25], FC_every_cost[26], FC_every_cost[27], FC_every_cost[28], FC_every_cost[29], FC_every_cost[30], FC_every_cost[31], FC_every_cost[32], FC_every_cost[33], FC_every_cost[34], FC_every_cost[35], FC_every_cost[36], FC_every_cost[37], FC_every_cost[38], FC_every_cost[39], FC_every_cost[40], FC_every_cost[41], FC_every_cost[42], FC_every_cost[43], FC_every_cost[44], FC_every_cost[45], FC_every_cost[46], FC_every_cost[47], FC_every_cost[48], FC_every_cost[49], FC_every_cost[50], FC_every_cost[51], FC_every_cost[52], FC_every_cost[53], FC_every_cost[54], FC_every_cost[55], FC_every_cost[56], FC_every_cost[57], FC_every_cost[58], FC_every_cost[59], FC_every_cost[60], FC_every_cost[61], FC_every_cost[62], FC_every_cost[63], FC_every_cost[64], FC_every_cost[65], FC_every_cost[66], FC_every_cost[67], FC_every_cost[68], FC_every_cost[69], FC_every_cost[70], FC_every_cost[71], FC_every_cost[72], FC_every_cost[73], FC_every_cost[74], FC_every_cost[75], FC_every_cost[76], FC_every_cost[77], FC_every_cost[78], FC_every_cost[79], FC_every_cost[80], FC_every_cost[81], FC_every_cost[82], FC_every_cost[83], FC_every_cost[84], FC_every_cost[85], FC_every_cost[86], FC_every_cost[87], FC_every_cost[88], FC_every_cost[89], FC_every_cost[90], FC_every_cost[91], FC_every_cost[92], FC_every_cost[93], FC_every_cost[94], FC_every_cost[95]);
		sent_query();
	}
	else
	{
		printf("LINE %d: sample time : %d\n\t", __LINE__, sample_time);
		//all grid
		printf("LINE %d: Update cost_name : %s\n\t", __LINE__, "total_load_power");
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM cost WHERE (cost_name = '%s')", column, "total_load_power");
		fetch_row_value();
		for (j = 0; j < sample_time; j++)
		{
			now_grid[j] = turn_float(j);
			now_power_result += now_grid[j];
		}
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f' WHERE control_id='%d';", now_grid[0], now_grid[1], now_grid[2], now_grid[3], now_grid[4], now_grid[5], now_grid[6], now_grid[7], now_grid[8], now_grid[9], now_grid[10], now_grid[11], now_grid[12], now_grid[13], now_grid[14], now_grid[15], now_grid[16], now_grid[17], now_grid[18], now_grid[19], now_grid[20], now_grid[21], now_grid[22], now_grid[23], now_grid[24], now_grid[25], now_grid[26], now_grid[27], now_grid[28], now_grid[29], now_grid[30], now_grid[31], now_grid[32], now_grid[33], now_grid[34], now_grid[35], now_grid[36], now_grid[37], now_grid[38], now_grid[39], now_grid[40], now_grid[41], now_grid[42], now_grid[43], now_grid[44], now_grid[45], now_grid[46], now_grid[47], now_grid[48], now_grid[49], now_grid[50], now_grid[51], now_grid[52], now_grid[53], now_grid[54], now_grid[55], now_grid[56], now_grid[57], now_grid[58], now_grid[59], now_grid[60], now_grid[61], now_grid[62], now_grid[63], now_grid[64], now_grid[65], now_grid[66], now_grid[67], now_grid[68], now_grid[69], now_grid[70], now_grid[71], now_grid[72], now_grid[73], now_grid[74], now_grid[75], now_grid[76], now_grid[77], now_grid[78], now_grid[79], now_grid[80], now_grid[81], now_grid[82], now_grid[83], now_grid[84], now_grid[85], now_grid[86], now_grid[87], now_grid[88], now_grid[89], now_grid[90], now_grid[91], now_grid[92], now_grid[93], now_grid[94], now_grid[95], 1);
		sent_query();

		//all grid
		printf("LINE %d: Update cost_name : %s\n\t", __LINE__, "total_load_price");
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM cost WHERE (cost_name = '%s')", column, "total_load_price");
		fetch_row_value();
		for (j = 0; j < sample_time; j++)
		{
			varying_grid[j] = turn_float(j);
			var_grid_result += varying_grid[j];
		}
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f' WHERE control_id='%d';", varying_grid[0], varying_grid[1], varying_grid[2], varying_grid[3], varying_grid[4], varying_grid[5], varying_grid[6], varying_grid[7], varying_grid[8], varying_grid[9], varying_grid[10], varying_grid[11], varying_grid[12], varying_grid[13], varying_grid[14], varying_grid[15], varying_grid[16], varying_grid[17], varying_grid[18], varying_grid[19], varying_grid[20], varying_grid[21], varying_grid[22], varying_grid[23], varying_grid[24], varying_grid[25], varying_grid[26], varying_grid[27], varying_grid[28], varying_grid[29], varying_grid[30], varying_grid[31], varying_grid[32], varying_grid[33], varying_grid[34], varying_grid[35], varying_grid[36], varying_grid[37], varying_grid[38], varying_grid[39], varying_grid[40], varying_grid[41], varying_grid[42], varying_grid[43], varying_grid[44], varying_grid[45], varying_grid[46], varying_grid[47], varying_grid[48], varying_grid[49], varying_grid[50], varying_grid[51], varying_grid[52], varying_grid[53], varying_grid[54], varying_grid[55], varying_grid[56], varying_grid[57], varying_grid[58], varying_grid[59], varying_grid[60], varying_grid[61], varying_grid[62], varying_grid[63], varying_grid[64], varying_grid[65], varying_grid[66], varying_grid[67], varying_grid[68], varying_grid[69], varying_grid[70], varying_grid[71], varying_grid[72], varying_grid[73], varying_grid[74], varying_grid[75], varying_grid[76], varying_grid[77], varying_grid[78], varying_grid[79], varying_grid[80], varying_grid[81], varying_grid[82], varying_grid[83], varying_grid[84], varying_grid[85], varying_grid[86], varying_grid[87], varying_grid[88], varying_grid[89], varying_grid[90], varying_grid[91], varying_grid[92], varying_grid[93], varying_grid[94], varying_grid[95], 2);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 23 ", var_grid_result);
		sent_query();

		//minimum cost
		printf("LINE %d: Update cost_name : %s\n\t", __LINE__, "real_buy_grid_price");
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM cost WHERE (cost_name = '%s')", column, "real_buy_grid_price");
		fetch_row_value();
		for (j = 0; j < sample_time; j++)
		{
			cost[j] = turn_float(j);
			opt_cost_result += cost[j];
		}
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f' WHERE control_id='%d';", cost[0], cost[1], cost[2], cost[3], cost[4], cost[5], cost[6], cost[7], cost[8], cost[9], cost[10], cost[11], cost[12], cost[13], cost[14], cost[15], cost[16], cost[17], cost[18], cost[19], cost[20], cost[21], cost[22], cost[23], cost[24], cost[25], cost[26], cost[27], cost[28], cost[29], cost[30], cost[31], cost[32], cost[33], cost[34], cost[35], cost[36], cost[37], cost[38], cost[39], cost[40], cost[41], cost[42], cost[43], cost[44], cost[45], cost[46], cost[47], cost[48], cost[49], cost[50], cost[51], cost[52], cost[53], cost[54], cost[55], cost[56], cost[57], cost[58], cost[59], cost[60], cost[61], cost[62], cost[63], cost[64], cost[65], cost[66], cost[67], cost[68], cost[69], cost[70], cost[71], cost[72], cost[73], cost[74], cost[75], cost[76], cost[77], cost[78], cost[79], cost[80], cost[81], cost[82], cost[83], cost[84], cost[85], cost[86], cost[87], cost[88], cost[89], cost[90], cost[91], cost[92], cost[93], cost[94], cost[95], 3);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 24 ", opt_cost_result);
		sent_query();
		/*
		//maximum sell
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM cost WHERE (control_id = '%d')", column, 4);
		fetch_row_value();
		for (j = 0; j < sample_time; j++)
		{
			sell[j] = turn_float(j);
			opt_sell_result += sell[j];
			
		}
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f' WHERE control_id='%d';", sell[0], sell[1], sell[2], sell[3], sell[4], sell[5], sell[6], sell[7], sell[8], sell[9], sell[10], sell[11], sell[12], sell[13], sell[14], sell[15], sell[16], sell[17], sell[18], sell[19], sell[20], sell[21], sell[22], sell[23], sell[24], sell[25], sell[26], sell[27], sell[28], sell[29], sell[30], sell[31], sell[32], sell[33], sell[34], sell[35], sell[36], sell[37], sell[38], sell[39], sell[40], sell[41], sell[42], sell[43], sell[44], sell[45], sell[46], sell[47], sell[48], sell[49], sell[50], sell[51], sell[52], sell[53], sell[54], sell[55], sell[56], sell[57], sell[58], sell[59], sell[60], sell[61], sell[62], sell[63], sell[64], sell[65], sell[66], sell[67], sell[68], sell[69], sell[70], sell[71], sell[72], sell[73], sell[74], sell[75], sell[76], sell[77], sell[78], sell[79], sell[80], sell[81], sell[82], sell[83], sell[84], sell[85], sell[86], sell[87], sell[88], sell[89], sell[90], sell[91], sell[92], sell[93], sell[94], sell[95], 4);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 25 ", opt_sell_result);
		sent_query();
		*/
		//minimum FC cost
		printf("LINE %d: Update cost_name : %s\n\t", __LINE__, "FC_price");
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM cost WHERE (cost_name = '%s')", column, "FC_price");
		fetch_row_value();
		for (j = 0; j < sample_time; j++)
		{
			FC_cost[j] = turn_float(j);
			opt_FC_cost_result += FC_cost[j];
		}
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f' WHERE control_id='%d';", FC_cost[0], FC_cost[1], FC_cost[2], FC_cost[3], FC_cost[4], FC_cost[5], FC_cost[6], FC_cost[7], FC_cost[8], FC_cost[9], FC_cost[10], FC_cost[11], FC_cost[12], FC_cost[13], FC_cost[14], FC_cost[15], FC_cost[16], FC_cost[17], FC_cost[18], FC_cost[19], FC_cost[20], FC_cost[21], FC_cost[22], FC_cost[23], FC_cost[24], FC_cost[25], FC_cost[26], FC_cost[27], FC_cost[28], FC_cost[29], FC_cost[30], FC_cost[31], FC_cost[32], FC_cost[33], FC_cost[34], FC_cost[35], FC_cost[36], FC_cost[37], FC_cost[38], FC_cost[39], FC_cost[40], FC_cost[41], FC_cost[42], FC_cost[43], FC_cost[44], FC_cost[45], FC_cost[46], FC_cost[47], FC_cost[48], FC_cost[49], FC_cost[50], FC_cost[51], FC_cost[52], FC_cost[53], FC_cost[54], FC_cost[55], FC_cost[56], FC_cost[57], FC_cost[58], FC_cost[59], FC_cost[60], FC_cost[61], FC_cost[62], FC_cost[63], FC_cost[64], FC_cost[65], FC_cost[66], FC_cost[67], FC_cost[68], FC_cost[69], FC_cost[70], FC_cost[71], FC_cost[72], FC_cost[73], FC_cost[74], FC_cost[75], FC_cost[76], FC_cost[77], FC_cost[78], FC_cost[79], FC_cost[80], FC_cost[81], FC_cost[82], FC_cost[83], FC_cost[84], FC_cost[85], FC_cost[86], FC_cost[87], FC_cost[88], FC_cost[89], FC_cost[90], FC_cost[91], FC_cost[92], FC_cost[93], FC_cost[94], FC_cost[95], 5);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 26 ", opt_FC_cost_result);
		sent_query();

		//hydrogen comsumption
		printf("LINE %d: Update cost_name : %s\n\t", __LINE__, "hydrogen_consumption");
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM cost WHERE (cost_name = '%s')", column, "hydrogen_consumption");
		fetch_row_value();
		for (j = 0; j < sample_time; j++)
		{
			Hydrogen_com[j] = turn_float(j);
			opt_Hydrogen_result += Hydrogen_com[j];
		}
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f' WHERE control_id='%d';", Hydrogen_com[0], Hydrogen_com[1], Hydrogen_com[2], Hydrogen_com[3], Hydrogen_com[4], Hydrogen_com[5], Hydrogen_com[6], Hydrogen_com[7], Hydrogen_com[8], Hydrogen_com[9], Hydrogen_com[10], Hydrogen_com[11], Hydrogen_com[12], Hydrogen_com[13], Hydrogen_com[14], Hydrogen_com[15], Hydrogen_com[16], Hydrogen_com[17], Hydrogen_com[18], Hydrogen_com[19], Hydrogen_com[20], Hydrogen_com[21], Hydrogen_com[22], Hydrogen_com[23], Hydrogen_com[24], Hydrogen_com[25], Hydrogen_com[26], Hydrogen_com[27], Hydrogen_com[28], Hydrogen_com[29], Hydrogen_com[30], Hydrogen_com[31], Hydrogen_com[32], Hydrogen_com[33], Hydrogen_com[34], Hydrogen_com[35], Hydrogen_com[36], Hydrogen_com[37], Hydrogen_com[38], Hydrogen_com[39], Hydrogen_com[40], Hydrogen_com[41], Hydrogen_com[42], Hydrogen_com[43], Hydrogen_com[44], Hydrogen_com[45], Hydrogen_com[46], Hydrogen_com[47], Hydrogen_com[48], Hydrogen_com[49], Hydrogen_com[50], Hydrogen_com[51], Hydrogen_com[52], Hydrogen_com[53], Hydrogen_com[54], Hydrogen_com[55], Hydrogen_com[56], Hydrogen_com[57], Hydrogen_com[58], Hydrogen_com[59], Hydrogen_com[60], Hydrogen_com[61], Hydrogen_com[62], Hydrogen_com[63], Hydrogen_com[64], Hydrogen_com[65], Hydrogen_com[66], Hydrogen_com[67], Hydrogen_com[68], Hydrogen_com[69], Hydrogen_com[70], Hydrogen_com[71], Hydrogen_com[72], Hydrogen_com[73], Hydrogen_com[74], Hydrogen_com[75], Hydrogen_com[76], Hydrogen_com[77], Hydrogen_com[78], Hydrogen_com[79], Hydrogen_com[80], Hydrogen_com[81], Hydrogen_com[82], Hydrogen_com[83], Hydrogen_com[84], Hydrogen_com[85], Hydrogen_com[86], Hydrogen_com[87], Hydrogen_com[88], Hydrogen_com[89], Hydrogen_com[90], Hydrogen_com[91], Hydrogen_com[92], Hydrogen_com[93], Hydrogen_com[94], Hydrogen_com[95], 6);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 27 ", opt_Hydrogen_result);
		sent_query();

		//FC cost every kW
		printf("LINE %d: Update cost_name : %s\n\t", __LINE__, "FC_price_every_kW");
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM cost WHERE (cost_name = '%s')", column, "FC_price_every_kW");
		fetch_row_value();
		for (j = 0; j < sample_time; j++)
		{
			FC_every_cost[j] = turn_float(j);
		}
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f' WHERE control_id='%d';", FC_every_cost[0], FC_every_cost[1], FC_every_cost[2], FC_every_cost[3], FC_every_cost[4], FC_every_cost[5], FC_every_cost[6], FC_every_cost[7], FC_every_cost[8], FC_every_cost[9], FC_every_cost[10], FC_every_cost[11], FC_every_cost[12], FC_every_cost[13], FC_every_cost[14], FC_every_cost[15], FC_every_cost[16], FC_every_cost[17], FC_every_cost[18], FC_every_cost[19], FC_every_cost[20], FC_every_cost[21], FC_every_cost[22], FC_every_cost[23], FC_every_cost[24], FC_every_cost[25], FC_every_cost[26], FC_every_cost[27], FC_every_cost[28], FC_every_cost[29], FC_every_cost[30], FC_every_cost[31], FC_every_cost[32], FC_every_cost[33], FC_every_cost[34], FC_every_cost[35], FC_every_cost[36], FC_every_cost[37], FC_every_cost[38], FC_every_cost[39], FC_every_cost[40], FC_every_cost[41], FC_every_cost[42], FC_every_cost[43], FC_every_cost[44], FC_every_cost[45], FC_every_cost[46], FC_every_cost[47], FC_every_cost[48], FC_every_cost[49], FC_every_cost[50], FC_every_cost[51], FC_every_cost[52], FC_every_cost[53], FC_every_cost[54], FC_every_cost[55], FC_every_cost[56], FC_every_cost[57], FC_every_cost[58], FC_every_cost[59], FC_every_cost[60], FC_every_cost[61], FC_every_cost[62], FC_every_cost[63], FC_every_cost[64], FC_every_cost[65], FC_every_cost[66], FC_every_cost[67], FC_every_cost[68], FC_every_cost[69], FC_every_cost[70], FC_every_cost[71], FC_every_cost[72], FC_every_cost[73], FC_every_cost[74], FC_every_cost[75], FC_every_cost[76], FC_every_cost[77], FC_every_cost[78], FC_every_cost[79], FC_every_cost[80], FC_every_cost[81], FC_every_cost[82], FC_every_cost[83], FC_every_cost[84], FC_every_cost[85], FC_every_cost[86], FC_every_cost[87], FC_every_cost[88], FC_every_cost[89], FC_every_cost[90], FC_every_cost[91], FC_every_cost[92], FC_every_cost[93], FC_every_cost[94], FC_every_cost[95], 7);
		sent_query();
	}

	//NOW taipower cost reference --> https://www.taipower.com.tw/upload/238/2018070210412196443.pdf
	//float tmp_power[6] = { 0.0 };
	printf("LINE %d: Calculate price_sum_now_power \n\t", __LINE__);
	// if (now_power_result <= (120.0 / 30.0))
	// {
	// 	price_sum_now_power = now_power_result * P_1;
	// }
	// else if ((now_power_result > (120.0 / 30.0)) & (now_power_result <= 330.0 / 30.0))
	// {
	// 	price_sum_now_power = (120.0 * P_1 + (now_power_result * 30.0 - 120.0) * P_2) / 30.0;
	// }
	// else if ((now_power_result > (330.0 / 30.0)) & (now_power_result <= 500.0 / 30.0))
	// {
	// 	price_sum_now_power = (120.0 * P_1 + (330.0 - 120.0) * P_2 + (now_power_result * 30.0 - 330.0) * P_3) / 30.0;
	// }
	// else if ((now_power_result > (500.0 / 30.0)) & (now_power_result <= 700.0 / 30.0))
	// {
	// 	price_sum_now_power = (120.0 * P_1 + (330.0 - 120.0) * P_2 + (500.0 - 330.0) * P_3 + (now_power_result * 30.0 - 500.0) * P_4) / 30.0;
	// }
	// else if ((now_power_result > (700.0 / 30.0)) & (now_power_result <= 1000.0 / 30.0))
	// {
	// 	price_sum_now_power = (120.0 * P_1 + (330.0 - 120.0) * P_2 + (500.0 - 330.0) * P_3 + (700.0 - 500.0) * P_4 + (now_power_result * 30.0 - 700.0) * P_5) / 30.0;
	// }
	// else if (now_power_result > (1000.0 / 30.0))
	// {
	// 	price_sum_now_power = (120.0 * P_1 + (330.0 - 120.0) * P_2 + (500.0 - 330.0) * P_3 + (700.0 - 500.0) * P_4 + (1000.0 - 700.0) * P_5 + (now_power_result * 30.0 - 1000.0) * P_6) / 30.0;
	// }
	snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_id = 22 ", price_sum_now_power);
	sent_query();

	printf("LINE %d: total loads power cost(kW): %f\n\t", __LINE__, now_power_result);
	printf("LINE %d: total loads power cost by taipower(NTD): %f\n\t", __LINE__, price_sum_now_power);
	printf("LINE %d: total loads power cost by three level electric price(NTD): %f\n\t", __LINE__, var_grid_result);
	printf("LINE %d: buy total grid(NTD): %f\n\t", __LINE__, opt_cost_result);
	// printf("LINE %d: Grid_sell:%f\n", __LINE__, opt_sell_result);
	printf("LINE %d: FC_cost(NTD): %f\n\t", __LINE__, opt_FC_cost_result);
	printf("LINE %d: hydrogen comsumotion(g): %f\n\t", __LINE__, opt_Hydrogen_result);
	// step1_bill = opt_cost_result - opt_sell_result;
	// step1_sell = opt_sell_result;
}

void countUninterruptAndVaryingLoads_Flag(int *uninterrupt_flag, int *varying_flag, int household_id)
{
	printf("\nFunction: %s\n\t", __func__);
	int flag = 0;
	if (sample_time != 0)
	{
		for (i = 0; i < uninterrupt_num; i++)
		{
			flag = 0;
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM LHEMS_control_status WHERE equip_name = '%s' and household_id = %d", column, ("uninterDelta" + to_string(i + 1)).c_str(), household_id);
			fetch_row_value();
			for (int j = 0; j < sample_time; j++)
			{
				flag += turn_int(j);
			}
			uninterrupt_flag[i] = flag;
		}
		for (i = 0; i < varying_num; i++)
		{
			flag = 0;
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM LHEMS_control_status WHERE equip_name = '%s' and household_id = %d", column, ("varyingDelta" + to_string(i + 1)).c_str(), household_id);
			fetch_row_value();
			for (int j = 0; j < sample_time; j++)
			{
				flag += turn_int(j);
			}
			varying_flag[i] = flag;
		}
	}
	for (int i = 0; i < uninterrupt_num; i++)
		printf("LINE %d: uninterrupt_flag[%d] : %d\n\t", __LINE__, i, uninterrupt_flag[i]);
	for (int i = 0; i < varying_num; i++)
		printf("LINE %d: varying_flag[%d] : %d\n", __LINE__, i, varying_flag[i]);
}

void countLoads_AlreadyOpenedTimes(int *buff, int household_id)
{
	printf("\nFunction: %s\n\t", __func__);
	int coun = 0;
	if (sample_time != 0)
	{
		for (i = 0; i < app_count; i++)
		{
			coun = 0;

			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM LHEMS_control_status WHERE equip_name = '%s' and household_id = %d", column, variable_name[i].c_str(), household_id);
			fetch_row_value();
			for (int j = 0; j < sample_time; j++)
			{
				coun += turn_int(j);
			}
			buff[i] = coun;
		}
	}
	printf("LINE %d: ", __LINE__);
	for (int i = 0; i < app_count; i++)
		printf("buff[%d]: %d ", i, buff[i]);
	printf("\n");
}

void count_interruptLoads_RemainOperateTime(int interrupt_num, int *interrupt_ot, int *interrupt_reot, int *buff)
{
	printf("\nFunction: %s\n\t", __func__);
	for (i = 0; i < interrupt_num; i++) //(Interrupt load)
	{
		if ((interrupt_ot[i] - buff[i]) == interrupt_ot[i])
		{
			interrupt_reot[i] = interrupt_ot[i];
		}
		else if (((interrupt_ot[i] - buff[i]) < interrupt_ot[i]) && ((interrupt_ot[i] - buff[i]) > 0))
		{
			interrupt_reot[i] = interrupt_ot[i] - buff[i];
		}
		else if ((interrupt_ot[i] - buff[i]) <= 0)
		{
			interrupt_reot[i] = 0;
		}
		printf("LINE %d: load %d : reot = %d\n\t", __LINE__, i, interrupt_reot[i]);
	}
}

void count_uninterruptAndVaryingLoads_RemainOperateTime(int group_id, int loads_total, int *total_operateTime, int *remain_operateTime, int *end_time, int *flag, int *buff)
{
	switch (group_id)
	{
	case 2:
		printf("\nFunction: %s group id : %d\n\t", __func__, group_id);
		for (i = 0; i < uninterrupt_num; i++)
		{
			if (flag[i] == 0) //(Uninterrupted load has not yet started)
			{
				remain_operateTime[i] = total_operateTime[i];
			}
			if (flag[i] == 1) //(Uninterrupted load is started (modify load start time))
			{
				if (((total_operateTime[i] - buff[i + interrupt_num]) < total_operateTime[i]) && ((total_operateTime[i] - buff[i + interrupt_num]) > 0))
				{
					remain_operateTime[i] = total_operateTime[i] - buff[i + interrupt_num];
					if (remain_operateTime[i] != 0)
					{
						end_time[i] = sample_time + remain_operateTime[i] - 1;
					}
				}
				else if ((total_operateTime[i] - buff[i + interrupt_num]) <= 0)
				{
					remain_operateTime[i] = 0;
				}
			}
		}
		break;
	case 3:
		printf("\nFunction: %s group id : %d\n\t", __func__, group_id);
		for (i = 0; i < varying_num; i++)
		{
			if (flag[i] == 0) //(Varying load has not yet started)
			{
				remain_operateTime[i] = total_operateTime[i];
			}
			if (flag[i] == 1) //(Varying load is started (modify load start time))
			{
				if (((total_operateTime[i] - buff[i + interrupt_num + uninterrupt_num]) < total_operateTime[i]) && ((total_operateTime[i] - buff[i + interrupt_num + uninterrupt_num]) > 0))
				{
					remain_operateTime[i] = total_operateTime[i] - buff[i + interrupt_num + uninterrupt_num];
					if (remain_operateTime[i] != 0)
					{
						end_time[i] = sample_time + remain_operateTime[i] - 1;
					}
				}
				else if ((total_operateTime[i] - buff[i + interrupt_num + uninterrupt_num]) <= 0)
				{
					remain_operateTime[i] = 0;
				}
			}
		}
		break;
	default:
		printf("\nFunction: %s no matching group id : %d\n\t", __func__, group_id);
		break;
	}
}

void init_VaryingLoads_OperateTimeAndPower(int **varying_t_d, float **varying_p_d, int *varying_ot)
{
	printf("\nFunction: %s \n\t", __func__);
	for (i = 0; i < varying_num; i++)
	{
		for (m = 0; m < (time_block - sample_time); m++)
		{
			varying_t_d[i][m] = 0;
		}
		for (m = 0; m < varying_ot[i]; m++)
		{
			varying_p_d[i][m] = 0.0;
		}
	}
}

void putValues_VaryingLoads_OperateTimeAndPower(int **varying_t_d, float **varying_p_d, int **varying_t_pow, float **varying_p_pow, int *varying_start, int *varying_end, float *varying_p_max)
{
	printf("\nFunction: %s \n\t", __func__);
	/*========================(Energy consumption model of varying load)==============================*/
	for (i = 0; i < varying_num; i++)
	{
		for (j = 0; j < varying_t_pow[i][0]; j++)
		{
			varying_p_d[i][j] = varying_p_pow[i][0];
		}
		for (j = varying_t_pow[i][0]; j < varying_t_pow[i][0] + varying_t_pow[i][1]; j++)
		{
			varying_p_d[i][j] = varying_p_pow[i][1];
		}
		for (j = varying_t_pow[i][0] + varying_t_pow[i][1]; j < varying_t_pow[i][0] + varying_t_pow[i][1] + varying_t_pow[i][2]; j++)
		{
			varying_p_d[i][j] = varying_p_pow[i][2];
		}
	}

	/*========================(Start and end period variable of varying load) =========================== */
	for (i = 0; i < varying_num; i++)
	{
		if ((varying_end[i] - sample_time) >= 0)
		{
			if ((varying_start[i] - sample_time) >= 0)
			{
				for (m = (varying_start[i] - sample_time); m <= (varying_end[i] - sample_time); m++)
				{
					varying_t_d[i][m] = 1;
				}
			}
			else if ((varying_start[i] - sample_time) < 0)
			{
				for (m = 0; m <= (varying_end[i] - sample_time); m++)
				{
					varying_t_d[i][m] = 1;
				}
			}
		}
	}

	/*========================(maximum power of varying load) =========================== */
	for (i = 0; i < varying_num; i++)
	{
		varying_p_max[i] = 0.0;

		for (j = 0; j < 3; j++)
		{
			if (varying_p_pow[i][j] > varying_p_max[i])
			{
				varying_p_max[i] = varying_p_pow[i][j];
			}
		}
	}
	// printf("LINE %d: Varying loads power model : ", __LINE__);
	// for (i = 0; i < varying_num; i++)
	// {
	// 	for (j = 0; j < varying_t_pow[i][0] + varying_t_pow[i][1] + varying_t_pow[i][2]; j++)
	// 		printf("%.2f ", varying_p_d[i][j]);
	// 	printf("\n\tLINE %d: Varying loads Max power = %.2f\n", __LINE__, varying_p_max[i]);
	// }
}

void update_loadModel(float *interrupt_p, float *uninterrupt_p, int household_id)
{
	functionPrint(__func__);
	float *power_tmp = new float[time_block - sample_time];
	for (int i = 0; i < time_block - sample_time; i++)
		power_tmp[i] = 0.0;

	for (int i = 0; i < interrupt_num; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM LHEMS_control_status WHERE equip_name = '%s' and household_id = %d", column, ("interrupt" + to_string(i + 1)).c_str(), household_id);
		fetch_row_value();
		for (int j = sample_time; j < time_block; j++)
		{
			power_tmp[j - sample_time] += turn_float(j) * interrupt_p[i];
		}
	}
	for (int i = 0; i < uninterrupt_num; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM LHEMS_control_status WHERE equip_name = '%s' and household_id = %d", column, ("uninterrupt" + to_string(i + 1)).c_str(), household_id);
		fetch_row_value();
		for (int j = sample_time; j < time_block; j++)
		{
			power_tmp[j - sample_time] += turn_float(j) * uninterrupt_p[i];
		}
	}
	for (int i = 0; i < varying_num; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM LHEMS_control_status WHERE equip_name = '%s' and household_id = %d", column, ("varyingPsi" + to_string(i + 1)).c_str(), household_id);
		fetch_row_value();
		for (int j = sample_time; j < time_block; j++)
		{
			power_tmp[j - sample_time] += turn_float(j);
		}
	}
	for (int i = sample_time; i < time_block; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `totalLoad_model` SET `household%d` = '%.3f' WHERE `totalLoad_model`.`time_block` = %d;", household_id, power_tmp[i - sample_time], i);
		sent_query();
	}
	// =-=-=-=-=-=-=- Caculate for total load model -=-=-=-=-=-=-= //
	if (household_id == householdTotal)
	{
		for (int i = 1; i < householdTotal; i++)
		{
			for (int j = sample_time; j < time_block; j++)
			{
				snprintf(sql_buffer, sizeof(sql_buffer), "SELECT household%d FROM totalLoad_model WHERE time_block = %d", i, j);
				power_tmp[j - sample_time] += turn_value_to_float(0);
			}
		}
		for (int i = sample_time; i < time_block; i++)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `totalLoad_model` SET `totalLoad` = '%.3f', `time` = CURRENT_TIMESTAMP WHERE `totalLoad_model`.`time_block` = %d;", power_tmp[i - sample_time], i);
			sent_query();
		}
	}
}

float *rand_operationTime()
{
	functionPrint(__func__);
	float *result = new float[time_block];
	for (int i = 0; i < time_block; i++)
		result[i] = 0.0;

	int uncontrollable_load_flag = value_receive("BaseParameter", "parameter_name", "uncontrollable_load_flag");
	if (uncontrollable_load_flag == 0)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `LHEMS_uncontrollable_load` SET `household%d` = '0.0' ", household_id);
		sent_query();
		if (household_id == householdTotal)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `LHEMS_uncontrollable_load` SET `totalLoad` = '0.0' ");
			sent_query();
		}
		return result;
	}

	srand(time(NULL));
	if (sample_time == 0)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT COUNT(*) FROM `load_list` WHERE group_id = 4");
		int uncontrollableLoad_num = turn_value_to_int(0);
		for (int i = 0; i < uncontrollableLoad_num; i++)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT uncontrollable_loads, power1 FROM load_list WHERE group_id = 4 LIMIT %d, %d", i, i + 1);
			fetch_row_value();
			char *seo_time = mysql_row[0];
			float power = atof(mysql_row[1]);
			char *tmp;
			tmp = strtok(seo_time, "~");
			vector<int> time_seperate;
			while (tmp != NULL)
			{
				time_seperate.push_back(atoi(tmp));
				tmp = strtok(NULL, "~");
			}

			int operate_count = 0;
			for (int i = time_seperate[0]; i < time_seperate[1] - 1; i++)
			{
				if (operate_count != time_seperate[2])
				{
					int operate_tmp = rand() % 2;
					float operate_power = operate_tmp * power;
					operate_count += operate_tmp;
					result[i] += operate_power;
				}
			}
			time_seperate.clear();
		}
		for (int i = 0; i < time_block; i++)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `LHEMS_uncontrollable_load` SET `household%d` = '%.1f' WHERE `time_block` = %d;", household_id, result[i], i);
			sent_query();
		}
		if (household_id == householdTotal)
		{
			for (int j = 0; j < time_block; j++)
			{
				float power_total = 0.0;
				for (int i = 1; i <= householdTotal; i++)
				{
					snprintf(sql_buffer, sizeof(sql_buffer), "SELECT household%d FROM `LHEMS_uncontrollable_load` WHERE time_block = %d", i, j);
					power_total += turn_value_to_float(0);
				}
				snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `LHEMS_uncontrollable_load` SET `totalLoad` = '%.1f' WHERE `time_block` = %d;", power_total, j);
				sent_query();
			}
		}
	}
	else
	{
		for (int i = 0; i < time_block; i++)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT `household%d` FROM `LHEMS_uncontrollable_load` WHERE `time_block` = %d;", household_id, i);
			result[i] = turn_value_to_float(0);
		}
	}

	return result;
}

float *household_weighting()
{
	functionPrint(__func__);

	float *result = new float[dr_endTime - dr_startTime];
	string sql_table;
	if (sample_time != 0)
	{
		sql_table = "LHEMS_control_status";
	}
	else
	{
		sql_table = "LHEMS_histroy_control_status";
		if (household_id == 1)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "TRUNCATE TABLE `demand_response_alpha`");
			sent_query();
			snprintf(sql_buffer, sizeof(sql_buffer), "FLUSH TABLE `demand_response_alpha`");
			sent_query();
		}
	}

	for (int i = dr_startTime; i < dr_endTime; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT SUM(A%d) FROM `%s` WHERE `equip_name` = 'Pgrid'", i, sql_table.c_str());
		float total_load = turn_value_to_float(0);
		// total load = 0 while all households' Pgrid = 0, when result[] = 0/0 will be nan
		if (total_load != 0)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT SUM(A%d) FROM `%s` WHERE `equip_name` = 'Pgrid' && `household_id` = %d", i, sql_table.c_str(), household_id);
			float each_household_load = turn_value_to_float(0);
			result[i - dr_startTime] = each_household_load / total_load;
		}
		else
		{
			result[i - dr_startTime] = 0.0;
		}

		printf("\thousehold %d timeblock %d weighting %.3f\n", household_id, i, result[i - dr_startTime]);
	}
	if (sample_time != 0)
	{
		for (int i = 0; i < dr_endTime - dr_startTime; i++)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `demand_response_alpha` SET A%d = %.3f WHERE household_id = %d", sample_time, result[i], household_id);
			sent_query();
		}
	}
	else
	{
		for (int i = 0; i < dr_endTime - dr_startTime; i++)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO demand_response_alpha (A0, dr_timeblock, household_id) VALUES('%.3f', %d, '%d');", result[i], i + dr_startTime, household_id);
			sent_query();
		}
	}

	return result;
}