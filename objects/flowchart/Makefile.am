## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libflowchart_objects.la
libflowchart_objects_la_SOURCES = \
			box.c \
			parallelogram.c \
			diamond.c \
			ellipse.c \
			flowchart.c

libflowchart_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(srcdir)/../../lib @CFLAGS@ @GTK_CFLAGS@ @GNOME_CFLAGS@ \
  -DSHAPE_DIR=\""$(shapedir)"\"

shapedir = $(pkgdatadir)/shape-internal/flowchart
shape_DATA = \
	display.shape \
	document.shape \
	transaction.shape \
	offpageconn.shape \
	manualop.shape \
	preparation.shape \
	manualinput.shape \
	predefdproc.shape \
	terminal.shape

EXTRA_DIST = \
	$(shape_DATA) \
	pixmaps/box.xpm \
	pixmaps/diamond.xpm \
	pixmaps/ellipse.xpm \
	pixmaps/pgram.xpm \
	pixmaps/display.xpm \
	pixmaps/document.xpm \
	pixmaps/transaction.xpm \
	pixmaps/offpageconn.xpm \
	pixmaps/manualop.xpm \
	pixmaps/preparation.xpm \
	pixmaps/manualinput.xpm \
	pixmaps/predefdproc.xpm \
	pixmaps/terminal.xpm
