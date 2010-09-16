#include <stdio.h>
#include <stdlib.h>
#include <iconv.h>
#include <string.h>
#include <malloc.h>
#include "charset.h"

int convert_to_utf8(char * s_charset, char *src, char *dest)
{
	iconv_t cd;
	unsigned int src_len=0, dest_len=0;
	int ret;

	//utf8使用1～3个字节表示一个字符
	//GBK使用2个字节表示一个汉字
	//因此转换后的大小最多是原字串的3倍
	src_len = strlen(src);
	dest_len = src_len*3;

	cd = iconv_open("utf-8", s_charset);

	if(iconv(cd, &src, &src_len, &dest, &dest_len) == -1)
		perror("Error iconv");

	iconv_close(cd);

	dest[0]='\0';

	return ret;
}

