#ifndef	__CHARSET_H__
#define	__CHARSET_H__

#define convert_to_utf8_inplace(s_charset, str)	\
	convert_to_utf8(s_charset, str, str)
int convert_to_utf8(char * s_charset, char *src, char *dest);

#endif
