## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = 	libcustom_objects.a
libcustom_objects_a_SOURCES = \
			shape_info.c \
			shape_info.h \
			custom_object.c \
			custom_util.c \
			custom_util.h \
			load_sheet.c \
			load_sheet.h \
			custom.c

shapedir = $(pkgdatadir)/shapes

# add -DDEBUG_CUSTOM here to get extra debugging info.
INCLUDES = -I$(top_srcdir)/lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(XML_CFLAGS) \
	-DDIA_SHAPEDIR=\""$(shapedir)"\"

EXTRA_DIST = \
	pixmaps/custom.xpm