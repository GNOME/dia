pkglib_LTLIBRARIES = libhpgl_filter.la

libhpgl_filter_la_SOURCES = hpgl.c
libhpgl_filter_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)

libhpgl_filter_la_LIBADD = $(top_builddir)/lib/libdia.la

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(top_srcdir)/lib $(DEBUG_FLAGS) $(GTK_CFLAGS)
