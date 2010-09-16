#include <string.h>
#include <stdio.h>
#include <time.h>

#include "misc.h"

int str_replace(char *str, char orig, char new)
{
	int i;

	i = strlen(str) + 1;

	while(i--){
		if(str[i]==orig)
			str[i]=new;
	}
}

int strtime_to_tm(char * str, struct tm * time)
{
	char str_week[10]={0}, str_month[10]={0};

	sscanf(str, "%s %d %s %d %d:%d:%d",
			&str_week, &time->tm_mday, &str_month, &time->tm_year,
			&time->tm_hour, &time->tm_min, &time->tm_sec);

	if(!strcmp(str_week,"Mon,"))
		time->tm_wday=0;
	else if(!strcmp(str_week,"Tue,"))
		time->tm_wday=1;
	else if(!strcmp(str_week,"Wed,"))
		time->tm_wday=2;
	else if(!strcmp(str_week,"Thu,"))
		time->tm_wday=3;
	else if(!strcmp(str_week,"Fri,"))
		time->tm_wday=4;
	else if(!strcmp(str_week,"Sat,"))
		time->tm_wday=5;
	else if(!strcmp(str_week,"Sun,"))
		time->tm_wday=6;

	if(!strcmp(str_month,"Jan"))
		time->tm_mon=0;
	else if(!strcmp(str_month,"Feb"))
		time->tm_mon=1;
	else if(!strcmp(str_month,"Mar"))
		time->tm_mon=2;
	else if(!strcmp(str_month,"Apr"))
		time->tm_mon=3;
	else if(!strcmp(str_month,"May"))
		time->tm_mon=4;
	else if(!strcmp(str_month,"Jun"))
		time->tm_mon=5;
	else if(!strcmp(str_month,"Jul"))
		time->tm_mon=6;
	else if(!strcmp(str_month,"Aug"))
		time->tm_mon=7;
	else if(!strcmp(str_month,"Sep"))
		time->tm_mon=8;
	else if(!strcmp(str_month,"Oct"))
		time->tm_mon=9;
	else if(!strcmp(str_month,"Nov"))
		time->tm_mon=10;
	else if(!strcmp(str_month,"Dec"))
		time->tm_mon=11;

}
