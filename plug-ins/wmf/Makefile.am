plugin_sources = wmf.cpp 

if !WITH_LIBEMF
plugin_sources += wmf_gdi.cpp wmf_gdi.h
endif

pkglib_LTLIBRARIES = libwmf_filter.la

libwmf_filter_la_SOURCES = $(plugin_sources)
libwmf_filter_la_LDFLAGS = -export-dynamic -module -avoid-version 
libwmf_filter_la_LIBADD = -lstdc++ $(LIBEMF_LIBS)

INCLUDES = -I$(top_srcdir)/intl -I$(top_srcdir)/lib \
  $(GTK_CFLAGS) $(GNOME_CFLAGS) $(PANGOFT2_CFLAGS) $(UNICODE_CFLAGS) \
  $(DEBUG_CFLAGS) $(LIBEMF_CFLAGS)

EXTRA_DIST = \
	paginate_gdiprint.cpp \
	paginate_gdiprint.h \
	wmf_gdi.cpp \
	wmf_gdi.h \
	libemf-todo.txt

# Prevent installation
install-pkglibLTLIBRARIES:
	:
