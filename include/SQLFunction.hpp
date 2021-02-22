#ifndef SQLFunction_H
#define SQLFunction_H

#include <string>
#include <vector>
using namespace std;

extern MYSQL *mysql_con;
extern MYSQL_RES *mysql_result;
extern MYSQL_ROW mysql_row;
extern char sql_buffer[2000];
extern vector<string> variable_name;

void fetch_row_value();
void sent_query();
int turn_int(int col_num);
float turn_float(int col_num);
int turn_value_to_int(int col_num);
float turn_value_to_float(int col_num);
void free_result();
void messagePrint(int lineNum, const char *message, char contentSize = 'S', float content = 0,  char tabInHeader = 'N');
void functionPrint(const char* functionName);
int find_variableName_position(vector<string> variableNameArray, string target);

void *new2d(int, int, int);
#define NEW2D(H, W, TYPE) (TYPE **)new2d(H, W, sizeof(TYPE))
#endif 