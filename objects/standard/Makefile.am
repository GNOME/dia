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
			polygon.c \
			beziergon.c \
			tool-icons.h

libstandard_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

ICON_PNG_PAIRS= \
	arc_icon pixmaps/arc.png \
	beziergon_icon pixmaps/beziergon.png \
	bezierline_icon pixmaps/bezierline.png \
	box_icon pixmaps/box.png \
	ellipse_icon pixmaps/ellipse.png \
	image_icon pixmaps/image.png \
	line_icon pixmaps/line.png \
	polygon_icon pixmaps/polygon.png \
	polyline_icon pixmaps/polyline.png \
	text_icon pixmaps/text.png \
	zigzagline_icon pixmaps/zigzagline.png

ICON_PNGS= \
	pixmaps/arc.png \
	pixmaps/beziergon.png \
	pixmaps/bezierline.png \
	pixmaps/box.png \
	pixmaps/ellipse.png \
	pixmaps/image.png \
	pixmaps/line.png \
	pixmaps/polygon.png \
	pixmaps/polyline.png \
	pixmaps/text.png \
	pixmaps/zigzagline.png 

tool-icons.h: $(ICON_PNGS)
	gdk-pixbuf-csource --raw --build-list $(ICON_PNG_PAIRS) > tool-icons.h

BUILT_SOURCES = tool-icons.h

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

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
	pixmaps/polygon.xpm \
	pixmaps/beziergon.xpm
