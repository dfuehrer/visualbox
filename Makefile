include config.mk

CFLAGS += -Wall -Wextra

visualbox: clparser/libparseargs.a

clparser/libparseargs.a: clparser/*.[ch]
	$(MAKE) -C clparser $(@F)

PHONY : clean install

clean :
	rm -f ${OBJ} visualbox
	
install : visualbox
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f visualbox ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/visualbox
