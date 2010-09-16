#ifndef __MBOX_H__
#define __MBOX_H__

#include <stdlib.h>
#include <stdio.h>

#define MSG_STATUS_READED	1
#define MSG_STATUS_UNREAD	2
#define MSG_STATUS_NEWRCV	3

int parse_header(FILE *fp, char * name, char *addr, char *subject, char *date);

#endif
