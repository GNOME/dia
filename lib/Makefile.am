## Process this file with automake to produce Makefile.in

stdprop_files = \
	properties.h \
	properties.c \
	propinternals.h \
	propdesc.c \
	proplist.c \
	propoffsets.c \
	propobject.c \
	prop_basic.c \
	prop_basic.h \
	prop_inttypes.c \
	prop_inttypes.h \
	prop_geomtypes.c \
	prop_geomtypes.h \
	prop_attr.c \
	prop_attr.h \
	prop_text.c \
	prop_text.h \
	prop_widgets.c \
	prop_widgets.h \
	prop_sdarray.c \
	prop_sdarray.h \
	propdialogs.c  \
	propregistry.c 

noinst_LIBRARIES =     libdia.a
libdia_a_SOURCES =  \
		diavar.h \
		diamenu.h \
		diagramdata.h \
		diagramdata.c \
		geometry.h \
		geometry.c \
		color.h \
		color.c \
		dia_xml.c \
		dia_xml.h \
		dia_xml_libxml.h \
		attributes.h \
		attributes.c \
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
		render.c \
		object.h \
		object.c \
		object_defaults.c \
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
		beziershape.c \
		beziershape.h \
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
		$(stdprop_files) \
		textattr.h \
		dia_dirs.c \
		dia_dirs.h \
		plug-ins.c \
		plug-ins.h \
		dummy_dep.h \
		paper.c \
		paper.h \
		ps-utf8.c \
		ps-utf8.h \
		boundingbox.c \
		boundingbox.h

#		charconv.c \
#		charconv.h \

CPPFLAGS = \
	$(LIBART_CFLAGS) \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(pkgdatadir)\"

sheetdir = $(pkgdatadir)/sheets

INCLUDES = -I$(top_srcdir)/intl $(DEBUG_FLAGS) $(GDK_PIXBUF_CFLAGS) \
  $(XML_CFLAGS) $(GNOME_CFLAGS) $(GTK_CFLAGS) -DDIA_SHEETDIR=\"$(sheetdir)\" \
  $(FREETYPE_CFLAGS) $(LIBART_CFLAGS) $(UNICODE_CFLAGS)

EXTRA_DIST = \
	pixmaps/broken.xpm
