## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/intl \
	-I$(top_srcdir)/lib \
	$(DEBUG_FLAGS) \
	$(GTK_CFLAGS) \
        -DPREFIX=\""$(prefix)"\" \
        -DSYSCONFDIR=\""$(sysconfdir)"\" \
        -DDATADIR=\""$(datadir)"\" \
        -DLIBDIR=\""$(libdir)"\" \
	-DLOCALEDIR=\"$(localedir)\"

print_files = \
	paginate_psprint.c \
	paginate_psprint.h

noinst_PROGRAMS = run_dia.sh #run_diaconv.sh diaconv

run_dia_sh_SOURCES = 
#run_diaconv_sh_SOURCES = 

if HAVE_BONOBO
dia_embedd = dia-embedd
dia_oafinfo = GNOME_Dia.oaf
else
dia_embedd =
dia_oafinfo =
endif

bin_PROGRAMS = dia $(dia_embedd)

ICON_PNG_PAIRS= \
	dia_connectable_icon pixmaps/connectable.png \
	dia_connectable_empty_icon pixmaps/connectable-empty.png \
	dia_selectable_icon pixmaps/selectable.png \
	dia_selectable_empty_icon pixmaps/selectable-empty.png \
	dia_on_grid_icon pixmaps/on-grid.png \
	dia_off_grid_icon pixmaps/off-grid.png \
	dia_visible_icon pixmaps/visible.png \
	dia_visible_empty_icon pixmaps/visible-empty.png \
        dia_diagram_icon ../dia-diagram.png \
	dia_app_icon ../dia_gnome_icon.png \
	dia_modify_tool_icon pixmaps/arrow-22.png \
	dia_scroll_tool_icon pixmaps/scroll-22.png \
	dia_zoom_tool_icon pixmaps/zoom-22.png \
	dia_group_icon pixmaps/group.png \
	dia_ungroup_icon pixmaps/ungroup.png \
	dia_mainpoints_on_icon pixmaps/mainpoints-on.png \
	dia_mainpoints_off_icon pixmaps/mainpoints-off.png

ICON_PNGS= \
	pixmaps/connectable.png \
	pixmaps/connectable-empty.png \
	pixmaps/selectable.png \
	pixmaps/selectable-empty.png \
	pixmaps/on-grid.png \
	pixmaps/off-grid.png \
	pixmaps/visible.png \
	pixmaps/visible-empty.png \
        \
	\
	pixmaps/arrow-22.png \
	pixmaps/scroll-22.png \
	pixmaps/zoom-22.png \
	pixmaps/group.png \
	pixmaps/ungroup.png \
	pixmaps/mainpoints-on.png \
	pixmaps/mainpoints-off.png
	

dia-app-icons.h: $(ICON_PNGS)
	gdk-pixbuf-csource --raw --build-list $(ICON_PNG_PAIRS) > dia-app-icons.h

BUILT_SOURCES = dia-app-icons.h

oafinfodir = $(datadir)/oaf
oafinfo_DATA = $(dia_oafinfo)

