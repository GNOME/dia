## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libstandard_objects.la
libstandard_objects_la_SOURCES = 	arc.c \
			box.c \
			ellipse.c \
			textobj.c \
			line.c \
			zigzagline.c \
			polyline.c \
			bezier.c \
			standard.c \
			image.c \
			polygon.c

libstandard_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib @GTK_CFLAGS@ @GNOME_CFLAGS@

EXTRA_DIST = \
	pixmaps/arc.xpm \
	pixmaps/box.xpm \
	pixmaps/ellipse.xpm \
	pixmaps/image.xpm \
	pixmaps/line.xpm \
	pixmaps/polyline.xpm \
	pixmaps/bezier.xpm \
	pixmaps/text.xpm \
	pixmaps/zigzag.xpm \
	pixmaps/polygon.xpm
