## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = 	libnetwork_objects.la
libnetwork_objects_la_SOURCES = \
			      network.h \
			      bus.c \
			      wanlink.c 

##			      computer.c \
##			      disc.c \
##			      monitor.c \
##			      printer.c \
##			      network.c \
##			      antenna.c \
##			      modem.c \
##			      flash.c \
##			      hub.c \
##			      modularswitch.c \
##			      rj45plug.c \
##			      scead-plug.c

libnetwork_objects_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS)

EXTRA_DIST = \
	pixmaps/antenna.xpm \
	pixmaps/bus.xpm \
	pixmaps/computer.xpm \
	pixmaps/disc.xpm \
	pixmaps/flash.xpm \
	pixmaps/wanlink.xpm \
	pixmaps/hub.xpm \
	pixmaps/modem.xpm \
	pixmaps/modularswitch.xpm \
	pixmaps/monitor.xpm \
	pixmaps/printer.xpm \
	pixmaps/rj45plug.xpm \
	pixmaps/sceadplug.xpm
