pkglib_LTLIBRARIES = libhpgl_filter.la

libhpgl_filter_la_SOURCES = hpgl.c
libhpgl_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(top_srcdir)/lib \
  $(GTK_CFLAGS) $(GNOME_CFLAGS) $(FREETYPE_CFLAGS)
