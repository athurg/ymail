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
	fpos_t start_pos;
};

int parse_header(FILE *fp, struct mail_hdr * hdr);
void free_mail_hdr(struct mail_hdr *);

#endif
