## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libxfig_filter.la

libxfig_filter_la_SOURCES = \
	xfig.c \
	xfig-import.c

libxfig_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS)
