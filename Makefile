PROG=ymail

CC := gcc
CFLAGS = `pkg-config --cflags --libs gtk+-2.0` -std=gnu99 -export-dynamic #--shared
SRCS=$(wildcard *.c)
OBJS=${patsubst %.c,%.o,${SRCS}}

all:${OBJS}
	$(CC) ${CFLAGS} ${OBJS} -o ${PROG}

clean:
	-@rm *.o

distclean:clean
	-@rm ${PROG}

rebuild:clean all
