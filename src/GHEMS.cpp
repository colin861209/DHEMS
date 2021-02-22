#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glpk.h>
#include <math.h>
#include <mysql.h>
#include <iostream>
#include "SQLFunction.hpp"
// use function 'find_variableName_position' needs
#include <string>
#include <vector>
#include <algorithm>
//common variable

#define P_1 1.63
#define P_2 2.38
#define P_3 3.52
#define P_4 4.80
#define P_5 5.66
#define P_6 6.41

#define Hydro_Price 0.08
#define Hydro_Cons 0.04		// unit kWh/g
#define FC_START_POWER 0.35 // Pfc start power
using namespace std;
int h, i, j, k, m, n = 0;
double z = 0;
vector<string> variable_name;
// common parameter
int time_block = 0, variable = 0, divide = 0, sample_time = 0, point_num = 0, piecewise_num;

float delta_T = 0.0;
float Cbat = 0.0, Vsys = 0.0, SOC_ini = 0.0, SOC_min = 0.0, SOC_max = 0.0, SOC_thres = 0.0, Pbat_min = 0.0, Pbat_max = 0.0, Pgrid_max = 0.0, Psell_max = 0.0, Delta_battery = 0.0, Pfc_max = 0.0;

float step1_bill = 0.0, step1_sell = 0.0, step1_PESS = 0.0;

char column[400] = "A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,A15,A16,A17,A18,A19,A20,A21,A22,A23,A24,A25,A26,A27,A28,A29,A30,A31,A32,A33,A34,A35,A36,A37,A38,A39,A40,A41,A42,A43,A44,A45,A46,A47,A48,A49,A50,A51,A52,A53,A54,A55,A56,A57,A58,A59,A60,A61,A62,A63,A64,A65,A66,A67,A68,A69,A70,A71,A72,A73,A74,A75,A76,A77,A78,A79,A80,A81,A82,A83,A84,A85,A86,A87,A88,A89,A90,A91,A92,A93,A94,A95";
int determine_realTimeOrOneDayMode_andGetSOC(int same_day, int real_time, vector<string> variable_name);
void updateBaseParameter_from_1To(int length, vector<float>);
void print_BaseParameter_SystemState(int real_time, int same_day);
void getOrUpdate_SolarInfo_ThroughSampleTime(const char *weather, float *solar2);
void updateTableCost(float *totalLoad, float *totalLoad_price, float *real_grid_pirce, float *fuelCell_kW_price, float *Hydrogen_g_consumption, float *real_sell_price, float totalLoad_sum, float totalLoad_priceSum, float real_grid_pirceSum, float fuelCell_kW_priceSum, float Hydrogen_g_consumptionSum, float real_sell_priceSum, float totalLoad_taipowerPriceSum);
void optimization(vector<string> variable_name, float *load_model, float *price2);
void calculateCostInfo(float *price2);

int main(int argc, const char** argv)
{
	time_t t = time(NULL);
	struct tm now_time = *localtime(&t);
	int same_day = 0, real_time = 0;

	int *position = new int[16];

	if ((mysql_real_connect(mysql_con, "140.124.42.70", "root", "fuzzy314", "DHEMS", 6666, NULL, 0)) == NULL)
	{
		printf("Failed to connect to Mysql!\n");
		system("pause");
		return 0;
	}
	else
	{
		messagePrint(__LINE__, "Connect to Mysql sucess!!");
		mysql_set_character_set(mysql_con, "utf8");
	}

	// =-=-=-=-=-=-=- get parameter values from BaseParameter in need -=-=-=-=-=-=-= //
	vector<float> parameter_tmp;
	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value from BaseParameter where parameter_name = 'time_block' ");
	parameter_tmp.push_back(turn_value_to_float(0));
	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value from BaseParameter where parameter_name = 'householdAmount' ");
	parameter_tmp.push_back(turn_value_to_float(0));
	for (i = 8; i <= 17; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value from BaseParameter where parameter_id = '%d'", i);
		parameter_tmp.push_back(turn_value_to_float(0));
	}
	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value from BaseParameter where parameter_name = 'Global_real_time' ");
	parameter_tmp.push_back(turn_value_to_float(0));

	// =-=-=-=-=-=-=- we suppose that enerage appliance in community should same as the single appliance times household amount -=-=-=-=-=-=-= //
	time_block = parameter_tmp[0];
	Vsys = parameter_tmp[2] * parameter_tmp[1];
	Cbat = parameter_tmp[3];
	SOC_min = parameter_tmp[4];
	SOC_max = parameter_tmp[5];
	SOC_thres = parameter_tmp[6];
	Pbat_min = parameter_tmp[7] * parameter_tmp[1];
	Pbat_max = parameter_tmp[8] * parameter_tmp[1];
	Pgrid_max = parameter_tmp[9] * parameter_tmp[1];
	Psell_max = parameter_tmp[10] * parameter_tmp[1];
	Pfc_max = parameter_tmp[11];
	real_time = (int)parameter_tmp[12];

	divide = (time_block / 24);
	delta_T = 1.0 / (float)divide;
	point_num = 6;
	piecewise_num = point_num - 1;

	// Most important thing
	variable_name.push_back("Pgrid");
	variable_name.push_back("mu_grid");
	variable_name.push_back("Psell");
	variable_name.push_back("Pess");
	variable_name.push_back("Pcharge");
	variable_name.push_back("Pdischarge");
	variable_name.push_back("SOC");
	variable_name.push_back("Z");
	variable_name.push_back("Pfc");
	variable_name.push_back("Pfct");
	variable_name.push_back("PfcON");
	variable_name.push_back("PfcOFF");
	variable_name.push_back("muFC");
	for (int i = 0; i < piecewise_num; i++)
		variable_name.push_back("zPfc" + to_string(i + 1));
	for (int i = 0; i < piecewise_num; i++)
		variable_name.push_back("lambda_Pfc" + to_string(i + 1));
	variable = variable_name.size();

	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value from BaseParameter where parameter_name = 'Global_next_simulate_timeblock'");
	sample_time = turn_value_to_int(0);

	// updateBaseParameter_from_1To(13, parameter_tmp);
	// print_BaseParameter_SystemState(real_time, same_day);

	// =-=-=-=-=-=-=- get electric price data -=-=-=-=-=-=-= //
	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT COUNT(*) FROM price");
	int priceAmount = turn_value_to_int(0);
	float *price = new float[priceAmount];
	float *price2 = new float[time_block];
	for (i = 1; i < 25; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT price_value FROM price WHERE price_period = %d", i - 1);
		price[i - 1] = turn_value_to_float(0);
	}
	for (int x = 0; x < 24; x++)
	{
		for (int y = x * divide; y < (x * divide) + divide; y++)
		{
			price2[y] = price[x];
		}
	}

	// =-=-=-=-=-=-=- get households' loads consumption from table 'totalLoad_model' -=-=-=-=-=-=-= //
	float *load_model = new float[time_block];
	for (int i = 0; i < time_block; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT totalLoad FROM totalLoad_model WHERE time_block = %d", i);
		load_model[i] = turn_value_to_float(0);
	}

	// =-=-=-=-=-=-=- return 1 after determine mode and get SOC -=-=-=-=-=-=-= //
	if ((sample_time + 1) != 97)
		same_day = 1;
	else
		same_day = 0;
	real_time = determine_realTimeOrOneDayMode_andGetSOC(same_day, real_time, variable_name);

	snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value FROM BaseParameter WHERE parameter_name = 'Global_next_simulate_timeblock' ");
	sample_time = turn_value_to_float(0);
	messagePrint(__LINE__, "sample time from database = ", 'I', sample_time);

	snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = '%d-%02d-%02d' WHERE parameter_name = 'lastTime_execute' ", now_time.tm_year + 1900, now_time.tm_mon + 1, now_time.tm_mday);
	sent_query();

	optimization(variable_name, load_model, price2);
	calculateCostInfo(price2);

	printf("LINE %d: sample_time = %d\n", __LINE__, sample_time);
	printf("LINE %d: next sample_time = %d\n\n", __LINE__, sample_time + 1);

	snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = '%d' WHERE  parameter_name = 'Global_next_simulate_timeblock' ", sample_time + 1);
	sent_query();

	mysql_close(mysql_con);
	return 0;
}

