## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libcustom_objects.la
libcustom_objects_la_SOURCES = \
	shape_info.c \
	shape_info.h \
	custom_object.c \
	custom_util.c \
	custom_util.h \
	custom.c

libcustom_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

shapedir = $(pkgdatadir)/shapes

# add -DDEBUG_CUSTOM here to get extra debugging info.
INCLUDES = -I$(top_srcdir)/intl -I$(top_srcdir)/lib $(GTK_CFLAGS) \
	$(GNOME_CFLAGS) $(XML_CFLAGS) -DDIA_SHAPEDIR=\""$(shapedir)"\"

EXTRA_DIST = \
	pixmaps/custom.xpm