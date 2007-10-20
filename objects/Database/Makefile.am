## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libdb_objects.la
libdb_objects_la_SOURCES = \
			database.c \
			table.c \
			table_dialog.c \
			reference.c \
			compound.c

libdb_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/table.xpm \
	pixmaps/reference.xpm \
	pixmaps/compound.xpm
