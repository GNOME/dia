## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libmetapost_filter.la

libmetapost_filter_la_SOURCES = \
	metapost.c \
	render_metapost.c \
	render_metapost.h

libmetapost_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib \
 $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(DEBUG_CFLAGS) \
 $(XML_CFLAGS) $(UNICODE_CFLAGS)
