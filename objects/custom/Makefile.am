## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

pkglib_LTLIBRARIES = 	libcustom_objects.la
libcustom_objects_la_SOURCES = \
			shape_info.c \
			shape_info.h \
			custom_object.c \
			custom.c

libcustom_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(srcdir)/../../lib $(CFLAGS) $(GTK_CFLAGS) $(GNOME_CFLAGS) $(XML_CFLAGS)
