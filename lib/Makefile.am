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
	prop_dict.c \
	prop_dict.h \
	prop_matrix.c \
	prop_matrix.h \
	prop_pattern.c \
	prop_pattern.h \
	prop_pixbuf.c \
	prop_pixbuf.h \
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
	prop_sdarray_widget.c \
	prop_sdarray_widget.h \
	propdialogs.c  \
	propregistry.c

if OS_WIN32
lib_LTLIBRARIES = libdia.la
else
pkglib_LTLIBRARIES = libdia.la
endif

ICON_PNG_PAIRS= \
	dia_broken_icon $(srcdir)/pixmaps/broken.png \
	dia_broken_chain_icon $(srcdir)/pixmaps/broken-chain.png \
	dia_unbroken_chain_icon $(srcdir)/pixmaps/unbroken-chain.png \
	stdpath_icon $(srcdir)/pixmaps/standard-path.png

# there is probably some smarter way than duplicating this
# but having the right dependecy for dia-lib-icons.h was 
# broken since 2005 - and no one cared;)
ICON_PNGS= \
	$(srcdir)/pixmaps/broken.png \
	$(srcdir)/pixmaps/broken-chain.png \
	$(srcdir)/pixmaps/unbroken-chain.png \
	$(srcdir)/pixmaps/standard-path.png

dia-lib-icons.h: $(ICON_PNGS)
	gdk-pixbuf-csource --build-list $(ICON_PNG_PAIRS) > dia-lib-icons.h

BUILT_SOURCES = dia-lib-icons.h diamarshal.h diamarshal.c

libdia_la_SOURCES =  \
		diatypes.h \
		diavar.h \
		diamenu.h \
		diagramdata.h \
		diagramdata.c \
		layer.c \
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
		object-alias.h \
		object-alias.c \
		object_defaults.c \
		connection.c \
		connection.h \
		orth_conn.c \
		orth_conn.h \
		poly_conn.c \
		poly_conn.h \
		bezier_conn.c \
		bezier_conn.h \
		bezier-common.c \
		bezier-common.h \
		connection.h \
		element.c \
		element.h \
		polyshape.c \
		polyshape.h \
		beziershape.c \
		beziershape.h \
		objchange.c \
		objchange.h \
		dialogs.c \
		dialogs.h \
		widgets.c \
		widgets.h \
		diaoptionmenu.c \
		diaoptionmenu.h \
		units.c \
		units.h \
		dia_image.c \
		dia_image.h \
		pattern.c \
		pattern.h \
		standard-path.c \
		standard-path.h \
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
		diaimportrenderer.c \
		diaimportrenderer.h \
		diainteractiverenderer.c \
		diagdkrenderer.h \
		diagdkrenderer.c \
		diapathrenderer.h \
		diapathrenderer.c \
		path-math.h \
		path-math.c \
		diapatternselector.h \
		diapatternselector.c \
		diatransformrenderer.h \
		diatransformrenderer.c \
		diasvgrenderer.h \
		diasvgrenderer.c \
		dia_svg.h \
		dia_svg.c \
		autoroute.c \
		autoroute.h \
		parent.c \
		parent.h \
		render_pixmap.h \
		render_pixmap.c \
		diaarrowchooser.h \
		diaarrowchooser.c \
		diaarrowselector.c \
		diacolorselector.c \
		dialinestyleselector.c \
		dialinechooser.h \
		dialinechooser.c \
		diadynamicmenu.h \
		diadynamicmenu.c \
		diafontselector.h \
		diafontselector.c \
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
		dialib.c \
		dialib.h \
		diacontext.c \
		diacontext.h \
		\
		diacellrendererenum.c \
		diacellrendererenum.h \
		\
		diacellrendererproperty.c \
		diacellrendererproperty.h \
		\
		dia-lib-icons.h

#		charconv.c \
#		charconv.h 
#		render.h \
#		render.c

libdia_la_LDFLAGS = -avoid-version --export-dynamic $(NO_UNDEFINED)
if OS_WIN32
libdia_la_LDFLAGS += -export-symbols $(srcdir)/libdia.def
endif
libdia_la_LIBADD = $(GTK_LIBS)

AM_CPPFLAGS = \
	$(LIBART_CFLAGS) \
	-DLIBDIA_COMPILATION \
	-DLIBDIR=\"$(libdir)\" \
	-DDATADIR=\"$(pkgdatadir)\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPREFIX=\"$(prefix)\"

sheetdir = $(pkgdatadir)/sheets

AM_CPPFLAGS += -I$(top_srcdir)/intl $(DEBUG_FLAGS) $(GDK_PIXBUF_CFLAGS) \
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

