## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

pkglib_LTLIBRARIES = 	libuml_objects.la
libuml_objects_la_SOURCES = \
			uml.h \
			uml.c \
			class.c \
			class.h \
			class_dialog.c \
			note.c \
			realizes.c \
			constraint.c \
			small_package.c \
			large_package.c \
			implements.c \
			generalization.c \
			association.c \
			dependency.c

libuml_objects_la_LDFLAGS = -export-dynamic -version-info 0:0:0

CPPFLAGS = @GTK_CFLAGS@
INCLUDES = -I$(srcdir)/../../lib
CFLAGS = -Wall -g
