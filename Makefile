include config.mk

SRC = visualbox.c
OBJ = ${SRC:.c=.o}

visualbox : ${OBJ}
	$(CC) -o $@ ${OBJ}

.c.o:
	${CC} -c ${CFLAGS} $<

PHONY : clean install

clean :
	rm -f ${OBJ} visualbox
	
install : visualbox
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f visualbox ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/visualbox
