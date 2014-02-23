plugin_sources = \
	dia-graph.h \
	dia-graph.cpp \
	layout.cpp \
	ogdf-simple.h

if HAVE_OGDF
plugin_sources += ogdf-simple.cpp
endif

# Can't do without C++
if HAVE_CPLUSPLUS
pkglib_LTLIBRARIES = liblayout_filter.la
endif

liblayout_filter_la_SOURCES = $(plugin_sources)
liblayout_filter_la_LDFLAGS =  -pthread -export-dynamic -module -avoid-version $(NO_UNDEFINED)
liblayout_filter_la_LIBADD = $(OGDF_LIBS)  $(top_builddir)/lib/libdia.la
if OS_WIN32
liblayout_filter_la_LIBADD += -lpsapi
endif

AM_CPPFLAGS = -I$(top_srcdir)/intl -I$(top_srcdir)/lib -I$(top_srcdir)/app \
  $(DEBUG_FLAGS) $(GTK_CFLAGS) $(OGDF_CFLAGS)

EXTRA_DIST = \
	ogdf-simple.cpp \
	ogdf-simple.h \
	makefile.msc \
	readme.txt

if !HAVE_OGDF
# Prevent installation
install-pkglibLTLIBRARIES:
	:
endif