void optimization(vector<string> variable_name, float *load_model, float *price2)
{
	functionPrint(__func__);
	time_t t = time(NULL);
	struct tm now_time = *localtime(&t);

	int noo;
	if (((now_time.tm_min) % (60 / divide)) != 0)
		noo = (now_time.tm_hour) * divide + (int)((now_time.tm_min) / (60 / divide)); //divide=4
	else
		noo = (now_time.tm_hour) * divide + (int)((now_time.tm_min) / (60 / divide));

	messagePrint(__LINE__, "Now time block : ", 'I', noo, 'Y');

	piecewise_num;
	float *data_power = new float[101];		// power
	float *data_power_all = new float[101]; // total power

	float *P_power = new float[point_num];	   // power
	float *P_power_all = new float[point_num]; // total power

	for (i = 1; i <= 101; i++)
	{
		if (i == 1)
			data_power[i - 1] = FC_START_POWER;

		else
			data_power[i - 1] = (i - 1) * (Pfc_max - FC_START_POWER) * 0.01 + FC_START_POWER;

		data_power_all[i - 1] = data_power[i - 1] / (0.00025732 * (pow(data_power[i - 1], 5)) - 0.003463 * (pow(data_power[i - 1], 4)) + 0.016041 * (pow(data_power[i - 1], 3)) - 0.02706 * (pow(data_power[i - 1], 2)) - 0.026982 * (pow(data_power[i - 1], 1)) + 0.5692);
	}

	P_power[0] = 0.0;
	P_power_all[0] = 0.0;
	// messagePrint(__LINE__, "x_ is Pfc coeficient, y_ is Pfct coeficient", 'S', 0, 'Y');
	// printf("\tLINE %d: x_0:%f  y_0:%f\n", __LINE__, P_power[0], P_power_all[0]);
	for (j = 1; j < point_num; j++)
	{
		P_power[j] = data_power[(j - 1) * (100 / (piecewise_num - 1))];
		P_power_all[j] = data_power_all[(j - 1) * (100 / (piecewise_num - 1))];
		// printf("\tLINE %d: x_%d:%f  y_%d:%f\n", __LINE__, j, P_power[j], j, P_power_all[j]);
	}

	// =-=-=-=-=-=-=- choose column 'big_sunny' 'sunny' 'cloudy' in table solar_data -=-=-=-=-=-=-= //
	float *solar2 = new float[time_block];
	getOrUpdate_SolarInfo_ThroughSampleTime("cloudy", solar2);

	printf("\n------ Starting GLPK Part ------\n");

	int rowTotal = (time_block - sample_time) * 200 + 1;
	int colTotal = variable * (time_block - sample_time);

	glp_prob *mip;
	mip = glp_create_prob();
	glp_set_prob_name(mip, "GHEMS");
	glp_set_obj_dir(mip, GLP_MIN);
	glp_add_rows(mip, rowTotal);
	glp_add_cols(mip, colTotal);

	for (i = 0; i < (time_block - sample_time); i++)
	{
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pgrid") + 1 + i * variable), GLP_DB, 0.0, Pgrid_max); //Pgrid
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pgrid") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "mu_grid") + 1 + i * variable), GLP_DB, 0.0, 1.0);
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "mu_grid") + 1 + i * variable), GLP_BV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Psell") + 1 + i * variable), GLP_DB, -0.00001, Psell_max);
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "Psell") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pess") + 1 + i * variable), GLP_DB, -Pbat_min, Pbat_max); // Pess
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pess") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pcharge") + 1 + i * variable), GLP_FR, 0.0, Pbat_max); // Pess +
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pcharge") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pdischarge") + 1 + i * variable), GLP_FR, 0.0, Pbat_min); // Pess -
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pdischarge") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "SOC") + 1 + i * variable), GLP_DB, SOC_min, SOC_max); //SOC
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "SOC") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Z") + 1 + i * variable), GLP_DB, 0.0, 1.0); //Z
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "Z") + 1 + i * variable), GLP_BV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pfc") + 1 + i * variable), GLP_DB, -0.00001, Pfc_max); //Pfc
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pfc") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "Pfct") + 1 + i * variable), GLP_LO, 0.0, 0.0); //Total_Pfc
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "Pfct") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "PfcON") + 1 + i * variable), GLP_DB, -0.00001, Pfc_max); //Pfc_ON_POWER
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "PfcON") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "PfcOFF") + 1 + i * variable), GLP_FX, 0.0, 0.0); //Pfc_OFF_POWER
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "PfcOFF") + 1 + i * variable), GLP_CV);
		glp_set_col_bnds(mip, (find_variableName_position(variable_name, "muFC") + 1 + i * variable), GLP_DB, 0.0, 1.0); //ufc
		glp_set_col_kind(mip, (find_variableName_position(variable_name, "muFC") + 1 + i * variable), GLP_BV);

		for (j = 1; j <= piecewise_num; j++)
		{
			glp_set_col_bnds(mip, (find_variableName_position(variable_name, "zPfc" + to_string(j)) + 1 + i * variable), GLP_DB, 0.0, 1.0); //z_Pfc
			glp_set_col_kind(mip, (find_variableName_position(variable_name, "zPfc" + to_string(j)) + 1 + i * variable), GLP_BV);
		}

		for (j = 1; j <= piecewise_num; j++)
		{
			glp_set_col_bnds(mip, (find_variableName_position(variable_name, "lambda_Pfc" + to_string(j)) + 1 + i * variable), GLP_LO, 0.0, 0.0); //λ_Pfc
			glp_set_col_kind(mip, (find_variableName_position(variable_name, "lambda_Pfc" + to_string(j)) + 1 + i * variable), GLP_CV);
		}
	}

	float **coefficient = NEW2D(rowTotal, colTotal, float);
	for (m = 0; m < rowTotal; m++)
	{
		for (n = 0; n < colTotal; n++)
			coefficient[m][n] = 0.0;
	}

	// 0 < Pgrid j < μgrid j * Pgrid max
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[i][i * variable + find_variableName_position(variable_name, "Pgrid")] = 1.0;
		coefficient[i][i * variable + find_variableName_position(variable_name, "mu_grid")] = -Pgrid_max;
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, i, "");
		glp_set_row_bnds(mip, i, GLP_UP, 0.0, 0.0);
	}

	// Psell j < (1 - μgrid j) * Psell max
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[time_block - sample_time][i * variable + find_variableName_position(variable_name, "Psell")] = 1.0;
		coefficient[time_block - sample_time][i * variable + find_variableName_position(variable_name, "mu_grid")] = Psell_max;
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, (time_block - sample_time) + 1 + i, "");
		glp_set_row_bnds(mip, (time_block - sample_time) + 1 + i, GLP_UP, 0.0, Psell_max);
	}

	// Psell j <= Pfc j + Ppv j
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 2][i * variable + find_variableName_position(variable_name, "Psell")] = 1.0;
		coefficient[(time_block - sample_time) * 2][i * variable + find_variableName_position(variable_name, "Pfc")] = -1.0;
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, (time_block - sample_time) * 2 + 1 + i, "");
		glp_set_row_bnds(mip, (time_block - sample_time) * 2 + 1 + i, GLP_UP, 0.0, solar2[i - 1 + sample_time]);
	}

	// SOC j - 1 + sum((Pess * Ts) / (Cess * Vess)) >= SOC threshold, only one constranit formula
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 3][i * variable + find_variableName_position(variable_name, "Pess")] = 1.0;
	}
	glp_set_row_name(mip, ((time_block - sample_time) * 3 + 1), "");
	if (sample_time == 0)
	{
		glp_set_row_bnds(mip, ((time_block - sample_time) * 3 + 1), GLP_LO, ((SOC_thres - SOC_ini) * Cbat * Vsys) / delta_T, 0.0);
	}
	else
	{
		// avoid the row max is bigger than SOC max
		glp_set_row_bnds(mip, ((time_block - sample_time) * 3 + 1), GLP_DB, ((SOC_thres - SOC_ini) * Cbat * Vsys) / delta_T, ((0.89 - SOC_ini) * Cbat * Vsys) / delta_T);
	}

	// next SOC
	// SOC j = SOC j - 1 + (Pess j * Ts) / (Cess * Vess)
	for (i = 0; i < (time_block - sample_time); i++)
	{
		for (j = 0; j <= i; j++)
		{
			coefficient[(time_block - sample_time) * 3 + 1 + i][j * variable + find_variableName_position(variable_name, "Pess")] = -1.0; // Pess
		}
		coefficient[(time_block - sample_time) * 3 + 1 + i][i * variable + find_variableName_position(variable_name, "SOC")] = Cbat * Vsys / delta_T; //SOC
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 3 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 3 + 1 + i), GLP_FX, (SOC_ini * Cbat * Vsys / delta_T), (SOC_ini * Cbat * Vsys / delta_T));
	}

	//(Balanced function) Pgrid j + Pfc j + Ppv j = sum(Pa j) + Pess j + Psell j
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 4 + 1 + i][i * variable + find_variableName_position(variable_name, "Pgrid")] = -1.0;
	}
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 4 + 1 + i][i * variable + find_variableName_position(variable_name, "Pess")] = 1.0; // Pess j
	}
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 4 + 1 + i][i * variable + find_variableName_position(variable_name, "Pfc")] = -1.0; //pfc
	}
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 4 + 1 + i][i * variable + find_variableName_position(variable_name, "Psell")] = -1.0; //pfc
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 4 + 1 + i), "");
		if (solar2[i - 1 + sample_time] - load_model[i - 1 + sample_time] < 0)
			glp_set_row_bnds(mip, ((time_block - sample_time) * 4 + 1 + i), GLP_FX, solar2[i - 1 + sample_time] - load_model[i - 1 + sample_time], solar2[i - 1 + sample_time] - load_model[i - 1 + sample_time]);
		else
			glp_set_row_bnds(mip, ((time_block - sample_time) * 4 + 1 + i), GLP_DB, -0.0001, solar2[i - 1 + sample_time] - load_model[i - 1 + sample_time]);
	}

	//(Charge limit) Pess + <= z * Pcharge max
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 5 + 1 + i][i * variable + find_variableName_position(variable_name, "Pcharge")] = 1.0; //Pess +
		coefficient[(time_block - sample_time) * 5 + 1 + i][i * variable + find_variableName_position(variable_name, "Z")] = -Pbat_max; //Z
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 5 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 5 + 1 + i), GLP_UP, 0.0, 0.0);
	}

	// (Discharge limit) Pess - <= (1 - z) * (-Pdischarge max)
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 6 + 1 + i][i * variable + find_variableName_position(variable_name, "Pdischarge")] = 1.0; //Pess -
		coefficient[(time_block - sample_time) * 6 + 1 + i][i * variable + find_variableName_position(variable_name, "Z")] = Pbat_min;	   //Z
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 6 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 6 + 1 + i), GLP_UP, 0.0, Pbat_min);
	}

	// (Battery power) Pdischarge max <= Pess j <= Pcharge max
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 7 + 1 + i][i * variable + find_variableName_position(variable_name, "Pess")] = 1.0;	   //Pess
		coefficient[(time_block - sample_time) * 7 + 1 + i][i * variable + find_variableName_position(variable_name, "Pcharge")] = -1.0;   //Pess +
		coefficient[(time_block - sample_time) * 7 + 1 + i][i * variable + find_variableName_position(variable_name, "Pdischarge")] = 1.0; //Pess -
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 7 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 7 + 1 + i), GLP_FX, 0.0, 0.0);
	}

	// =-=-=-=-=-=-=- FC part -=-=-=-=-=-=-=
	//fc constraint
	//pfc=pfc_on+pfc_off
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 8 + 1 + i][i * variable + find_variableName_position(variable_name, "Pfc")] = 1.0;		// Pfc
		coefficient[(time_block - sample_time) * 8 + 1 + i][i * variable + find_variableName_position(variable_name, "PfcON")] = -1.0;	// Pfc_on
		coefficient[(time_block - sample_time) * 8 + 1 + i][i * variable + find_variableName_position(variable_name, "PfcOFF")] = -1.0; // Pfc_off
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 8 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 8 + 1 + i), GLP_FX, 0.0, 0.0);
	}

	//pfc_on<=ufc*Pfc_max
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 9 + 1 + i][i * variable + find_variableName_position(variable_name, "PfcON")] = 1.0;	  //Pfc_on    //Pfc_on<=ufc*Pfc_max
		coefficient[(time_block - sample_time) * 9 + 1 + i][i * variable + find_variableName_position(variable_name, "muFC")] = -Pfc_max; // ufc
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 9 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 9 + 1 + i), GLP_UP, 0.0, 0.0);
	}

	//pfc_on>=ufc*pfc_min
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 10 + 1 + i][i * variable + find_variableName_position(variable_name, "PfcON")] = 1.0; //Pfc_on    //Pfc_on>=ufc*Pfc_min
		coefficient[(time_block - sample_time) * 10 + 1 + i][i * variable + find_variableName_position(variable_name, "muFC")] = -1.5; // ufc
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 10 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 10 + 1 + i), GLP_LO, 0.0, 0.0);
	}

	//pfc_off j<=(1-ufc)*pfc off
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 11 + 1 + i][i * variable + find_variableName_position(variable_name, "PfcOFF")] = 1.0; //Pfc_off    //Pfc_off<=ufc*0.000000001
		coefficient[(time_block - sample_time) * 11 + 1 + i][i * variable + find_variableName_position(variable_name, "muFC")] = 0.0;	//ufc
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 11 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 11 + 1 + i), GLP_UP, 0.0, 0.0);
	}

	//pfc=x1z1+(x2-x1)s1......
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 12 + 1 + i][i * variable + find_variableName_position(variable_name, "Pfc")] = 1.0; //Pfc j

		for (k = 1; k <= piecewise_num; k++) //X=z1*x1+(x2-x1)*s1...
		{
			coefficient[(time_block - sample_time) * 12 + 1 + i][i * variable + find_variableName_position(variable_name, "zPfc" + to_string(k))] = -P_power[k - 1];							//z
			coefficient[(time_block - sample_time) * 12 + 1 + i][i * variable + find_variableName_position(variable_name, "lambda_Pfc" + to_string(k))] = -1.0 * (P_power[k] - P_power[k - 1]); //λ
		}
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 12 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 12 + 1 + i), GLP_UP, 0.0, 0.0);
	}

	//pfc_tatol=y1z1+(y2-y1)s1......
	for (i = 0; i < (time_block - sample_time); i++)
	{
		coefficient[(time_block - sample_time) * 13 + 1 + i][i * variable + find_variableName_position(variable_name, "Pfct")] = 1.0; //Pfct j

		for (k = 1; k <= piecewise_num; k++) //Y=z1*y1+(y2-y1)*s1...
		{
			coefficient[(time_block - sample_time) * 13 + 1 + i][i * variable + find_variableName_position(variable_name, "zPfc" + to_string(k))] = -P_power_all[k - 1];								//z
			coefficient[(time_block - sample_time) * 13 + 1 + i][i * variable + find_variableName_position(variable_name, "lambda_Pfc" + to_string(k))] = -1.0 * (P_power_all[k] - P_power_all[k - 1]); //λ
		}
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 13 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 13 + 1 + i), GLP_LO, 0.0, 0.0);
	}

	//z1+z2+z3+.....=1
	for (i = 0; i < (time_block - sample_time); i++)
	{
		for (k = 1; k <= piecewise_num; k++)
		{
			coefficient[(time_block - sample_time) * 14 + 1 + i][i * variable + find_variableName_position(variable_name, "zPfc" + to_string(k))] = 1.0; //z       //z1+z2+z3+......=1
		}
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		glp_set_row_name(mip, ((time_block - sample_time) * 14 + 1 + i), "");
		glp_set_row_bnds(mip, ((time_block - sample_time) * 14 + 1 + i), GLP_FX, 1.0, 1.0);
	}

	// 0 <= λi j <= zi j
	for (i = 0; i < (time_block - sample_time); i++)
	{
		for (k = 1; k <= piecewise_num; k++)
		{
			coefficient[(time_block - sample_time) * (14 + k) + 1 + i][i * variable + find_variableName_position(variable_name, "zPfc" + to_string(k))] = -1.0;		 //z       //λi-zi  <=0
			coefficient[(time_block - sample_time) * (14 + k) + 1 + i][i * variable + find_variableName_position(variable_name, "lambda_Pfc" + to_string(k))] = 1.0; //λ
		}
	}
	for (i = 1; i <= (time_block - sample_time); i++)
	{
		for (j = 1; j <= piecewise_num; j++) //s-z<=0
		{
			glp_set_row_name(mip, ((time_block - sample_time) * (14 + j) + 1 + i), "");
			glp_set_row_bnds(mip, ((time_block - sample_time) * (14 + j) + 1 + i), GLP_UP, 0.0, 0.0);
		}
	}

	for (j = 0; j < (time_block - sample_time); j++)
	{
		glp_set_obj_coef(mip, (find_variableName_position(variable_name, "Pgrid") + 1 + j * variable), price2[j + sample_time] * delta_T);
		glp_set_obj_coef(mip, (find_variableName_position(variable_name, "Psell") + 1 + j * variable), price2[j + sample_time] * delta_T * (-1));
		glp_set_obj_coef(mip, (find_variableName_position(variable_name, "Pfct") + 1 + j * variable), Hydro_Price / Hydro_Cons * delta_T); //FC cost
	}

	int *ia = new int[rowTotal * colTotal + 1];
	int *ja = new int[rowTotal * colTotal + 1];
	double *ar = new double[rowTotal * colTotal + 1];
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
	//parm.ps_heur = GLP_ON;
	//parm.bt_tech = GLP_BT_BPH;
	//parm.br_tech = GLP_BR_PCH;

	//cloud
	// parm.gmi_cuts = GLP_ON;
	// parm.ps_heur = GLP_ON;
	// parm.bt_tech = GLP_BT_BFS;
	// parm.br_tech = GLP_BR_PCH;

	//no fc+ no sell
	//fc+no sell
	parm.gmi_cuts = GLP_ON;
	parm.bt_tech = GLP_BT_BPH;
	parm.br_tech = GLP_BR_PCH;

	//FC+sell
	//parm.fp_heur = GLP_ON;
	// parm.bt_tech = GLP_BT_BPH;
	//parm.br_tech = GLP_BR_PCH;

	int err = glp_intopt(mip, &parm);

	z = glp_mip_obj_val(mip);
	printf("\n");
	printf("sol = %f; \n", z);

	if (z == 0.0 && glp_mip_col_val(mip, find_variableName_position(variable_name, "SOC") + 1) == 0.0)
	{
		printf("Error > sol is 0, No Solution, give up the solution\n");
		printf("%.2f\n", glp_mip_col_val(mip, find_variableName_position(variable_name, "SOC") + 1));
		system("pause");
		exit(1);
	}

	/*==============================��N?M?????????G???X==================================*/
	float *s = new float[time_block];

	//get now time that can used in the real experiment
	if (((now_time.tm_min) % (60 / divide)) != 0)
		noo = (now_time.tm_hour) * divide + (int)((now_time.tm_min) / (60 / divide));
	else
		noo = (now_time.tm_hour) * divide + (int)((now_time.tm_min) / (60 / divide));

	for (i = 1; i <= variable; i++)
	{
		h = i;
		if (sample_time == 0)
		{
			for (j = 0; j < time_block; j++)
			{
				s[j] = glp_mip_col_val(mip, h);
				h = (h + variable);
			}
			// =-=-=-=-=-=-=-=-=-=- update each variables's A0 ~ A95 in each for loop -=-=-=-=-=-=-=-=-=-= //
			snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO GHEMS_control_status (%s, equip_name) VALUES('%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%s');", column, s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15], s[16], s[17], s[18], s[19], s[20], s[21], s[22], s[23], s[24], s[25], s[26], s[27], s[28], s[29], s[30], s[31], s[32], s[33], s[34], s[35], s[36], s[37], s[38], s[39], s[40], s[41], s[42], s[43], s[44], s[45], s[46], s[47], s[48], s[49], s[50], s[51], s[52], s[53], s[54], s[55], s[56], s[57], s[58], s[59], s[60], s[61], s[62], s[63], s[64], s[65], s[66], s[67], s[68], s[69], s[70], s[71], s[72], s[73], s[74], s[75], s[76], s[77], s[78], s[79], s[80], s[81], s[82], s[83], s[84], s[85], s[86], s[87], s[88], s[89], s[90], s[91], s[92], s[93], s[94], s[95], variable_name[i - 1].c_str());
			sent_query();
		}

		if (sample_time != 0)
		{
			// =-=-=-=-=-=-=-=-=-=- history about the control status from each control id -=-=-=-=-=-=-=-=-=-= //
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT %s FROM GHEMS_control_status WHERE equip_name = '%s'", column, variable_name[i - 1].c_str());
			fetch_row_value();
			for (int k = 0; k < sample_time; k++)
			{
				s[k] = turn_float(k);
			}
			// =-=-=-=-=-=-=-=-=-=- change new result after the sample time -=-=-=-=-=-=-=-=-=-= //
			for (j = 0; j < (time_block - sample_time); j++)
			{
				s[j + sample_time] = glp_mip_col_val(mip, h);
				h = (h + variable);
			}
			// =-=-=-=-=-=-=-=-=-=- full result update -=-=-=-=-=-=-=-=-=-= //
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE GHEMS_control_status set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f' WHERE equip_name = '%s';", s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15], s[16], s[17], s[18], s[19], s[20], s[21], s[22], s[23], s[24], s[25], s[26], s[27], s[28], s[29], s[30], s[31], s[32], s[33], s[34], s[35], s[36], s[37], s[38], s[39], s[40], s[41], s[42], s[43], s[44], s[45], s[46], s[47], s[48], s[49], s[50], s[51], s[52], s[53], s[54], s[55], s[56], s[57], s[58], s[59], s[60], s[61], s[62], s[63], s[64], s[65], s[66], s[67], s[68], s[69], s[70], s[71], s[72], s[73], s[74], s[75], s[76], s[77], s[78], s[79], s[80], s[81], s[82], s[83], s[84], s[85], s[86], s[87], s[88], s[89], s[90], s[91], s[92], s[93], s[94], s[95], variable_name[i - 1].c_str());
			sent_query();

			// =-=-=-=-=-=-=-=-=-=- result update from the sample time until end timeblock (96) -=-=-=-=-=-=-=-=-=-= //
			for (j = 0; j < sample_time; j++)
			{
				s[j] = 0;
			}
			snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO GHEMS_real_status (%s, equip_name) VALUES('%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%s');", column, s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10], s[11], s[12], s[13], s[14], s[15], s[16], s[17], s[18], s[19], s[20], s[21], s[22], s[23], s[24], s[25], s[26], s[27], s[28], s[29], s[30], s[31], s[32], s[33], s[34], s[35], s[36], s[37], s[38], s[39], s[40], s[41], s[42], s[43], s[44], s[45], s[46], s[47], s[48], s[49], s[50], s[51], s[52], s[53], s[54], s[55], s[56], s[57], s[58], s[59], s[60], s[61], s[62], s[63], s[64], s[65], s[66], s[67], s[68], s[69], s[70], s[71], s[72], s[73], s[74], s[75], s[76], s[77], s[78], s[79], s[80], s[81], s[82], s[83], s[84], s[85], s[86], s[87], s[88], s[89], s[90], s[91], s[92], s[93], s[94], s[95], variable_name[i - 1].c_str());
			sent_query();
		}
	}

	glp_delete_prob(mip);
	delete[] ia, ja, ar, s;
	delete[] coefficient;
	return;
}

