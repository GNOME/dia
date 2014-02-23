## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libmetapost_filter.la

libmetapost_filter_la_SOURCES = \
	metapost.c \
	render_metapost.c \
	render_metapost.h

libmetapost_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libmetapost_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib  $(DEBUG_FLAGS) $(GTK_CFLAGS)
