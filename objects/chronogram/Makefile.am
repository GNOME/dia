## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libchronogram_objects.la
libchronogram_objects_la_SOURCES = \
			chronogram.h \
			chronogram.c \
			chronoref.c \
			chronoline.c \
			chronoline_event.h \
			chronoline_event.c


libchronogram_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS)

EXTRA_DIST = \
	pixmaps/chronoref.xpm \
	pixmaps/chronoline.xpm


