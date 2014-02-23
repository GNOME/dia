## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libchronogram_objects.la
libchronogram_objects_la_SOURCES = \
			chronogram.h \
			chronogram.c \
			chronoref.c \
			chronoline.c \
			chronoline_event.h \
			chronoline_event.c


libchronogram_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libchronogram_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/chronoref.xpm \
	pixmaps/chronoline.xpm


