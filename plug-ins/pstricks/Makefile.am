## Process this file with automake to produce Makefile.in

pkglib_LTLIBRARIES = libpstricks_filter.la

libpstricks_filter_la_SOURCES = \
	pstricks.c \
	render_pstricks.c \
	render_pstricks.h

libpstricks_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libpstricks_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(srcdir)/../../lib $(DEBUG_FLAGS) $(GTK_CFLAGS)
