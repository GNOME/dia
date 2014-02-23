## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libfs_objects.la
libfs_objects_la_SOURCES = \
			fs.c \
			flow.c \
			flow-ortho.c \
			function.c

libfs_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libfs_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/flow.xpm \
	pixmaps/function.xpm \
	pixmaps/orthflow.xpm
