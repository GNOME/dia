## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/intl \
	-I$(top_srcdir)/lib \
	$(DEBUG_FLAGS) \
	$(GTK_CFLAGS) \
	$(GTK_MAC_CFLAGS) \
	$(LIBART_CFLAGS) \
	-DPREFIX=\""$(prefix)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DLOCALEDIR=\"$(localedir)\" \
	-DUIDATADIR=\"$(uidatadir)\"



noinst_PROGRAMS = run_dia.sh

run_dia_sh_SOURCES = 

bin_PROGRAMS = dia

ICON_PNG_PAIRS= \
	dia_connectable_icon $(srcdir)/pixmaps/connectable.png \
	dia_connectable_empty_icon $(srcdir)/pixmaps/connectable-empty.png \
	dia_selectable_icon $(srcdir)/pixmaps/selectable.png \
	dia_selectable_empty_icon $(srcdir)/pixmaps/selectable-empty.png \
	dia_on_grid_icon $(srcdir)/pixmaps/on-grid.png \
	dia_off_grid_icon $(srcdir)/pixmaps/off-grid.png \
	dia_visible_icon $(srcdir)/pixmaps/visible.png \
	dia_visible_empty_icon $(srcdir)/pixmaps/visible-empty.png \
	dia_modify_tool_icon $(srcdir)/pixmaps/arrow-22.png \
	dia_scroll_tool_icon $(srcdir)/pixmaps/scroll-22.png \
	dia_textedit_tool_icon $(srcdir)/pixmaps/textedit-22.png \
	dia_zoom_tool_icon $(srcdir)/pixmaps/zoom-22.png \
	dia_group_icon $(srcdir)/pixmaps/group.png \
	dia_ungroup_icon $(srcdir)/pixmaps/ungroup.png \
	dia_mainpoints_on_icon $(srcdir)/pixmaps/mainpoints-on.png \
	dia_mainpoints_off_icon $(srcdir)/pixmaps/mainpoints-off.png \
	\
	dia_diagram_icon $(srcdir)/pixmaps/dia-diagram.png \
	dia_app_icon $(srcdir)/pixmaps/dia-app.png \
	\
	dia_objects_layer_above $(srcdir)/pixmaps/objects-layer-above.png \
	dia_objects_layer_below $(srcdir)/pixmaps/objects-layer-below.png \
	dia_layers $(srcdir)/pixmaps/layers.png \
	dia_layer_add $(srcdir)/pixmaps/layer-add.png \
	dia_layer_rename $(srcdir)/pixmaps/layer-rename.png \
	\
	dia_cursor_create $(srcdir)/pixmaps/cursor-create.png \
	dia_cursor_hand_open $(srcdir)/pixmaps/cursor-hand-open.png \
	dia_cursor_hand_closed $(srcdir)/pixmaps/cursor-hand-closed.png \
	dia_cursor_magnify_minus $(srcdir)/pixmaps/cursor-magnify-minus.png \
	dia_cursor_magnify_plus $(srcdir)/pixmaps/cursor-magnify-plus.png
	

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
	pixmaps/cursor-create.png \
	pixmaps/cursor-hand-open.png \
	pixmaps/cursor-hand-closed.png \
	pixmaps/cursor-magnify-minus.png \
	pixmaps/cursor-magnify-plus.png \
	\
	pixmaps/arrow-22.png \
	pixmaps/scroll-22.png \
	pixmaps/textedit-22.png \
	pixmaps/zoom-22.png \
	pixmaps/group.png \
	pixmaps/ungroup.png \
	pixmaps/mainpoints-on.png \
	pixmaps/mainpoints-off.png \
	\
	pixmaps/objects-layer-above.png \
	pixmaps/objects-layer-below.png \
	pixmaps/layers.png \
	pixmaps/layer-add.png \
	pixmaps/layer-rename.png
	

dia-app-icons.h: $(ICON_PNGS)
	gdk-pixbuf-csource --raw --build-list $(ICON_PNG_PAIRS) > dia-app-icons.h

BUILT_SOURCES = dia-app-icons.h

dia_core_files = \
	confirm.h \
	confirm.c \
	exit_dialog.h \
	exit_dialog.c \
	diagram.c \
	diagram.h \
	preferences.c \
	preferences.h \
	load_save.c \
	load_save.h \
	properties-dialog.c \
	properties-dialog.h \
	defaults.c \
	defaults.h \
	undo.c \
	undo.h \
	object_ops.c \
	object_ops.h \
	layer_dialog.c \
	layer_dialog.h \
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
	toolbox.c \
	toolbox.h \
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
	ruler.c \
	ruler.h \
	tool.c \
	tool.h \
	diapagelayout.c \
	diapagelayout.h \
	pagesetup.c \
	pagesetup.h \
	filedlg.c \
	filedlg.h \
	find-and-replace.c \
	find-and-replace.h \
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
	\
	dia-application.h \
	dia-application.c \
	\
	diagram_tree.h \
	diagram_tree_model.c \
	diagram_tree_model.h \
	diagram_tree_view.c \
	\
	sheets.c \
	sheets.h \
	sheets_dialog.c \
	sheets_dialog.h \
	sheets_dialog_callbacks.c \
	sheets_dialog_callbacks.h \
	autosave.c \
	autosave.h \
	authors.h \
	dynamic_refresh.h \
	dynamic_refresh.c \
	navigation.h \
	navigation.c \
	highlight.c \
	highlight.h \
	textedit.c \
	textedit.h \
	textedit_tool.c \
	textedit_tool.h \
	diagrid.h

dia_SOURCES = \
	$(dia_core_files) \
	main.c 

EFENCE =

dia_LDADD = ../lib/libdia.la \
	$(EFENCE) \
	$(APP_LIBS) $(Z_LIBS) $(FREETYPE_LIBS) $(GTK_LIBS) $(PNG_LIBS) $(INTLLIBS) $(INTLOBJS) \
	$(XML_LIBS) $(LIBART_LIBS) $(GLIB_LIBS) $(GTK_MAC_LIBS)

## Bonobo embeddable dia. Commented out, because it's a bit raw.
## Especially the configure/make parts.

if HAVE_GCC
# OS X would like += rather than =, but Linux automake no like that.  Grrr.
dia_LDFLAGS = -export-dynamic
endif

EXTRA_DIST = \
	GNOME_Dia.oaf \
	winmain.c \
	makefile.msc \
	dia.def \
	dia.ico \
	dia.rc \
	\
	dia-win-remote.c \
	\
	pixmaps/arrow.xpm \
	pixmaps/swap.xpm \
	pixmaps/default.xpm \
	\
	pixmaps/portrait.xpm \
	pixmaps/landscape.xpm \
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
	echo DIA_XSLT_PATH=\"'$$DIA_BASE_PATH'/plug-ins/xslt\" >> run_dia.sh
	echo >> run_dia.sh
	echo "export DIA_BASE_PATH DIA_LIB_PATH DIA_SHAPE_PATH DIA_INT_SHAPE_PATH DIA_SHEET_PATH DIA_PLUGIN_PATH DIA_PYTHON_PATH DIA_XSLT_PATH" >> run_dia.sh
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
