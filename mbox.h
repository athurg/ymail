#ifndef __MBOX_H__
#define __MBOX_H__

#include <stdlib.h>
#include <stdio.h>

#define MSG_STATUS_READED	1
#define MSG_STATUS_UNREAD	2
#define MSG_STATUS_NEWRCV	3

struct mail_hdr{
	char * sender;
	char * email;
	char * subject;
	char * time;
	char * boundary;
	char * filename;
	unsigned int type;//'r'=>相关型（有附件，且附件与HTML正文相关）、'm'=>混合型（有附件）、'a'=>可选型（HTML和纯文本）
	size_t c_size;
	size_t h_size;
};

char parse_header_iterm(char *line, char **rst);
char * decode(char *str);
int parse_header(FILE *fp, struct mail_hdr * hdr);
void free_mail_hdr(struct mail_hdr *);

#endif
