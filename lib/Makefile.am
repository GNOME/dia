## Process this file with automake to produce Makefile.in

noinst_LIBRARIES =     libdia.a
libdia_a_SOURCES =  \
		diamenu.h \
		diagramdata.h \
		diagramdata.c \
		geometry.h \
		geometry.c \
		color.h \
		color.c \
		dia_xml.c \
		dia_xml.h \
		attributes.h \
		attributes.c \
		render_store.c \
		render_store.h \
		render_object.c \
		render_object.h \
		text.c \
		text.h \
		font.c \
		font.h \
		utils.c \
		utils.h \
		arrows.c \
		arrows.h \
		message.c \
		message.h \
		focus.c \
		focus.h \
		sheet.c \
		sheet.h \
		connectionpoint.h \
		handle.h \
		render.h \
		object.h \
		object.c \
		connection.c \
		connection.h \
		orth_conn.c \
		orth_conn.h \
		neworth_conn.c \
		neworth_conn.h \
		poly_conn.c \
		poly_conn.h \
		bezier_conn.c \
		bezier_conn.h \
		connection.h \
		element.c \
		element.h \
		polyshape.c \
		polyshape.h \
		objchange.c \
		objchange.h \
		widgets.c \
		widgets.h \
		dia_image.c \
		dia_image.h \
		intl.c \
		intl.h \
		filter.c \
		filter.h \
		connpoint_line.c \
		connpoint_line.h \
		properties.c \
		properties.h \
		lazyprops.c \
		lazyprops.h \
		dia_dirs.c \
		dia_dirs.h \
		dummy_dep.h

CPPFLAGS = \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(pkgdatadir)\"

sheetdir = $(pkgdatadir)/sheets

INCLUDES = -I$(top_srcdir)/intl @GDK_IMLIB_CFLAGS@ @XML_CFLAGS@ @GNOME_CFLAGS@ @GTK_CFLAGS@ -DDIA_SHEETDIR=\"$(sheetdir)\"

EXTRA_DIST = \
	pixmaps/broken.xpm
