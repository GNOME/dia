## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libflowchart_objects.la
libflowchart_objects_la_SOURCES = \
			box.c \
			parallelogram.c \
			diamond.c \
			ellipse.c \
			flowchart.c

libflowchart_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib @CFLAGS@ @GTK_CFLAGS@ \
	@GNOME_CFLAGS@

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
	terminal.shape \
	magdisk.shape \
	magtape.shape \
	intstorage.shape \
	merge.shape \
	extract.shape \
	delay.shape \
	sumjunction.shape \
	collate.shape \
	sort.shape \
	or.shape \
	magdrum.shape \
	offlinestore.shape \
	punchedcard.shape \
	punchedtape.shape \
	transmittape.shape

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
	pixmaps/terminal.xpm \
	pixmaps/magdisk.xpm \
	pixmaps/magtape.xpm \
	pixmaps/intstorage.xpm \
	pixmaps/merge.xpm \
	pixmaps/extract.xpm \
	pixmaps/delay.xpm \
	pixmaps/sumjunction.xpm \
	pixmaps/collate.xpm \
	pixmaps/sort.xpm \
	pixmaps/or.xpm \
	pixmaps/magdrum.xpm \
	pixmaps/offlinestore.xpm \
	pixmaps/punchedcard.xpm \
	pixmaps/punchedtape.xpm \
	pixmaps/transmittape.xpm
