## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

pkglib_LTLIBRARIES = 	libfs_objects.la
libfs_objects_la_SOURCES = \
			fs.h \
			fs.c \
			flow.c \
			flow-ortho.c \
			function.c

libfs_objects_la_LDFLAGS = -export-dynamic -version-info 0:0:0
libfs_objects_la_LIBADD = @GTK_LIBS@

CPPFLAGS = @GTK_CFLAGS@
INCLUDES = -I$(srcdir)/../../lib
CFLAGS = @CFLAGS@
