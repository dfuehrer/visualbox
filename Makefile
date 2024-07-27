include config.mk

CFLAGS += -Wall -Wextra

visualbox: clparser/libparseargs.a libgrapheme/libgrapheme.a

clparser/libparseargs.a: clparser/*.[ch]
	$(MAKE) -C $(@D) $(@F)

libgrapheme/libgrapheme.a:
	./$(@D)/configure
	$(MAKE) -C $(@D) $(@F)

PHONY : clean install

clean :
	rm -f ${OBJ} visualbox
	
install : visualbox
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f visualbox ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/visualbox