int determine_realTimeOrOneDayMode_andGetSOC(int same_day, int real_time, vector<string> variable_name)
{
	// 'Realtime mode' if same day & real time = 1;
	// 'One day mode' =>
	// 		1. SOC = 0.7 if real_time = 0,
	// 		2. Use Previous SOC if real_time = 1.
	functionPrint(__func__);
	if (same_day == 1 && real_time == 1)
	{
		messagePrint(__LINE__, "Real Time Mode...", 'S', 0, 'Y');

		snprintf(sql_buffer, sizeof(sql_buffer), "TRUNCATE TABLE GHEMS_real_status"); //clean GHEMS_real_status;
		sent_query();

		// get previous SOC value
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM GHEMS_control_status WHERE control_id = %d ", sample_time - 1, find_variableName_position(variable_name, "SOC") + 1);
		SOC_ini = turn_value_to_float(0);
		messagePrint(__LINE__, "SOC = ", 'F', SOC_ini, 'Y');

		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE `BaseParameter` SET `value` = '%f' WHERE parameter_name = 'now_SOC' ", SOC_ini);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value FROM BaseParameter WHERE parameter_name = 'now_SOC' "); //get now_SOC
		SOC_ini = turn_value_to_float(0);
		if (SOC_ini > 90)
			SOC_ini = 89.8;

		messagePrint(__LINE__, "NOW REAL SOC = ", 'F', SOC_ini, 'Y');
		messagePrint(__LINE__, "Should same as previous SOC or 89.8 (previous SOC > 90)", 'S', 0, 'Y');
	}
	else
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "TRUNCATE TABLE GHEMS_control_status");
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "TRUNCATE TABLE GHEMS_real_status");
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "TRUNCATE TABLE cost");
		sent_query();

		if (real_time == 0)
		{
			// don't consider the day before yesterday SOC
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value FROM BaseParameter WHERE parameter_name = 'ini_SOC' "); //get ini_SOC
			SOC_ini = turn_value_to_float(0);
			messagePrint(__LINE__, "ini_SOC : ", 'F', SOC_ini, 'Y');
		}
		else
		{
			// consider the day before yesterday SOC
			snprintf(sql_buffer, sizeof(sql_buffer), "SELECT value FROM BaseParameter WHERE parameter_name = 'now_SOC' "); //get now_SOC
			SOC_ini = turn_value_to_float(0);
			messagePrint(__LINE__, "now_SOC : ", 'F', SOC_ini, 'Y');
		}

		sample_time = 0;
		real_time = 1; //if you don't want do real_time,please commend it.
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %d WHERE parameter_name = 'Global_real_time' ", real_time);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %d WHERE parameter_name = 'Global_next_simulate_timeblock' ", sample_time);
		sent_query();
	}

	return real_time;
}

