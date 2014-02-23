## Process this file with automake to produce Makefile.in
pkglib_LTLIBRARIES = 	libstandard_objects.la
libstandard_objects_la_SOURCES = \
	arc.c \
	box.c \
	ellipse.c \
	textobj.c \
	line.c \
	zigzagline.c \
	polyline.c \
	bezier.c \
	standard.c \
	image.c \
	outline.c \
	polygon.c \
	beziergon.c \
	tool-icons.h

libstandard_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libstandard_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

ICON_PNG_PAIRS= \
	arc_icon $(srcdir)/pixmaps/arc.png \
	beziergon_icon $(srcdir)/pixmaps/beziergon.png \
	bezierline_icon $(srcdir)/pixmaps/bezierline.png \
	box_icon $(srcdir)/pixmaps/box.png \
	ellipse_icon $(srcdir)/pixmaps/ellipse.png \
	image_icon $(srcdir)/pixmaps/image.png \
	line_icon $(srcdir)/pixmaps/line.png \
	outline_icon $(srcdir)/pixmaps/outline.png \
	polygon_icon $(srcdir)/pixmaps/polygon.png \
	polyline_icon $(srcdir)/pixmaps/polyline.png \
	text_icon $(srcdir)/pixmaps/text.png \
	zigzagline_icon $(srcdir)/pixmaps/zigzagline.png

ICON_PNGS= \
	pixmaps/arc.png \
	pixmaps/beziergon.png \
	pixmaps/bezierline.png \
	pixmaps/box.png \
	pixmaps/ellipse.png \
	pixmaps/image.png \
	pixmaps/line.png \
	pixmaps/outline.png \
	pixmaps/polygon.png \
	pixmaps/polyline.png \
	pixmaps/text.png \
	pixmaps/zigzagline.png 

tool-icons.h: $(ICON_PNGS)
	gdk-pixbuf-csource --raw --build-list $(ICON_PNG_PAIRS) > tool-icons.h

BUILT_SOURCES = tool-icons.h

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	$(ICON_PNGS)
