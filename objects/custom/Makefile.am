## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libcustom_objects.la
libcustom_objects_la_SOURCES = \
	shape_info.c \
	shape_info.h \
	shape_typeinfo.c \
	custom_object.c \
	custom_object.h \
	custom_util.c \
	custom_util.h \
	custom.c

libcustom_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libcustom_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

shapedir = $(pkgdatadir)/shapes

# add -DDEBUG_CUSTOM here to get extra debugging info.
AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(top_srcdir)/lib $(DEBUG_FLAGS) $(GTK_CFLAGS) \
	$(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(XML_CFLAGS) $(UNICODE_CFLAGS) \
	-DDIA_SHAPEDIR=\""$(shapedir)"\"

EXTRA_DIST = \
	pixmaps/custom.xpm
