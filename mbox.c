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
	int ret=MSG_STATUS_NEWRCV;
	struct tm time={0};
	unsigned int content_length=0;

	hdr->h_size=0;
	// 跳过空行，检查邮件头部标志是否有效
	unsigned int i=0;
	while(1){
		bzero(buff, sizeof(buff));
		if(feof(fp))
			return -1;

		fgets(buff, sizeof(buff), fp);
		if (strncmp(buff, "From ", 5)==0) {
			hdr->h_size += strlen(buff);
			break;
		}
	}

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
		}else if(!strncmp(buff, "Status: ", 8)){
			header='s';
			p += 8;
		}else if(!strncmp(buff, "Subject: ", 9)){
			header='S';
			p += 9;
		}else if(!strncmp(buff, "Content-Type: ", 14)){
			header='T';
			p+=14;
		}else if(!strncmp(buff, "Content-Length: ", 16)){
			header='C';
			p+=16;
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
			case 'C':
				sscanf(p,"%d", &content_length);
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

			case 's'://status
				if(NULL == strstr(p, "RO"))
					ret = MSG_STATUS_READED;
				else
					ret = MSG_STATUS_UNREAD;
				break;

			default:
				break;
		}
	}

	fpos_t pos;
	fgetpos(fp,&pos);
	//查找邮件的结束位置
	hdr->c_size=0;
	while(!feof(fp)){
		fgets(buff, sizeof(buff), fp);
		hdr->c_size += strlen(buff);

		if (hdr->type!='p') {
			if(!strcmp(buff, hdr->boundary)){
				break;
			}
		} else {
			if(buff[0]=='\r' || buff[0]=='\n')
				break;
		}
	}

	//返回时，应当保持在邮件头末尾处
	fsetpos(fp,&pos);
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
}

