## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

pkglib_LTLIBRARIES = 	liber_objects.la
liber_objects_la_SOURCES = \
			entity.c \
			relationship.c \
			attribute.c \
			participation.c \
			er.c

liber_objects_la_LDFLAGS = -export-dynamic -version-info 0:0:0
liber_objects_la_LIBADD = @GTK_LIBS@

CPPFLAGS = @GTK_CFLAGS@
INCLUDES = -I$(srcdir)/../../lib
CFLAGS = @CFLAGS@
