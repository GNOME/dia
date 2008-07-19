## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libmisc_objects.la
libmisc_objects_la_SOURCES = \
			libmisc.c \
			analog_clock.c \
			measure.c \
			tree.c


libmisc_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/analog_clock.xpm \
	pixmaps/measure.xpm \
	pixmaps/newgroup.xpm \
	pixmaps/tree.xpm



