## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libpixbuf_filter.la

libpixbuf_filter_la_SOURCES = \
	pixbuf.c

libpixbuf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(GTK_CFLAGS) $(GNOME_CFLAGS) $(DEBUG_FLAGS)
