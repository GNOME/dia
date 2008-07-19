## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libcgm_filter.la

libcgm_filter_la_SOURCES = cgm.c

libcgm_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(DEBUG_CFLAGS) $(GTK_CFLAGS)
