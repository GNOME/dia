## Process this file with automake to produce Makefile.in

## This is, essentially, empty. This will disappear soon.

pkglib_LTLIBRARIES = 	libsybase_objects.la
libsybase_objects_la_SOURCES = \
			      sybase.h \
                              sybase.c

libsybase_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(FREETYPE_CFLAGS)

EXTRA_DIST = \
	pixmaps/client.xpm \
	pixmaps/dataserver.xpm \
	pixmaps/ltm.xpm \
	pixmaps/repserver.xpm \
	pixmaps/rsm.xpm \
	pixmaps/stableq.xpm
