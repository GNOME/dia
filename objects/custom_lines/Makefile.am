## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libcustom_lines_objects.la
libcustom_lines_objects_la_SOURCES = \
	custom_lines.c \
	custom_linetypes.c \
	custom_linetypes.h \
	line_info.c \
	line_info.h

libcustom_lines_objects_la_LDFLAGS = -export-dynamic -module -avoid-version
libcustom_lines_objects_la_LIBADD = ../standard/libstandard_objects.la

# including objects/standard is plain wrong but needed for 'make distcheck'!
# the second include is to trick it to work with srcdir!=buildir, at least I think so;)
INCLUDES = \
	-I$(srcdir)/../../lib \
	-I$(srcdir)/../../objects/standard \
	-I$(srcdir)/../../../objects/standard \
	$(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/default.xpm 
