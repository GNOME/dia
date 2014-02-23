## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libcustom_lines_objects.la
libcustom_lines_objects_la_SOURCES = \
	custom_lines.c \
	custom_linetypes.c \
	custom_linetypes.h \
	line_info.c \
	line_info.h

libcustom_lines_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libcustom_lines_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = \
	-I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/default.xpm 
