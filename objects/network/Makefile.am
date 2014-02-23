## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libnetwork_objects.la
libnetwork_objects_la_SOURCES = \
			      network.h \
			      network.c \
			      bus.c \
			      radiocell.c \
			      basestation.c \
			      wanlink.c

libnetwork_objects_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libnetwork_objects_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
	$(DEBUG_FLAGS) $(GTK_CFLAGS)

EXTRA_DIST = \
	pixmaps/antenna.xpm \
	pixmaps/bus.xpm \
	pixmaps/computer.xpm \
	pixmaps/disc.xpm \
	pixmaps/flash.xpm \
	pixmaps/radiocell.xpm \
	pixmaps/basestation.xpm \
	pixmaps/wanlink.xpm \
	pixmaps/hub.xpm \
	pixmaps/modem.xpm \
	pixmaps/modularswitch.xpm \
	pixmaps/monitor.xpm \
	pixmaps/printer.xpm \
	pixmaps/rj45plug.xpm \
	pixmaps/sceadplug.xpm
