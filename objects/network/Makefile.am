## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

pkglib_LTLIBRARIES = 	libnetwork_objects.la
libnetwork_objects_la_SOURCES = \
			      network.h \
			      computer.c \
			      disc.c \
			      monitor.c \
			      printer.c \
			      bus.c \
			      network.c \
			      antenna.c \
			      modem.c \
			      flash.c \
			      hub.c \
			      modularswitch.c \
			      rj45plug.c \
			      scead-plug.c

libnetwork_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(srcdir)/../../lib @GTK_CFLAGS@ @GNOME_CFLAGS@
