## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libflowchart_objects.la
libflowchart_objects_la_SOURCES = \
			box.c \
			parallelogram.c \
			diamond.c \
			ellipse.c \
			flowchart.c

libflowchart_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libflowchart_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/box.xpm \
	pixmaps/diamond.xpm \
	pixmaps/ellipse.xpm \
	pixmaps/pgram.xpm 
