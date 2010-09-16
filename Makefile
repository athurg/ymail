
CFLAGS = `pkg-config --cflags --libs gtk+-2.0` -std=gnu99 -export-dynamic
OBJS= charset.o mbox.o misc.o mailer.o

all:${OBJS}
	$(CC) ${CFLAGS} ${OBJS} -o test

clean:
	-@rm *.o

distclean:clean
	-@rm test

rebuild:clean all
