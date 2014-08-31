## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libcgm_filter.la

libcgm_filter_la_SOURCES = cgm.c cgm.h

libcgm_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libcgm_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(DEBUG_CFLAGS) $(GTK_CFLAGS)
