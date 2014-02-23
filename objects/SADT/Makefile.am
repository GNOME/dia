## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libsadt_objects.la
libsadt_objects_la_SOURCES = \
			sadt.h \
			sadt.c \
			arrow.c \
			box.c \
			annotation.c

libsadt_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libsadt_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/arrow.xpm \
	pixmaps/sadtbox.xpm \
	pixmaps/annotation.xpm
