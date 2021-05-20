#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> //162 165 76~86 579~581�Ȯɵ��ѱ�
#include <glpk.h> //435~438�h�gsame_day
#include <math.h>
#include <mysql/mysql.h>
#include <iostream> //504~513 sample_time ��b�iGLPK�e�A�o�˹L�@�Ѥ~���|�n���s��
#include "SQLFunction.hpp"
#include "fifty_LHEMS_function.hpp"

char column[400] = "A0,A1,A2,A3,A4,A5,A6,A7,A8,A9,A10,A11,A12,A13,A14,A15,A16,A17,A18,A19,A20,A21,A22,A23,A24,A25,A26,A27,A28,A29,A30,A31,A32,A33,A34,A35,A36,A37,A38,A39,A40,A41,A42,A43,A44,A45,A46,A47,A48,A49,A50,A51,A52,A53,A54,A55,A56,A57,A58,A59,A60,A61,A62,A63,A64,A65,A66,A67,A68,A69,A70,A71,A72,A73,A74,A75,A76,A77,A78,A79,A80,A81,A82,A83,A84,A85,A86,A87,A88,A89,A90,A91,A92,A93,A94,A95";

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
	glp_set_prob_name(mip, "LHEMS");
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

	double z = glp_mip_obj_val(mip);
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