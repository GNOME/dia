## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libdb_objects.la
libdb_objects_la_SOURCES = \
	database.c \
	database.h \
	table.c \
	reference.c \
	compound.c

libdb_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libdb_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/table.xpm \
	pixmaps/reference.xpm \
	pixmaps/compound.xpm
