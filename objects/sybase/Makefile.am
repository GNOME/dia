## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

pkglib_LTLIBRARIES = 	libsybase_objects.la
libsybase_objects_la_SOURCES = \
			      sybase.h \
			      dataserver.c \
			      repserver.c \
			      ltm.c \
			      client.c \
			      stableq.c \
			      rsm.c \
			      sybase.c

libsybase_objects_la_LDFLAGS = -export-dynamic -version-info 0:0:0

CPPFLAGS = @GTK_CFLAGS@
INCLUDES = -I$(srcdir)/../../lib
CFLAGS = @CFLAGS@
