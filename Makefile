include config.mk

CFLAGS += -Wall -Wextra

visualbox: clparser/libparseargs.a

# only make visualbox depend on libgrapheme if not told not to
ifeq ($(NO_LIBGRAPHEME),0)
visualbox: libgrapheme/libgrapheme.a
else
visualbox: CFLAGS += -DNO_LIBGRAPHEME
endif

clparser/libparseargs.a: clparser/*.[ch]
	$(MAKE) -C $(@D) $(@F)

libgrapheme/libgrapheme.a:
	./$(@D)/configure
	$(MAKE) -C $(@D) $(@F)

PHONY : clean install

clean :
	rm -f ${OBJ} visualbox
	$(MAKE) -C libgrapheme clean
	$(MAKE) -C clparser clean
	
install : visualbox
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f visualbox ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/visualbox