dia_core_files = \
	    diagram.c \
	    diagram.h \
	    preferences.c \
	    preferences.h \
	    load_save.c \
	    load_save.h \
	    properties.c \
	    properties.h \
	    defaults.c \
	    defaults.h \
	    render_eps.c \
	    render_eps.h \
	    render_libart.c \
	    render_libart.h \
	    undo.c \
	    undo.h \
	    object_ops.c \
	    object_ops.h \
	    layer_dialog.c \
	    layer_dialog.h \
	    export_png.c \
	    commands.c \
	    commands.h \
	    app_procs.c \
	    app_procs.h \
	    connectionpoint_ops.h \
	    connectionpoint_ops.c \
	    disp_callbacks.c \
	    disp_callbacks.h \
	    display.c \
	    display.h \
	    select.c \
	    select.h \
	    color_area.c \
	    color_area.h \
	    linewidth_area.c \
	    linewidth_area.h \
	    grid.c \
	    grid.h \
	    handle_ops.c \
	    handle_ops.h \
	    interface.c \
	    interface.h \
	    dia-app-icons.h \
	    modify_tool.c \
	    modify_tool.h \
	    scroll_tool.c \
	    scroll_tool.h \
	    create_object.c \
	    create_object.h \
	    magnify.c \
	    magnify.h \
	    menus.c \
	    menus.h \
	    cut_n_paste.c \
	    cut_n_paste.h \
	    render_gdk.c \
	    render_gdk.h \
	    tool.c \
	    tool.h \
	    pixmaps.h \
	    diapagelayout.c \
	    diapagelayout.h \
	    pagesetup.c \
	    pagesetup.h \
	    filedlg.c \
	    filedlg.h \
	    plugin-manager.c \
	    plugin-manager.h \
	    dia-props.c \
	    dia-props.h \
	    gtkwrapbox.h \
	    gtkwrapbox.c \
	    gtkhwrapbox.h \
	    gtkhwrapbox.c \
	    gtkvwrapbox.h \
	    gtkvwrapbox.c \
	    cursor.c \
	    cursor.h \
	    splash.c \
	    dia-app-icons.h \
	    recent_files.h \
	    recent_files.c \
	    diagram_tree.h \
	    diagram_tree.c \
	    diagram_tree_window.h \
	    diagram_tree_window.c \
	    diagram_tree_menu.h \
	    diagram_tree_menu.c \
	    diagram_tree_menu_callbacks.h \
	    diagram_tree_menu_callbacks.c \
	    sheets.c \
	    sheets.h \
	    sheets_dialog.c \
	    sheets_dialog.h \
	    sheets_dialog_callbacks.c \
	    sheets_dialog_callbacks.h \
	    dialogs.c \
	    dialogs.h \
	    autosave.c \
	    autosave.h \
	    authors.h \
	    dynamic_refresh.h \
	    dynamic_refresh.c \
	    diapsrenderer.h \
	    diapsrenderer.c \
	    diapsft2renderer.h \
	    diapsft2renderer.c \
	    navigation.h \
	    navigation.c \
	    diacanvas.c \
	    diacanvas.h \
	    highlight.c \
	    highlight.h \
	    textedit.c \
	    textedit.h \
	    diagrid.h \
	    $(print_files)

dia_SOURCES = \
	    $(dia_core_files) \
	    main.c 

#diaconv_SOURCES = \
#	$(dia_core_files) \
#	diaconv.c

EFENCE =

dia_LDADD = ../lib/libdia.la \
	$(EFENCE) \
	$(APP_LIBS) $(Z_LIBS) $(FREETYPE_LIBS) $(GTK_LIBS) $(PNG_LIBS) $(INTLLIBS) $(INTLOBJS)

#diaconv_LDADD = ../lib/libdia.la \
#	$(EFENCE) \
#	$(APP_LIBS) $(GTK_LIBS) $(PNG_LIBS) $(INTLLIBS) $(INTLOBJS)

## Bonobo embeddable dia. Commented out, because it's a bit raw.
## Especially the configure/make parts.

dia_embedd_SOURCES = \
	    $(dia_core_files) \
	    dia_embedd.c

dia_embedd_LDADD = ../lib/libdia.la \
	$(EFENCE) \
	$(APP_LIBS) $(PNG_LIBS) $(GTK_LIBS) $(INTLLIBS) 
#$(BONOBO_LIBS)

if HAVE_GCC
# OS X would like += rather than =, but Linux automake no like that.  Grrr.
dia_LDFLAGS = -export-dynamic
endif

