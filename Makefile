PROG=ymail

MAKEFLAGS += --silent

CC := gcc
CFLAGS = `pkg-config --cflags --libs libnotify` -std=gnu99 -export-dynamic
SRCS=$(wildcard *.c)
OBJS=${patsubst %.c,%.o,${SRCS}}

all:${OBJS}
	$(CC) ${CFLAGS} ${OBJS} -o ${PROG}

clean:
	-@rm *.o

distclean:clean
	-@rm ${PROG}

rebuild:clean all