void updateBaseParameter_from_1To(int length, vector<float> parameter_tmp)
{
	functionPrint(__func__);
	for (int i = 1; i <= length; i++)
	{
		if (parameter_tmp[i - 1] - (int)parameter_tmp[i - 1] != 0)
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = '%.3f' WHERE  parameter_id = '%d'", parameter_tmp[i - 1], i);
			sent_query();
		}
		else
		{
			snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = '%.0f' WHERE  parameter_id = '%d'", parameter_tmp[i - 1], i);
			sent_query();
		}
	}
}

void print_BaseParameter_SystemState(int real_time, int same_day)
{

	functionPrint(__func__);
	messagePrint(__LINE__, "time block : ", 'I', time_block, 'Y');
	messagePrint(__LINE__, "variable numbers : ", 'I', variable, 'Y');
	messagePrint(__LINE__, "system voltage : ", 'I', Vsys, 'Y');
	printf("\tLINE %d: battery capacity:%.3f\n", __LINE__, Cbat);
	messagePrint(__LINE__, "SOC min : ", 'F', SOC_min, 'Y');
	messagePrint(__LINE__, "SOC max : ", 'F', SOC_max, 'Y');
	messagePrint(__LINE__, "SOC max : ", 'F', SOC_max, 'Y');
	messagePrint(__LINE__, "SOC threads : ", 'F', SOC_thres, 'Y');
	messagePrint(__LINE__, "Pbat min : ", 'F', Pbat_min, 'Y');
	messagePrint(__LINE__, "Pbat max : ", 'F', Pbat_max, 'Y');
	messagePrint(__LINE__, "Pgrid max : ", 'F', Pgrid_max, 'Y');
	messagePrint(__LINE__, "Psell max : ", 'F', Psell_max, 'Y');
	messagePrint(__LINE__, "User set for realtime(0->no 1->yes) : ", 'I', real_time, 'Y');
	messagePrint(__LINE__, "Last running were same day(0->no 1->yes) : ", 'I', same_day, 'Y');
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

void updateTableCost(float *totalLoad, float *totalLoad_price, float *real_grid_pirce, float *fuelCell_kW_price, float *Hydrogen_g_consumption, float *real_sell_pirce, float totalLoad_sum, float totalLoad_priceSum, float real_grid_pirceSum, float fuelCell_kW_priceSum, float Hydrogen_g_consumptionSum, float real_sell_pirceSum, float totalLoad_taipowerPriceSum)
{
	functionPrint(__func__);

	if (sample_time == 0)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "total_load_power", totalLoad[0], totalLoad[1], totalLoad[2], totalLoad[3], totalLoad[4], totalLoad[5], totalLoad[6], totalLoad[7], totalLoad[8], totalLoad[9], totalLoad[10], totalLoad[11], totalLoad[12], totalLoad[13], totalLoad[14], totalLoad[15], totalLoad[16], totalLoad[17], totalLoad[18], totalLoad[19], totalLoad[20], totalLoad[21], totalLoad[22], totalLoad[23], totalLoad[24], totalLoad[25], totalLoad[26], totalLoad[27], totalLoad[28], totalLoad[29], totalLoad[30], totalLoad[31], totalLoad[32], totalLoad[33], totalLoad[34], totalLoad[35], totalLoad[36], totalLoad[37], totalLoad[38], totalLoad[39], totalLoad[40], totalLoad[41], totalLoad[42], totalLoad[43], totalLoad[44], totalLoad[45], totalLoad[46], totalLoad[47], totalLoad[48], totalLoad[49], totalLoad[50], totalLoad[51], totalLoad[52], totalLoad[53], totalLoad[54], totalLoad[55], totalLoad[56], totalLoad[57], totalLoad[58], totalLoad[59], totalLoad[60], totalLoad[61], totalLoad[62], totalLoad[63], totalLoad[64], totalLoad[65], totalLoad[66], totalLoad[67], totalLoad[68], totalLoad[69], totalLoad[70], totalLoad[71], totalLoad[72], totalLoad[73], totalLoad[74], totalLoad[75], totalLoad[76], totalLoad[77], totalLoad[78], totalLoad[79], totalLoad[80], totalLoad[81], totalLoad[82], totalLoad[83], totalLoad[84], totalLoad[85], totalLoad[86], totalLoad[87], totalLoad[88], totalLoad[89], totalLoad[90], totalLoad[91], totalLoad[92], totalLoad[93], totalLoad[94], totalLoad[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'totalLoad' ", totalLoad_sum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "total_load_price", totalLoad_price[0], totalLoad_price[1], totalLoad_price[2], totalLoad_price[3], totalLoad_price[4], totalLoad_price[5], totalLoad_price[6], totalLoad_price[7], totalLoad_price[8], totalLoad_price[9], totalLoad_price[10], totalLoad_price[11], totalLoad_price[12], totalLoad_price[13], totalLoad_price[14], totalLoad_price[15], totalLoad_price[16], totalLoad_price[17], totalLoad_price[18], totalLoad_price[19], totalLoad_price[20], totalLoad_price[21], totalLoad_price[22], totalLoad_price[23], totalLoad_price[24], totalLoad_price[25], totalLoad_price[26], totalLoad_price[27], totalLoad_price[28], totalLoad_price[29], totalLoad_price[30], totalLoad_price[31], totalLoad_price[32], totalLoad_price[33], totalLoad_price[34], totalLoad_price[35], totalLoad_price[36], totalLoad_price[37], totalLoad_price[38], totalLoad_price[39], totalLoad_price[40], totalLoad_price[41], totalLoad_price[42], totalLoad_price[43], totalLoad_price[44], totalLoad_price[45], totalLoad_price[46], totalLoad_price[47], totalLoad_price[48], totalLoad_price[49], totalLoad_price[50], totalLoad_price[51], totalLoad_price[52], totalLoad_price[53], totalLoad_price[54], totalLoad_price[55], totalLoad_price[56], totalLoad_price[57], totalLoad_price[58], totalLoad_price[59], totalLoad_price[60], totalLoad_price[61], totalLoad_price[62], totalLoad_price[63], totalLoad_price[64], totalLoad_price[65], totalLoad_price[66], totalLoad_price[67], totalLoad_price[68], totalLoad_price[69], totalLoad_price[70], totalLoad_price[71], totalLoad_price[72], totalLoad_price[73], totalLoad_price[74], totalLoad_price[75], totalLoad_price[76], totalLoad_price[77], totalLoad_price[78], totalLoad_price[79], totalLoad_price[80], totalLoad_price[81], totalLoad_price[82], totalLoad_price[83], totalLoad_price[84], totalLoad_price[85], totalLoad_price[86], totalLoad_price[87], totalLoad_price[88], totalLoad_price[89], totalLoad_price[90], totalLoad_price[91], totalLoad_price[92], totalLoad_price[93], totalLoad_price[94], totalLoad_price[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'LoadSpend(threeLevelPrice)' ", totalLoad_priceSum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "real_buy_grid_price", real_grid_pirce[0], real_grid_pirce[1], real_grid_pirce[2], real_grid_pirce[3], real_grid_pirce[4], real_grid_pirce[5], real_grid_pirce[6], real_grid_pirce[7], real_grid_pirce[8], real_grid_pirce[9], real_grid_pirce[10], real_grid_pirce[11], real_grid_pirce[12], real_grid_pirce[13], real_grid_pirce[14], real_grid_pirce[15], real_grid_pirce[16], real_grid_pirce[17], real_grid_pirce[18], real_grid_pirce[19], real_grid_pirce[20], real_grid_pirce[21], real_grid_pirce[22], real_grid_pirce[23], real_grid_pirce[24], real_grid_pirce[25], real_grid_pirce[26], real_grid_pirce[27], real_grid_pirce[28], real_grid_pirce[29], real_grid_pirce[30], real_grid_pirce[31], real_grid_pirce[32], real_grid_pirce[33], real_grid_pirce[34], real_grid_pirce[35], real_grid_pirce[36], real_grid_pirce[37], real_grid_pirce[38], real_grid_pirce[39], real_grid_pirce[40], real_grid_pirce[41], real_grid_pirce[42], real_grid_pirce[43], real_grid_pirce[44], real_grid_pirce[45], real_grid_pirce[46], real_grid_pirce[47], real_grid_pirce[48], real_grid_pirce[49], real_grid_pirce[50], real_grid_pirce[51], real_grid_pirce[52], real_grid_pirce[53], real_grid_pirce[54], real_grid_pirce[55], real_grid_pirce[56], real_grid_pirce[57], real_grid_pirce[58], real_grid_pirce[59], real_grid_pirce[60], real_grid_pirce[61], real_grid_pirce[62], real_grid_pirce[63], real_grid_pirce[64], real_grid_pirce[65], real_grid_pirce[66], real_grid_pirce[67], real_grid_pirce[68], real_grid_pirce[69], real_grid_pirce[70], real_grid_pirce[71], real_grid_pirce[72], real_grid_pirce[73], real_grid_pirce[74], real_grid_pirce[75], real_grid_pirce[76], real_grid_pirce[77], real_grid_pirce[78], real_grid_pirce[79], real_grid_pirce[80], real_grid_pirce[81], real_grid_pirce[82], real_grid_pirce[83], real_grid_pirce[84], real_grid_pirce[85], real_grid_pirce[86], real_grid_pirce[87], real_grid_pirce[88], real_grid_pirce[89], real_grid_pirce[90], real_grid_pirce[91], real_grid_pirce[92], real_grid_pirce[93], real_grid_pirce[94], real_grid_pirce[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'realGridPurchase' ", real_grid_pirceSum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name,%s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "real_sell_grid_price", real_sell_pirce[0], real_sell_pirce[1], real_sell_pirce[2], real_sell_pirce[3], real_sell_pirce[4], real_sell_pirce[5], real_sell_pirce[6], real_sell_pirce[7], real_sell_pirce[8], real_sell_pirce[9], real_sell_pirce[10], real_sell_pirce[11], real_sell_pirce[12], real_sell_pirce[13], real_sell_pirce[14], real_sell_pirce[15], real_sell_pirce[16], real_sell_pirce[17], real_sell_pirce[18], real_sell_pirce[19], real_sell_pirce[20], real_sell_pirce[21], real_sell_pirce[22], real_sell_pirce[23], real_sell_pirce[24], real_sell_pirce[25], real_sell_pirce[26], real_sell_pirce[27], real_sell_pirce[28], real_sell_pirce[29], real_sell_pirce[30], real_sell_pirce[31], real_sell_pirce[32], real_sell_pirce[33], real_sell_pirce[34], real_sell_pirce[35], real_sell_pirce[36], real_sell_pirce[37], real_sell_pirce[38], real_sell_pirce[39], real_sell_pirce[40], real_sell_pirce[41], real_sell_pirce[42], real_sell_pirce[43], real_sell_pirce[44], real_sell_pirce[45], real_sell_pirce[46], real_sell_pirce[47], real_sell_pirce[48], real_sell_pirce[49], real_sell_pirce[50], real_sell_pirce[51], real_sell_pirce[52], real_sell_pirce[53], real_sell_pirce[54], real_sell_pirce[55], real_sell_pirce[56], real_sell_pirce[57], real_sell_pirce[58], real_sell_pirce[59], real_sell_pirce[60], real_sell_pirce[61], real_sell_pirce[62], real_sell_pirce[63], real_sell_pirce[64], real_sell_pirce[65], real_sell_pirce[66], real_sell_pirce[67], real_sell_pirce[68], real_sell_pirce[69], real_sell_pirce[70], real_sell_pirce[71], real_sell_pirce[72], real_sell_pirce[73], real_sell_pirce[74], real_sell_pirce[75], real_sell_pirce[76], real_sell_pirce[77], real_sell_pirce[78], real_sell_pirce[79], real_sell_pirce[80], real_sell_pirce[81], real_sell_pirce[82], real_sell_pirce[83], real_sell_pirce[84], real_sell_pirce[85], real_sell_pirce[86], real_sell_pirce[87], real_sell_pirce[88], real_sell_pirce[89], real_sell_pirce[90], real_sell_pirce[91], real_sell_pirce[92], real_sell_pirce[93], real_sell_pirce[94], real_sell_pirce[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'maximumSell' ", real_sell_pirceSum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "FC_price", fuelCell_kW_price[0], fuelCell_kW_price[1], fuelCell_kW_price[2], fuelCell_kW_price[3], fuelCell_kW_price[4], fuelCell_kW_price[5], fuelCell_kW_price[6], fuelCell_kW_price[7], fuelCell_kW_price[8], fuelCell_kW_price[9], fuelCell_kW_price[10], fuelCell_kW_price[11], fuelCell_kW_price[12], fuelCell_kW_price[13], fuelCell_kW_price[14], fuelCell_kW_price[15], fuelCell_kW_price[16], fuelCell_kW_price[17], fuelCell_kW_price[18], fuelCell_kW_price[19], fuelCell_kW_price[20], fuelCell_kW_price[21], fuelCell_kW_price[22], fuelCell_kW_price[23], fuelCell_kW_price[24], fuelCell_kW_price[25], fuelCell_kW_price[26], fuelCell_kW_price[27], fuelCell_kW_price[28], fuelCell_kW_price[29], fuelCell_kW_price[30], fuelCell_kW_price[31], fuelCell_kW_price[32], fuelCell_kW_price[33], fuelCell_kW_price[34], fuelCell_kW_price[35], fuelCell_kW_price[36], fuelCell_kW_price[37], fuelCell_kW_price[38], fuelCell_kW_price[39], fuelCell_kW_price[40], fuelCell_kW_price[41], fuelCell_kW_price[42], fuelCell_kW_price[43], fuelCell_kW_price[44], fuelCell_kW_price[45], fuelCell_kW_price[46], fuelCell_kW_price[47], fuelCell_kW_price[48], fuelCell_kW_price[49], fuelCell_kW_price[50], fuelCell_kW_price[51], fuelCell_kW_price[52], fuelCell_kW_price[53], fuelCell_kW_price[54], fuelCell_kW_price[55], fuelCell_kW_price[56], fuelCell_kW_price[57], fuelCell_kW_price[58], fuelCell_kW_price[59], fuelCell_kW_price[60], fuelCell_kW_price[61], fuelCell_kW_price[62], fuelCell_kW_price[63], fuelCell_kW_price[64], fuelCell_kW_price[65], fuelCell_kW_price[66], fuelCell_kW_price[67], fuelCell_kW_price[68], fuelCell_kW_price[69], fuelCell_kW_price[70], fuelCell_kW_price[71], fuelCell_kW_price[72], fuelCell_kW_price[73], fuelCell_kW_price[74], fuelCell_kW_price[75], fuelCell_kW_price[76], fuelCell_kW_price[77], fuelCell_kW_price[78], fuelCell_kW_price[79], fuelCell_kW_price[80], fuelCell_kW_price[81], fuelCell_kW_price[82], fuelCell_kW_price[83], fuelCell_kW_price[84], fuelCell_kW_price[85], fuelCell_kW_price[86], fuelCell_kW_price[87], fuelCell_kW_price[88], fuelCell_kW_price[89], fuelCell_kW_price[90], fuelCell_kW_price[91], fuelCell_kW_price[92], fuelCell_kW_price[93], fuelCell_kW_price[94], fuelCell_kW_price[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'fuelCellSpend' ", fuelCell_kW_priceSum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "INSERT INTO cost (cost_name, %s) VALUES('%s','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f','%.3f');", column, "hydrogen_consumption", Hydrogen_g_consumption[0], Hydrogen_g_consumption[1], Hydrogen_g_consumption[2], Hydrogen_g_consumption[3], Hydrogen_g_consumption[4], Hydrogen_g_consumption[5], Hydrogen_g_consumption[6], Hydrogen_g_consumption[7], Hydrogen_g_consumption[8], Hydrogen_g_consumption[9], Hydrogen_g_consumption[10], Hydrogen_g_consumption[11], Hydrogen_g_consumption[12], Hydrogen_g_consumption[13], Hydrogen_g_consumption[14], Hydrogen_g_consumption[15], Hydrogen_g_consumption[16], Hydrogen_g_consumption[17], Hydrogen_g_consumption[18], Hydrogen_g_consumption[19], Hydrogen_g_consumption[20], Hydrogen_g_consumption[21], Hydrogen_g_consumption[22], Hydrogen_g_consumption[23], Hydrogen_g_consumption[24], Hydrogen_g_consumption[25], Hydrogen_g_consumption[26], Hydrogen_g_consumption[27], Hydrogen_g_consumption[28], Hydrogen_g_consumption[29], Hydrogen_g_consumption[30], Hydrogen_g_consumption[31], Hydrogen_g_consumption[32], Hydrogen_g_consumption[33], Hydrogen_g_consumption[34], Hydrogen_g_consumption[35], Hydrogen_g_consumption[36], Hydrogen_g_consumption[37], Hydrogen_g_consumption[38], Hydrogen_g_consumption[39], Hydrogen_g_consumption[40], Hydrogen_g_consumption[41], Hydrogen_g_consumption[42], Hydrogen_g_consumption[43], Hydrogen_g_consumption[44], Hydrogen_g_consumption[45], Hydrogen_g_consumption[46], Hydrogen_g_consumption[47], Hydrogen_g_consumption[48], Hydrogen_g_consumption[49], Hydrogen_g_consumption[50], Hydrogen_g_consumption[51], Hydrogen_g_consumption[52], Hydrogen_g_consumption[53], Hydrogen_g_consumption[54], Hydrogen_g_consumption[55], Hydrogen_g_consumption[56], Hydrogen_g_consumption[57], Hydrogen_g_consumption[58], Hydrogen_g_consumption[59], Hydrogen_g_consumption[60], Hydrogen_g_consumption[61], Hydrogen_g_consumption[62], Hydrogen_g_consumption[63], Hydrogen_g_consumption[64], Hydrogen_g_consumption[65], Hydrogen_g_consumption[66], Hydrogen_g_consumption[67], Hydrogen_g_consumption[68], Hydrogen_g_consumption[69], Hydrogen_g_consumption[70], Hydrogen_g_consumption[71], Hydrogen_g_consumption[72], Hydrogen_g_consumption[73], Hydrogen_g_consumption[74], Hydrogen_g_consumption[75], Hydrogen_g_consumption[76], Hydrogen_g_consumption[77], Hydrogen_g_consumption[78], Hydrogen_g_consumption[79], Hydrogen_g_consumption[80], Hydrogen_g_consumption[81], Hydrogen_g_consumption[82], Hydrogen_g_consumption[83], Hydrogen_g_consumption[84], Hydrogen_g_consumption[85], Hydrogen_g_consumption[86], Hydrogen_g_consumption[87], Hydrogen_g_consumption[88], Hydrogen_g_consumption[89], Hydrogen_g_consumption[90], Hydrogen_g_consumption[91], Hydrogen_g_consumption[92], Hydrogen_g_consumption[93], Hydrogen_g_consumption[94], Hydrogen_g_consumption[95]);
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'hydrogenConsumption(g)' ", Hydrogen_g_consumptionSum);
		sent_query();
	}
	else
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f', `datetime` = CURRENT_TIMESTAMP WHERE cost_name = '%s';", totalLoad[0], totalLoad[1], totalLoad[2], totalLoad[3], totalLoad[4], totalLoad[5], totalLoad[6], totalLoad[7], totalLoad[8], totalLoad[9], totalLoad[10], totalLoad[11], totalLoad[12], totalLoad[13], totalLoad[14], totalLoad[15], totalLoad[16], totalLoad[17], totalLoad[18], totalLoad[19], totalLoad[20], totalLoad[21], totalLoad[22], totalLoad[23], totalLoad[24], totalLoad[25], totalLoad[26], totalLoad[27], totalLoad[28], totalLoad[29], totalLoad[30], totalLoad[31], totalLoad[32], totalLoad[33], totalLoad[34], totalLoad[35], totalLoad[36], totalLoad[37], totalLoad[38], totalLoad[39], totalLoad[40], totalLoad[41], totalLoad[42], totalLoad[43], totalLoad[44], totalLoad[45], totalLoad[46], totalLoad[47], totalLoad[48], totalLoad[49], totalLoad[50], totalLoad[51], totalLoad[52], totalLoad[53], totalLoad[54], totalLoad[55], totalLoad[56], totalLoad[57], totalLoad[58], totalLoad[59], totalLoad[60], totalLoad[61], totalLoad[62], totalLoad[63], totalLoad[64], totalLoad[65], totalLoad[66], totalLoad[67], totalLoad[68], totalLoad[69], totalLoad[70], totalLoad[71], totalLoad[72], totalLoad[73], totalLoad[74], totalLoad[75], totalLoad[76], totalLoad[77], totalLoad[78], totalLoad[79], totalLoad[80], totalLoad[81], totalLoad[82], totalLoad[83], totalLoad[84], totalLoad[85], totalLoad[86], totalLoad[87], totalLoad[88], totalLoad[89], totalLoad[90], totalLoad[91], totalLoad[92], totalLoad[93], totalLoad[94], totalLoad[95], "total_load_power");
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'totalLoad' ", totalLoad_sum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f', `datetime` = CURRENT_TIMESTAMP WHERE cost_name = '%s';", totalLoad_price[0], totalLoad_price[1], totalLoad_price[2], totalLoad_price[3], totalLoad_price[4], totalLoad_price[5], totalLoad_price[6], totalLoad_price[7], totalLoad_price[8], totalLoad_price[9], totalLoad_price[10], totalLoad_price[11], totalLoad_price[12], totalLoad_price[13], totalLoad_price[14], totalLoad_price[15], totalLoad_price[16], totalLoad_price[17], totalLoad_price[18], totalLoad_price[19], totalLoad_price[20], totalLoad_price[21], totalLoad_price[22], totalLoad_price[23], totalLoad_price[24], totalLoad_price[25], totalLoad_price[26], totalLoad_price[27], totalLoad_price[28], totalLoad_price[29], totalLoad_price[30], totalLoad_price[31], totalLoad_price[32], totalLoad_price[33], totalLoad_price[34], totalLoad_price[35], totalLoad_price[36], totalLoad_price[37], totalLoad_price[38], totalLoad_price[39], totalLoad_price[40], totalLoad_price[41], totalLoad_price[42], totalLoad_price[43], totalLoad_price[44], totalLoad_price[45], totalLoad_price[46], totalLoad_price[47], totalLoad_price[48], totalLoad_price[49], totalLoad_price[50], totalLoad_price[51], totalLoad_price[52], totalLoad_price[53], totalLoad_price[54], totalLoad_price[55], totalLoad_price[56], totalLoad_price[57], totalLoad_price[58], totalLoad_price[59], totalLoad_price[60], totalLoad_price[61], totalLoad_price[62], totalLoad_price[63], totalLoad_price[64], totalLoad_price[65], totalLoad_price[66], totalLoad_price[67], totalLoad_price[68], totalLoad_price[69], totalLoad_price[70], totalLoad_price[71], totalLoad_price[72], totalLoad_price[73], totalLoad_price[74], totalLoad_price[75], totalLoad_price[76], totalLoad_price[77], totalLoad_price[78], totalLoad_price[79], totalLoad_price[80], totalLoad_price[81], totalLoad_price[82], totalLoad_price[83], totalLoad_price[84], totalLoad_price[85], totalLoad_price[86], totalLoad_price[87], totalLoad_price[88], totalLoad_price[89], totalLoad_price[90], totalLoad_price[91], totalLoad_price[92], totalLoad_price[93], totalLoad_price[94], totalLoad_price[95], "total_load_price");
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'LoadSpend(threeLevelPrice)' ", totalLoad_priceSum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f', `datetime` = CURRENT_TIMESTAMP WHERE cost_name = '%s';", real_grid_pirce[0], real_grid_pirce[1], real_grid_pirce[2], real_grid_pirce[3], real_grid_pirce[4], real_grid_pirce[5], real_grid_pirce[6], real_grid_pirce[7], real_grid_pirce[8], real_grid_pirce[9], real_grid_pirce[10], real_grid_pirce[11], real_grid_pirce[12], real_grid_pirce[13], real_grid_pirce[14], real_grid_pirce[15], real_grid_pirce[16], real_grid_pirce[17], real_grid_pirce[18], real_grid_pirce[19], real_grid_pirce[20], real_grid_pirce[21], real_grid_pirce[22], real_grid_pirce[23], real_grid_pirce[24], real_grid_pirce[25], real_grid_pirce[26], real_grid_pirce[27], real_grid_pirce[28], real_grid_pirce[29], real_grid_pirce[30], real_grid_pirce[31], real_grid_pirce[32], real_grid_pirce[33], real_grid_pirce[34], real_grid_pirce[35], real_grid_pirce[36], real_grid_pirce[37], real_grid_pirce[38], real_grid_pirce[39], real_grid_pirce[40], real_grid_pirce[41], real_grid_pirce[42], real_grid_pirce[43], real_grid_pirce[44], real_grid_pirce[45], real_grid_pirce[46], real_grid_pirce[47], real_grid_pirce[48], real_grid_pirce[49], real_grid_pirce[50], real_grid_pirce[51], real_grid_pirce[52], real_grid_pirce[53], real_grid_pirce[54], real_grid_pirce[55], real_grid_pirce[56], real_grid_pirce[57], real_grid_pirce[58], real_grid_pirce[59], real_grid_pirce[60], real_grid_pirce[61], real_grid_pirce[62], real_grid_pirce[63], real_grid_pirce[64], real_grid_pirce[65], real_grid_pirce[66], real_grid_pirce[67], real_grid_pirce[68], real_grid_pirce[69], real_grid_pirce[70], real_grid_pirce[71], real_grid_pirce[72], real_grid_pirce[73], real_grid_pirce[74], real_grid_pirce[75], real_grid_pirce[76], real_grid_pirce[77], real_grid_pirce[78], real_grid_pirce[79], real_grid_pirce[80], real_grid_pirce[81], real_grid_pirce[82], real_grid_pirce[83], real_grid_pirce[84], real_grid_pirce[85], real_grid_pirce[86], real_grid_pirce[87], real_grid_pirce[88], real_grid_pirce[89], real_grid_pirce[90], real_grid_pirce[91], real_grid_pirce[92], real_grid_pirce[93], real_grid_pirce[94], real_grid_pirce[95], "real_buy_grid_price");
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'realGridPurchase' ", real_grid_pirceSum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f', `datetime` = CURRENT_TIMESTAMP WHERE cost_name = '%s';", real_sell_pirce[0], real_sell_pirce[1], real_sell_pirce[2], real_sell_pirce[3], real_sell_pirce[4], real_sell_pirce[5], real_sell_pirce[6], real_sell_pirce[7], real_sell_pirce[8], real_sell_pirce[9], real_sell_pirce[10], real_sell_pirce[11], real_sell_pirce[12], real_sell_pirce[13], real_sell_pirce[14], real_sell_pirce[15], real_sell_pirce[16], real_sell_pirce[17], real_sell_pirce[18], real_sell_pirce[19], real_sell_pirce[20], real_sell_pirce[21], real_sell_pirce[22], real_sell_pirce[23], real_sell_pirce[24], real_sell_pirce[25], real_sell_pirce[26], real_sell_pirce[27], real_sell_pirce[28], real_sell_pirce[29], real_sell_pirce[30], real_sell_pirce[31], real_sell_pirce[32], real_sell_pirce[33], real_sell_pirce[34], real_sell_pirce[35], real_sell_pirce[36], real_sell_pirce[37], real_sell_pirce[38], real_sell_pirce[39], real_sell_pirce[40], real_sell_pirce[41], real_sell_pirce[42], real_sell_pirce[43], real_sell_pirce[44], real_sell_pirce[45], real_sell_pirce[46], real_sell_pirce[47], real_sell_pirce[48], real_sell_pirce[49], real_sell_pirce[50], real_sell_pirce[51], real_sell_pirce[52], real_sell_pirce[53], real_sell_pirce[54], real_sell_pirce[55], real_sell_pirce[56], real_sell_pirce[57], real_sell_pirce[58], real_sell_pirce[59], real_sell_pirce[60], real_sell_pirce[61], real_sell_pirce[62], real_sell_pirce[63], real_sell_pirce[64], real_sell_pirce[65], real_sell_pirce[66], real_sell_pirce[67], real_sell_pirce[68], real_sell_pirce[69], real_sell_pirce[70], real_sell_pirce[71], real_sell_pirce[72], real_sell_pirce[73], real_sell_pirce[74], real_sell_pirce[75], real_sell_pirce[76], real_sell_pirce[77], real_sell_pirce[78], real_sell_pirce[79], real_sell_pirce[80], real_sell_pirce[81], real_sell_pirce[82], real_sell_pirce[83], real_sell_pirce[84], real_sell_pirce[85], real_sell_pirce[86], real_sell_pirce[87], real_sell_pirce[88], real_sell_pirce[89], real_sell_pirce[90], real_sell_pirce[91], real_sell_pirce[92], real_sell_pirce[93], real_sell_pirce[94], real_sell_pirce[95], "real_sell_grid_price");
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'maximumSell' ", real_sell_pirceSum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f', `datetime` = CURRENT_TIMESTAMP WHERE cost_name = '%s';", fuelCell_kW_price[0], fuelCell_kW_price[1], fuelCell_kW_price[2], fuelCell_kW_price[3], fuelCell_kW_price[4], fuelCell_kW_price[5], fuelCell_kW_price[6], fuelCell_kW_price[7], fuelCell_kW_price[8], fuelCell_kW_price[9], fuelCell_kW_price[10], fuelCell_kW_price[11], fuelCell_kW_price[12], fuelCell_kW_price[13], fuelCell_kW_price[14], fuelCell_kW_price[15], fuelCell_kW_price[16], fuelCell_kW_price[17], fuelCell_kW_price[18], fuelCell_kW_price[19], fuelCell_kW_price[20], fuelCell_kW_price[21], fuelCell_kW_price[22], fuelCell_kW_price[23], fuelCell_kW_price[24], fuelCell_kW_price[25], fuelCell_kW_price[26], fuelCell_kW_price[27], fuelCell_kW_price[28], fuelCell_kW_price[29], fuelCell_kW_price[30], fuelCell_kW_price[31], fuelCell_kW_price[32], fuelCell_kW_price[33], fuelCell_kW_price[34], fuelCell_kW_price[35], fuelCell_kW_price[36], fuelCell_kW_price[37], fuelCell_kW_price[38], fuelCell_kW_price[39], fuelCell_kW_price[40], fuelCell_kW_price[41], fuelCell_kW_price[42], fuelCell_kW_price[43], fuelCell_kW_price[44], fuelCell_kW_price[45], fuelCell_kW_price[46], fuelCell_kW_price[47], fuelCell_kW_price[48], fuelCell_kW_price[49], fuelCell_kW_price[50], fuelCell_kW_price[51], fuelCell_kW_price[52], fuelCell_kW_price[53], fuelCell_kW_price[54], fuelCell_kW_price[55], fuelCell_kW_price[56], fuelCell_kW_price[57], fuelCell_kW_price[58], fuelCell_kW_price[59], fuelCell_kW_price[60], fuelCell_kW_price[61], fuelCell_kW_price[62], fuelCell_kW_price[63], fuelCell_kW_price[64], fuelCell_kW_price[65], fuelCell_kW_price[66], fuelCell_kW_price[67], fuelCell_kW_price[68], fuelCell_kW_price[69], fuelCell_kW_price[70], fuelCell_kW_price[71], fuelCell_kW_price[72], fuelCell_kW_price[73], fuelCell_kW_price[74], fuelCell_kW_price[75], fuelCell_kW_price[76], fuelCell_kW_price[77], fuelCell_kW_price[78], fuelCell_kW_price[79], fuelCell_kW_price[80], fuelCell_kW_price[81], fuelCell_kW_price[82], fuelCell_kW_price[83], fuelCell_kW_price[84], fuelCell_kW_price[85], fuelCell_kW_price[86], fuelCell_kW_price[87], fuelCell_kW_price[88], fuelCell_kW_price[89], fuelCell_kW_price[90], fuelCell_kW_price[91], fuelCell_kW_price[92], fuelCell_kW_price[93], fuelCell_kW_price[94], fuelCell_kW_price[95], "FC_price");
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'fuelCellSpend' ", fuelCell_kW_priceSum);
		sent_query();

		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE cost set A0 = '%.3f', A1 = '%.3f', A2 = '%.3f', A3 = '%.3f', A4 = '%.3f', A5 = '%.3f', A6 = '%.3f', A7 = '%.3f', A8 = '%.3f', A9 = '%.3f', A10 = '%.3f', A11 = '%.3f', A12 = '%.3f', A13 = '%.3f', A14 = '%.3f', A15 = '%.3f', A16 = '%.3f', A17 = '%.3f', A18 = '%.3f', A19 = '%.3f', A20 = '%.3f', A21 = '%.3f', A22 = '%.3f', A23 = '%.3f', A24 = '%.3f', A25 = '%.3f', A26 = '%.3f', A27 = '%.3f', A28 = '%.3f', A29 = '%.3f', A30 = '%.3f', A31 = '%.3f', A32 = '%.3f', A33 = '%.3f', A34 = '%.3f', A35 = '%.3f', A36 = '%.3f', A37 = '%.3f', A38 = '%.3f', A39 = '%.3f', A40 = '%.3f', A41 = '%.3f', A42 = '%.3f', A43 = '%.3f', A44 = '%.3f', A45 = '%.3f', A46 = '%.3f', A47 = '%.3f', A48 = '%.3f', A49 = '%.3f', A50 = '%.3f', A51 = '%.3f', A52 = '%.3f', A53 = '%.3f', A54 = '%.3f', A55 = '%.3f', A56 = '%.3f', A57 = '%.3f', A58 = '%.3f', A59 = '%.3f', A60 = '%.3f', A61 = '%.3f', A62 = '%.3f', A63 = '%.3f', A64 = '%.3f', A65 = '%.3f', A66 = '%.3f', A67 = '%.3f', A68 = '%.3f', A69 = '%.3f', A70 = '%.3f', A71 = '%.3f', A72 = '%.3f', A73 = '%.3f', A74 = '%.3f', A75 = '%.3f', A76 = '%.3f', A77 = '%.3f', A78 = '%.3f', A79 = '%.3f', A80 = '%.3f', A81 = '%.3f', A82 = '%.3f', A83 = '%.3f', A84 = '%.3f', A85 = '%.3f', A86 = '%.3f', A87 = '%.3f', A88 = '%.3f', A89 = '%.3f', A90 = '%.3f', A91 = '%.3f', A92 = '%.3f', A93 = '%.3f', A94 = '%.3f', A95 = '%.3f', `datetime` = CURRENT_TIMESTAMP WHERE cost_name = '%s';", Hydrogen_g_consumption[0], Hydrogen_g_consumption[1], Hydrogen_g_consumption[2], Hydrogen_g_consumption[3], Hydrogen_g_consumption[4], Hydrogen_g_consumption[5], Hydrogen_g_consumption[6], Hydrogen_g_consumption[7], Hydrogen_g_consumption[8], Hydrogen_g_consumption[9], Hydrogen_g_consumption[10], Hydrogen_g_consumption[11], Hydrogen_g_consumption[12], Hydrogen_g_consumption[13], Hydrogen_g_consumption[14], Hydrogen_g_consumption[15], Hydrogen_g_consumption[16], Hydrogen_g_consumption[17], Hydrogen_g_consumption[18], Hydrogen_g_consumption[19], Hydrogen_g_consumption[20], Hydrogen_g_consumption[21], Hydrogen_g_consumption[22], Hydrogen_g_consumption[23], Hydrogen_g_consumption[24], Hydrogen_g_consumption[25], Hydrogen_g_consumption[26], Hydrogen_g_consumption[27], Hydrogen_g_consumption[28], Hydrogen_g_consumption[29], Hydrogen_g_consumption[30], Hydrogen_g_consumption[31], Hydrogen_g_consumption[32], Hydrogen_g_consumption[33], Hydrogen_g_consumption[34], Hydrogen_g_consumption[35], Hydrogen_g_consumption[36], Hydrogen_g_consumption[37], Hydrogen_g_consumption[38], Hydrogen_g_consumption[39], Hydrogen_g_consumption[40], Hydrogen_g_consumption[41], Hydrogen_g_consumption[42], Hydrogen_g_consumption[43], Hydrogen_g_consumption[44], Hydrogen_g_consumption[45], Hydrogen_g_consumption[46], Hydrogen_g_consumption[47], Hydrogen_g_consumption[48], Hydrogen_g_consumption[49], Hydrogen_g_consumption[50], Hydrogen_g_consumption[51], Hydrogen_g_consumption[52], Hydrogen_g_consumption[53], Hydrogen_g_consumption[54], Hydrogen_g_consumption[55], Hydrogen_g_consumption[56], Hydrogen_g_consumption[57], Hydrogen_g_consumption[58], Hydrogen_g_consumption[59], Hydrogen_g_consumption[60], Hydrogen_g_consumption[61], Hydrogen_g_consumption[62], Hydrogen_g_consumption[63], Hydrogen_g_consumption[64], Hydrogen_g_consumption[65], Hydrogen_g_consumption[66], Hydrogen_g_consumption[67], Hydrogen_g_consumption[68], Hydrogen_g_consumption[69], Hydrogen_g_consumption[70], Hydrogen_g_consumption[71], Hydrogen_g_consumption[72], Hydrogen_g_consumption[73], Hydrogen_g_consumption[74], Hydrogen_g_consumption[75], Hydrogen_g_consumption[76], Hydrogen_g_consumption[77], Hydrogen_g_consumption[78], Hydrogen_g_consumption[79], Hydrogen_g_consumption[80], Hydrogen_g_consumption[81], Hydrogen_g_consumption[82], Hydrogen_g_consumption[83], Hydrogen_g_consumption[84], Hydrogen_g_consumption[85], Hydrogen_g_consumption[86], Hydrogen_g_consumption[87], Hydrogen_g_consumption[88], Hydrogen_g_consumption[89], Hydrogen_g_consumption[90], Hydrogen_g_consumption[91], Hydrogen_g_consumption[92], Hydrogen_g_consumption[93], Hydrogen_g_consumption[94], Hydrogen_g_consumption[95], "hydrogen_consumption");
		sent_query();
		snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'hydrogenConsumption(g)' ", Hydrogen_g_consumptionSum);
		sent_query();
	}

	snprintf(sql_buffer, sizeof(sql_buffer), "UPDATE BaseParameter SET value = %f WHERE parameter_name = 'LoadSpend(taipowerPrice)' ", totalLoad_taipowerPriceSum);
	sent_query();

	messagePrint(__LINE__, "total loads power cost(kW): ", 'F', totalLoad_sum, 'Y');
	messagePrint(__LINE__, "total loads power cost by taipower(NTD): ", 'F', totalLoad_taipowerPriceSum, 'Y');
	messagePrint(__LINE__, "total loads power cost by three level electric price(NTD): ", 'F', totalLoad_priceSum, 'Y');
	messagePrint(__LINE__, "buy total grid(NTD): ", 'F', real_grid_pirceSum, 'Y');
	// messagePrint(__LINE__, "Grid_sell(NTD):  ", 'F', opt_sell_result, 'Y');
	messagePrint(__LINE__, "fuelCell cost(NTD): ", 'F', fuelCell_kW_priceSum, 'Y');
	messagePrint(__LINE__, "hydrogen comsumotion(g): ", 'F', Hydrogen_g_consumptionSum, 'Y');
	// step1_bill = opt_cost_result - opt_sell_result;
	// step1_sell = opt_sell_result;
}

