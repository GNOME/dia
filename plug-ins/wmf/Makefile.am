pkglib_LTLIBRARIES = libwmf_filter.la

libwmf_filter_la_SOURCES = wmf.cpp wmf_gdi.cpp wmf_gdi.h
libwmf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

INCLUDES = -I$(top_srcdir)/intl -I$(top_srcdir)/lib \
  $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS) \
  $(DEBUG_CFLAGS)

EXTRA_DIST = 

