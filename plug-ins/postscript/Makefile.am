if WITH_FREETYPE
freetype_files = diapsft2renderer.h diapsft2renderer.c
else
freetype_files = 
endif

plugin_sources = \
	postscript.c \
	ps-utf8.c \
	ps-utf8.h \
	diapsrenderer.h \
	diapsrenderer.c \
	$(freetype_files) \
	paginate_psprint.c \
	paginate_psprint.h \
	render_eps.c \
	render_eps.h

pkglib_LTLIBRARIES = libpostscript_filter.la

EXTRA_DIST = \
	win32print.c \
	win32print.h

libpostscript_filter_la_SOURCES = $(plugin_sources)

libpostscript_filter_la_LDFLAGS = -export-dynamic -module -avoid-version

libpostscript_filter_la_LIBADD =
if WITH_FREETYPE
libpostscript_filter_la_LIBADD += $(FREETYPE_LIBS)
endif

INCLUDES = -I$(srcdir)/../../lib $(GTK_CFLAGS) $(DEBUG_FLAGS) $(CAIRO_CFLAGS)
