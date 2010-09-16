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

	// 跳过空行，检查邮件头部标志是否有效
	while(1){
		if (fgets(buff, sizeof(buff), fp) == NULL) {
			return -1;	//End of the file
		} else if (strncmp(buff, "From ", 5)==0) {
			break;
		}
	}

	fgetpos(fp, &hdr->start_pos);
	while(NULL!=fgets(buff, sizeof(buff), fp)){
		char *p = buff;

		if(*p=='\n' || *p=='\r')
			break;
		else if(buff[0]=='\t'){
			//有些字段可能包含多个行，续行的时候会以\t开头
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
		}else if(!strncmp(buff, "Content-Length: ", 16)){
			header='C';
			p+=16;
		}else{
			header=0;
		}

		str_replace(buff, '\r', ' ');
		str_replace(buff, '\n', ' ');
		str_replace(buff, '"', ' ');
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
					convert_to_utf8(charset, name, name);
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
						time.tm_mon,
						time.tm_mday,
						time.tm_hour,
						time.tm_min,
						time.tm_sec);
				break;
			case 'C':
				sscanf(p,"%d", &content_length);
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

	// jump to the next message
	fseek(fp, content_length, SEEK_CUR);

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
}

