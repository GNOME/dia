## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

pkglib_LTLIBRARIES = 	libfs_objects.la
libfs_objects_la_SOURCES = \
			fs.c \
			flow.c \
			flow-ortho.c \
			function.c

libfs_objects_la_LDFLAGS = -export-dynamic -module -avoid-version
libfs_objects_la_LIBADD = @GTK_LIBS@

INCLUDES = -I$(srcdir)/../../lib @CFLAGS@ @GTK_CFLAGS@ @GNOME_CFLAGS@
