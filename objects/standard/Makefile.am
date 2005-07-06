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
	arc_icon pixmaps/arc-24x24.png \
	beziergon_icon pixmaps/beziergon-24x24.png \
	bezierline_icon pixmaps/bezierline-24x24.png \
	box_icon pixmaps/box-24x24.png \
	ellipse_icon pixmaps/ellipse-24x24.png \
	image_icon pixmaps/image-24x24.png \
	line_icon pixmaps/line-24x24.png \
	polygon_icon pixmaps/polygon-24x24.png \
	polyline_icon pixmaps/polyline-24x24.png \
	zigzagline_icon pixmaps/zigzagline-24x24.png

ICON_PNGS= \
	pixmaps/arc-24x24.png \
	pixmaps/beziergon-24x24.png \
	pixmaps/bezierline-24x24.png \
	pixmaps/box-24x24.png \
	pixmaps/ellipse-24x24.png \
	pixmaps/image-24x24.png \
	pixmaps/line-24x24.png \
	pixmaps/polygon-24x24.png \
	pixmaps/polyline-24x24.png \
	pixmaps/zigzagline-24x24.png 

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
