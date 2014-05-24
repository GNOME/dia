## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libmisc_objects.la
libmisc_objects_la_SOURCES = \
			libmisc.c \
			analog_clock.c \
			diagram_as_object.c \
			grid_object.c \
			measure.c \
			n_gon.c \
			tree.c


libmisc_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libmisc_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/analog_clock.xpm \
	pixmaps/diagram_as_element.xpm \
	pixmaps/grid_object.xpm \
	pixmaps/measure.xpm \
	pixmaps/n_gon.xpm \
	pixmaps/tree.xpm



