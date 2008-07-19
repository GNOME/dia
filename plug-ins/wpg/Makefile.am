pkglib_LTLIBRARIES = libwpg_filter.la

libwpg_filter_la_SOURCES = wpg.c wpg_defs.h
libwpg_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(top_srcdir)/lib \
  $(DEBUG_FLAGS) $(GTK_CFLAGS)
