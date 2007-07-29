## Process this file with automake to produce Makefile.in

SUBDIRS=pixmaps

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

pkglib_LTLIBRARIES =     libdia.la

ICON_PNG_PAIRS= \
	dia_broken_icon $(srcdir)/pixmaps/broken.png \
	dia_broken_chain_icon $(srcdir)/pixmaps/broken-chain.png \
	dia_unbroken_chain_icon $(srcdir)/pixmaps/unbroken-chain.png

# Note that this doesn't seem to get run automatically.  It should, but
# I'm not gonna chase goblins right now.
dia-lib-icons.h:
	gdk-pixbuf-csource --build-list $(ICON_PNG_PAIRS) > dia-lib-icons.h

BUILT_SOURCES = dia-lib-icons.h diamarshal.h diamarshal.c

libdia_la_SOURCES =  \
		diatypes.h \
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
		textline.c \
		textline.h \
		font.c \
		font.h \
	        group.c \
	        group.h \
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
		connectionpoint.c \
		connectionpoint.h \
		handle.h \
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
		boundingbox.h \
		dynamic_obj.c \
		dynamic_obj.h \
		create.c \
		create.h \
		dia-enums.h \
		diatransform.h \
		diatransform.c \
		diarenderer.h \
		diarenderer.c \
		diainteractiverenderer.c \
		diagdkrenderer.h \
		diagdkrenderer.c \
		diasvgrenderer.h \
		diasvgrenderer.c \
		dialibartrenderer.h \
		dialibartrenderer.c \
		dia_svg.h \
		dia_svg.c \
		diagtkfontsel.c \
		diagtkfontsel.h \
		autoroute.c \
		autoroute.h \
		parent.c \
		parent.h \
		render_pixmap.h \
		render_pixmap.c \
		diaarrowchooser.h \
		diaarrowchooser.c \
		dialinechooser.h \
		dialinechooser.c \
		persistence.c \
		persistence.h \
		diaerror.c \
		diaerror.h \
		diamarshal.c \
		diamarshal.h \
		debug.c \
		debug.h \
		prefs.c \
		prefs.h \
	        newgroup.c \
	        newgroup.h \
		dia-lib-icons.h

#		charconv.c \
#		charconv.h 
#		render.h \
#		render.c 

libdia_la_LDFLAGS = -avoid-version

AM_CPPFLAGS = \
	$(LIBART_CFLAGS) \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(pkgdatadir)\"

sheetdir = $(pkgdatadir)/sheets

INCLUDES = -I$(top_srcdir)/intl $(DEBUG_FLAGS) $(GDK_PIXBUF_CFLAGS) \
  $(XML_CFLAGS) $(GNOME_CFLAGS) $(GTK_CFLAGS) \
  -DDIA_SHEETDIR=\"$(sheetdir)\" \
  $(PANGOFT2_CFLAGS) $(LIBART_CFLAGS) $(UNICODE_CFLAGS)

libdia_la_extra_sources = diamarshal.list

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-gwmh xgen-gwmc diamarshal.c diamarshal.h
CLEANFILES = $(gen_sources)

diamarshal.h: diamarshal.list
	$(GLIB_GENMARSHAL) --prefix=dia_marshal $(srcdir)/diamarshal.list --header >> xgen-gwmh \
	&& (cmp -s xgen-gwmh $(@F) || cp xgen-gwmh $(@F)) \
	&& rm -f xgen-gwmh xgen-gwmh~

diamarshal.c: diamarshal.h
	echo "#include \"diamarshal.h\"" >> xgen-gwmc \
	&& $(GLIB_GENMARSHAL) --prefix=dia_marshal $(srcdir)/diamarshal.list --body >> xgen-gwmc \
	&& cp xgen-gwmc $(@F) \
	&& rm -f xgen-gwmc xgen-gwmc~


EXTRA_DIST = libdia.def makefile.msc diamarshal.list
