#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <iostream>
#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <algorithm>
#include "SQLFunction.hpp"

using namespace std;
MYSQL *mysql_con = mysql_init(NULL);
MYSQL_RES *mysql_result;
MYSQL_ROW mysql_row;
char sql_buffer[2000] = {'\0'};

void fetch_row_value() {

	sent_query();
	mysql_result = mysql_store_result(mysql_con);
	mysql_row = mysql_fetch_row(mysql_result);
	mysql_free_result(mysql_result);
	memset(sql_buffer, 0, sizeof(sql_buffer));
}

void sent_query() { mysql_query(mysql_con, sql_buffer); }

int turn_int(int col_num) { return atoi(mysql_row[col_num]); }

float turn_float(int col_num) { return atof(mysql_row[col_num]); }

float turn_value_to_float(int col_num) {
	
	fetch_row_value();
	float result = turn_float(col_num);
	return result;
}

int turn_value_to_int(int col_num) {

	fetch_row_value();
	int result = turn_int(col_num);
	return result;
}

void messagePrint(int lineNum, const char *message, char contentSize, float content, char tabInHeader) {

	// tap 'Y' or 'N' means yes or no
	if (tabInHeader == 'Y')
		printf("\t");
	// tap 'I' or 'F' means int or float, otherwise no contents be showed.
	switch (contentSize)
	{
	case 'I':
		printf("LINE %d: %s%d\n", lineNum, message, (int)content);
		break;
	case 'F':
		printf("LINE %d: %s%f\n", lineNum, message, content);
		break;
	default:
		printf("LINE %d: %s\n", lineNum, message);
	}
	
}

void functionPrint(const char* functionName) {

	printf("\nFunction: %s\n", functionName);
}

int find_variableName_position(vector<string> variableNameArray, string target)
{
	auto it = find(variableNameArray.begin(), variableNameArray.end(), target);

	// If element was found
	if (it != variableNameArray.end())
		return (it - variableNameArray.begin());
	else
		return -1;
}