void calculateCostInfo(float *price2)
{
	functionPrint(__func__);

	float totalLoad[time_block] = {0.0}, totalLoad_price[time_block] = {0.0}, real_grid_pirce[time_block] = {0.0};
	float totalLoad_sum = 0.0, totalLoad_priceSum = 0.0, real_grid_pirceSum = 0.0, totalLoad_taipowerPriceSum = 0.0;
	float fuelCell_kW_price[time_block] = {0.0}, Hydrogen_g_consumption[time_block] = {0.0};
	float fuelCell_kW_priceSum = 0.0, Hydrogen_g_consumptionSum = 0.0;
	float real_sell_pirce[time_block] = {0.0}, real_sell_pirceSum = 0.0;

	for (int i = 0; i < sample_time; i++)
	{
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM cost WHERE cost_name = '%s'", i, "total_load_power");
		totalLoad[i] = turn_value_to_float(0);
		totalLoad_sum += totalLoad[i];

		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM cost WHERE cost_name = '%s'", i, "total_load_price");
		totalLoad_price[i] = turn_value_to_float(0);
		totalLoad_priceSum += totalLoad_price[i];

		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM cost WHERE cost_name = '%s'", i, "real_buy_grid_price");
		real_grid_pirce[i] = turn_value_to_float(0);
		real_grid_pirceSum += real_grid_pirce[i];

		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM cost WHERE cost_name = '%s'", i, "real_sell_grid_price");
		real_sell_pirce[i] = turn_value_to_float(0);
		real_sell_pirceSum += real_sell_pirce[i];

		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM cost WHERE cost_name = '%s'", i, "FC_price");
		fuelCell_kW_price[i] = turn_value_to_float(0);
		fuelCell_kW_priceSum += fuelCell_kW_price[i];

		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM cost WHERE cost_name = '%s'", i, "hydrogen_consumption");
		Hydrogen_g_consumption[i] = turn_value_to_float(0);
		Hydrogen_g_consumptionSum += Hydrogen_g_consumption[i];
	}

	for (int i = sample_time; i < time_block; i++)
	{
		// =-=-=-=-=-=-=- calculate total load spend how much money if only use grid power -=-=-=-=-=-=-= //
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT totalLoad FROM totalLoad_model WHERE time_block = %d ", i);
		totalLoad[i] = turn_value_to_float(0);
		totalLoad_sum += totalLoad[i];
		totalLoad_price[i] = totalLoad[i] * price2[i] * delta_T;
		totalLoad_priceSum += totalLoad_price[i];

		// =-=-=-=-=-=-=- calcalte optimize Pgrid consumption spend how much money -=-=-=-=-=-=-= //
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM GHEMS_control_status WHERE equip_name = '%s' ", i, "Pgrid");
		real_grid_pirce[i] = turn_value_to_float(0) * price2[i] * delta_T;
		real_grid_pirceSum += real_grid_pirce[i];

		// =-=-=-=-=-=-=- calcalte optimize Psell consumption save how much money -=-=-=-=-=-=-= //
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM GHEMS_control_status WHERE equip_name = '%s' ", i, "Psell");
		real_sell_pirce[i] = turn_value_to_float(0) * price2[i] * delta_T;
		real_sell_pirceSum += real_sell_pirce[i];

		// =-=-=-=-=-=-=- calcalte optimize Pfct consumption how much money & how many grams hydrogen -=-=-=-=-=-=-= //
		snprintf(sql_buffer, sizeof(sql_buffer), "SELECT A%d FROM GHEMS_control_status WHERE equip_name = '%s' ", i, "Pfct");
		float fuelCell_tmp = turn_value_to_float(0);
		fuelCell_kW_price[i] = fuelCell_tmp * Hydro_Price / Hydro_Cons * delta_T;
		fuelCell_kW_priceSum += fuelCell_kW_price[i];
		Hydrogen_g_consumption[i] = fuelCell_tmp / Hydro_Cons * delta_T;
		Hydrogen_g_consumptionSum += Hydrogen_g_consumption[i];
	}

	//NOW taipower cost reference --> https://www.taipower.com.tw/upload/238/2018070210412196443.pdf
	if (totalLoad_sum <= (120.0 / 30.0))
		totalLoad_taipowerPriceSum = totalLoad_sum * P_1;

	else if ((totalLoad_sum > (120.0 / 30.0)) & (totalLoad_sum <= 330.0 / 30.0))
		totalLoad_taipowerPriceSum = (120.0 * P_1 + (totalLoad_sum * 30.0 - 120.0) * P_2) / 30.0;

	else if ((totalLoad_sum > (330.0 / 30.0)) & (totalLoad_sum <= 500.0 / 30.0))
		totalLoad_taipowerPriceSum = (120.0 * P_1 + (330.0 - 120.0) * P_2 + (totalLoad_sum * 30.0 - 330.0) * P_3) / 30.0;

	else if ((totalLoad_sum > (500.0 / 30.0)) & (totalLoad_sum <= 700.0 / 30.0))
		totalLoad_taipowerPriceSum = (120.0 * P_1 + (330.0 - 120.0) * P_2 + (500.0 - 330.0) * P_3 + (totalLoad_sum * 30.0 - 500.0) * P_4) / 30.0;

	else if ((totalLoad_sum > (700.0 / 30.0)) & (totalLoad_sum <= 1000.0 / 30.0))
		totalLoad_taipowerPriceSum = (120.0 * P_1 + (330.0 - 120.0) * P_2 + (500.0 - 330.0) * P_3 + (700.0 - 500.0) * P_4 + (totalLoad_sum * 30.0 - 700.0) * P_5) / 30.0;

	else if (totalLoad_sum > (1000.0 / 30.0))
		totalLoad_taipowerPriceSum = (120.0 * P_1 + (330.0 - 120.0) * P_2 + (500.0 - 330.0) * P_3 + (700.0 - 500.0) * P_4 + (1000.0 - 700.0) * P_5 + (totalLoad_sum * 30.0 - 1000.0) * P_6) / 30.0;

	updateTableCost(totalLoad, totalLoad_price, real_grid_pirce, fuelCell_kW_price, Hydrogen_g_consumption, real_sell_pirce, totalLoad_sum, totalLoad_priceSum, real_grid_pirceSum, fuelCell_kW_priceSum, Hydrogen_g_consumptionSum, real_sell_pirceSum, totalLoad_taipowerPriceSum);
}