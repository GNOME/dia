SUBDIRS=exports

tests:
	for DIR in $(SUBDIRS); do \
		cd $$DIR && $(MAKE) tests; \
	done

# where do i get GTESTER?
#test: $(TEST_PROGS)
#	@test -z "${TEST_PROGS}" || ${GTESTER} --verbose ${TEST_PROGS}

test: $(TEST_PROGS)
	./objects ../objects//

# rum make test as part of make check
check-local: test

INCLUDES = -g -I$(top_srcdir) -I$(top_srcdir)/lib $(GTK_CFLAGS)

noinst_PROGRAMS = $(TEST_PROGS)
progs_ldadd = $(top_builddir)/lib/libdia.la $(GTK_LIBS)



TEST_PROGS = boundingbox
boundingbox_SOURCES = test-boundingbox.c
boundingbox_LDADD = $(progs_ldadd)

TEST_PROGS += objects
objects_SOURCES = test-objects.c
objects_LDADD = $(progs_ldadd)

