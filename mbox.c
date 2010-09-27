#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "charset.h"
#include "mbox.h"
#include "misc.h"
#include <glib.h>


int parse_header(FILE * fp, struct mail_hdr * hdr)
{
	char name[100]={0}, addr[100]={0}, subject[200]={0};
	char buff[100];
	char header=0;
	int ret=0;
	struct tm time={0};

	if(feof(fp))
		return -1;

	hdr->h_size=0;
	while(!feof(fp)){
		bzero(buff, sizeof(buff));
		fgets(buff, sizeof(buff), fp);
		hdr->h_size += strlen(buff);

		char *p = buff;

		if(buff[0]=='\n' || buff[0]=='\r')
			break;
		else if(buff[0]=='\t' || buff[0]==' '){
			//有些字段可能包含多个行，续行的时候会以TAB或者空格开头
			header = header;
			p += 1;
		}else if(!strncmp(buff, "From: ", 6)){
			header='F';
			p += 6;
		}else if(!strncmp(buff, "Date: ", 6)){
			header='D';
			p += 6;
		}else if(!strncmp(buff, "Subject: ", 9)){
			header='S';
			p += 9;
		}else if(!strncmp(buff, "Content-Type: ", 14)){
			header='T';
			p+=14;
		}else{
			header=0;
		}

		str_replace(buff, '\r', ' ');
		str_replace(buff, '\n', ' ');
		str_replace(buff, '"', ' ');
		//跳过行首的空格
		while(*p==' ')	p++;

		switch(header){
			case 'F':
				str_replace(p, '<', ' ');
				str_replace(p, '>', ' ');

				if(NULL == strstr(p, "=?")){
					sscanf(p, "%s %s", name, addr);
				} else {
					char charset[10]={0}, email[100]={0};
					int len=0;

					str_replace(p, '?', ' ');
					sscanf(p, "= %s B %s = %s", &charset, name, addr);

					g_base64_decode_inplace(name, &len);
					name[len]='\0';

					// 如果不是utf-8编码，就需要转下
					if (strcasecmp(charset,"utf-8")) {
						convert_to_utf8(charset, name, name);
					}
				}
				break;

			case 'S':
				if(NULL == strstr(p, "=?")){
					strcpy(subject, p);
				} else {
					char charset[10]={0}, email[100]={0}, subject_tmp[400]={0};
					int len=0;


					str_replace(p, '?', ' ');
					sscanf(p, "= %s B %s", &charset, subject_tmp);

					g_base64_decode_inplace(subject_tmp, &len);
					subject_tmp[len]='\0';
					convert_to_utf8(charset, subject_tmp, subject+strlen(subject));
				}
				break;

			case 'D':
				strtime_to_tm(p, &time);
				hdr->time = malloc(20);
				sprintf(hdr->time,"%04d-%02d-%02d %02d:%02d:%02d",
						time.tm_year,
						time.tm_mon+1,
						time.tm_mday+1,
						time.tm_hour+1,
						time.tm_min+1,
						time.tm_sec+1);
				break;

			case 'T'://邮件（附件）类型处理
				if(!strcmp(p, "text/plain; "))
					hdr->type = 'p';
				else if(!strcmp(p, "multipart/alternative; "))
					hdr->type = 'a';
				else if(!strcmp(p, "multipart/mixed; "))
					hdr->type = 'm';
				else if(!strcmp(p, "multipart/related; "))
					hdr->type = 'r';
				else if(!strncmp(p, "boundary= ",10)){
					char bdry_tmp[50]={0};
					bdry_tmp[0]='-';
					bdry_tmp[1]='-';
					sscanf(p,"boundary= %s", bdry_tmp+2);
					strcat(bdry_tmp, "--\n");
					hdr->boundary = strdup(bdry_tmp);
				}
				break;

			default:
				break;
		}
	}

	hdr->sender  = strdup(name);
	hdr->email   = strdup(addr);
	hdr->subject = strdup(subject);

	return ret;
}

void free_mail_hdr(struct mail_hdr * hdr_p)
{
	free(hdr_p->sender);
	free(hdr_p->email);
	free(hdr_p->subject);
	free(hdr_p->time);
	free(hdr_p->boundary);
	free(hdr_p->filename);
}


char * decode(char *str)
{
	int len;
	char utf8_buff[1024]={0};
	char *p, *encode_type, *encode_str, *charset;

	p = strdup(str);

	charset = index(p, '?') + 1;
	encode_type = index(charset, '?') + 1;
	encode_str  = index(encode_type,'?') + 1;

	str_replace(p, '?', '\0');

	if (encode_type[0]=='B') {//BASE64
		g_base64_decode_inplace(encode_str, &len);
		encode_str[len]='\0';
	}

	if (0 == strcmp(charset, "gb2312")) {
		convert_to_utf8(charset, encode_str, utf8_buff);
	} else {

	}
	free(p);

	return strdup(utf8_buff);
}

