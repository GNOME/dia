AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/intl \
	-I$(top_srcdir)/lib \
	$(DEBUG_FLAGS) \
	$(GTK_CFLAGS) \
	$(LIBART_CFLAGS) \
	$(PYTHON_INCLUDES) \
	$(XML_CFLAGS) $(UNICODE_CFLAGS) 

if WITH_PYTHON
libpython_plugin = libpython_plugin.la
endif

pkglib_LTLIBRARIES = $(libpython_plugin)

libpython_plugin_la_LDFLAGS = -export-dynamic -module -avoid-version $(NO_UNDEFINED)
libpython_plugin_la_LIBADD = $(PYTHON_LIBS) $(top_builddir)/lib/libdia.la
libpython_plugin_la_SOURCES = \
	pydia-color.c \
	pydia-color.h \
	pydia-cpoint.c \
	pydia-cpoint.h \
	pydia-diagram.c \
	pydia-diagram.h \
	pydia-diagramdata.c \
	pydia-diagramdata.h \
	pydia-display.c \
	pydia-display.h \
	pydia-error.c \
	pydia-error.h \
	pydia-export.c \
	pydia-export.h \
	pydia-font.c \
	pydia-font.h \
	pydia-geometry.c \
	pydia-geometry.h \
	pydia-handle.c \
	pydia-handle.h \
	pydia-image.c \
	pydia-image.h \
	pydia-layer.c \
	pydia-layer.h \
	pydia-menuitem.c \
	pydia-menuitem.h \
	pydia-object.c \
	pydia-object.h \
	pydia-paperinfo.c \
	pydia-paperinfo.h \
	pydia-properties.c \
	pydia-properties.h \
	pydia-property.c \
	pydia-render.c \
	pydia-sheet.c \
	pydia-sheet.h \
	pydia-text.c \
	pydia-text.h \
	diamodule.c \
	python.c

pyplugin_SCRIPTS_TO_INSTALL = \
	aobjects.py \
	allprops.py \
	codegen.py \
	diasvg.py \
	diasvg_import.py \
	dot.py \
	doxrev.py \
	gtkcons.py \
	imgmap.py \
	otypes.py \
	pydiadoc.py \
	scascale.py \
	select_by.py \
	select_empty.py

if WITH_PYTHON

pkgdata_DATA = python-startup.py

pyplugindir = $(pkgdatadir)/python
pyplugin_DATA = \
	$(pyplugin_SCRIPTS_TO_INSTALL)
endif

EXTRA_DIST = \
	python-startup.py \
	\
	$(pyplugin_SCRIPTS_TO_INSTALL) \
	\
	autolayoutforce.py \
	group_props.py \
	\
	bbox.py \
	diastddia.py \
	debug_objects.py \
	dot2dia.py \
	export-object.py \
	export-render.py \
	\
	makefile.msc