EXTRA_DIST = \
	GNOME_Dia.oaf \
	paginate_psprint.c \
	paginate_psprint.h \
	paginate_gdiprint.cpp \
	paginate_gdiprint.h \
	win32print.c \
	win32print.h \
	winmain.c \
	makefile.msc \
	dia.def \
	dia.ico \
	dia.rc \
	\
	diaconv.c \
	\
	pixmaps/arrow.xpm \
	pixmaps/swap.xpm \
	pixmaps/default.xpm \
	pixmaps/magnify.xpm \
	pixmaps/scroll.xpm \
	\
	pixmaps/portrait.xpm \
	pixmaps/landscape.xpm \
	\
	pixmaps/hand-open-data.xbm \
	pixmaps/hand-open-mask.xbm \
	pixmaps/hand-closed-data.xbm \
	pixmaps/hand-closed-mask.xbm \
	pixmaps/magnify-plus-data.xbm \
	pixmaps/magnify-plus-mask.xbm \
	pixmaps/magnify-minus-data.xbm \
	pixmaps/magnify-minus-mask.xbm \
	\
	pixmaps/missing.xpm \
	pixmaps/n_a.xpm \
	pixmaps/line_break.xpm \
	\
	$(ICON_PNGS)

run_dia.sh$(EXEEXT): Makefile.am
	echo "#!$(SHELL) " > run_dia.sh
	echo DIA_APP_PATH=\"`pwd`\" >>run_dia.sh
	echo DIA_BASE_PATH=\"'$$DIA_APP_PATH'/..\" >>run_dia.sh
	echo DIA_LIBS_PATH=\"'$$DIA_APP_PATH'/.libs\" >>run_dia.sh
	echo >> run_dia.sh
	echo DIA_LIB_PATH=\"'$$DIA_BASE_PATH'/objects//:'$$DIA_BASE_PATH'/plug-ins//\" >> run_dia.sh
	echo DIA_PLUGIN_PATH=\"'$$DIA_BASE_PATH'/plug-ins\" >> run_dia.sh
	echo DIA_SHAPE_PATH=\"'$$DIA_BASE_PATH'/shapes\" >> run_dia.sh
	echo DIA_INT_SHAPE_PATH=\"'$$DIA_BASE_PATH'/objects\" >> run_dia.sh
	echo DIA_SHEET_PATH=\"'$$DIA_BASE_PATH'/sheets\" >> run_dia.sh
	echo DIA_PYTHON_PATH=\"'$$DIA_BASE_PATH'/plug-ins/python\" >> run_dia.sh
	echo >> run_dia.sh
	echo "export DIA_LIB_PATH DIA_SHAPE_PATH DIA_INT_SHAPE_PATH DIA_SHEET_PATH DIA_PLUGIN_PATH DIA_PYTHON_PATH" >> run_dia.sh
	echo "if [ \"x\$$DEBUGGER\" != \"x\" ] ; then " >> run_dia.sh
	echo "  if [ ! -f \"\$$DIA_LIBS_PATH/lt-dia\" -o \"\$$DIA_LIBS_PATH/lt-dia\" -ot \"\$$DIA_LIBS_PATH/dia\" ] ; then" >> run_dia.sh
	echo "    echo \"libtool relink stage necessary before debugging is possible.\"" >> run_dia.sh
	echo "    echo \"please run \$$0 once without a debugger.\"" >> run_dia.sh
	echo "    exit 1" >> run_dia.sh
	echo "  fi" >> run_dia.sh
	echo "   \$$DEBUGGER \"\$$DIA_LIBS_PATH/lt-dia\" \"\$$@\"" >> run_dia.sh
	echo "else" >> run_dia.sh
	echo "   \"\$$DIA_APP_PATH/dia\" \"\$$@\"" >> run_dia.sh
	echo "fi" >> run_dia.sh
	chmod a+x run_dia.sh

#run_diaconv.sh: run_dia.sh
#	cat run_dia.sh | sed -e "s%`pwd`/dia%`pwd`/diaconv%g" >run_diaconv.sh 
#	chmod a+x run_diaconv.sh

