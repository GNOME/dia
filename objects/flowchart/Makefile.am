## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libflowchart_objects.la
libflowchart_objects_la_SOURCES = \
			box.c \
			parallelogram.c \
			diamond.c \
			ellipse.c \
			flowchart.c

libflowchart_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) \
	$(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/box.xpm \
	pixmaps/diamond.xpm \
	pixmaps/ellipse.xpm \
	pixmaps/pgram.xpm 
