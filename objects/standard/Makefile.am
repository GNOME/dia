## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

pkglib_LTLIBRARIES = 	libstandard_objects.la
libstandard_objects_la_SOURCES = 	arc.c \
			box.c \
			ellipse.c \
			textobj.c \
			line.c \
			zigzagline.c \
			polyline.c \
			standard.c \
			image.c 

libstandard_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(srcdir)/../../lib @GTK_CFLAGS@ @GNOME_CFLAGS@
