## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/intl \
	-I$(top_srcdir)/lib \
	$(DEBUG_FLAGS) \
	$(XML_CFLAGS) \
	$(GTK_CFLAGS) \
        $(GNOME_CFLAGS) \
	$(LIBART_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(FREETYPE_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\"

if HAVE_GNOME_PRINT
print_files = \
	render_gnomeprint.c \
	render_gnomeprint.h \
	paginate_gnomeprint.c \
	paginate_gnomeprint.h
embed_extra_print_files =
else
print_files = \
	paginate_psprint.c \
	paginate_psprint.h
embed_extra_print_files = \
	render_gnomeprint.c \
	render_gnomeprint.h
endif

noinst_PROGRAMS = run_dia.sh run_diaconv.sh diaconv
run_dia_sh_SOURCES = 
run_diaconv_sh_SOURCES = 

if HAVE_BONOBO
dia_embedd = dia-embedd
dia_oafinfo = GNOME_Dia.oaf
else
dia_embedd =
dia_oafinfo =
endif

bin_PROGRAMS = dia $(dia_embedd)

oafinfodir = $(datadir)/oaf
oafinfo_DATA = $(dia_oafinfo)

dia_core_files = \
	    diagram.c \
	    diagram.h \
	    preferences.c \
	    preferences.h \
	    load_save.c \
	    load_save.h \
	    group.c \
	    group.h \
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
	    lineprops_area.c \
	    lineprops_area.h \
	    grid.c \
	    grid.h \
	    handle_ops.c \
	    handle_ops.h \
	    interface.c \
	    interface.h \
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
	    diaunitspinner.c \
	    diaunitspinner.h \
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
	    recent_files.h \
	    recent_files.c \
	    $(print_files)

dia_SOURCES = \
	    $(dia_core_files) \
	    main.c 

diaconv_SOURCES = \
	$(dia_core_files) \
	diaconv.c

EFENCE =

dia_LDADD = ../lib/libdia.a \
	$(EFENCE) \
	$(APP_LIBS) $(XML_LIBS) $(GTK_LIBS) $(GNOME_LIBS) \
	$(LIBART_LIBS) $(PNG_LIBS) $(GDK_PIXBUF_LIBS) $(FREETYPE_LIBS) \
	$(UNICODE_LIBS)  $(INTLLIBS) $(INTLOBJS)

diaconv_LDADD = ../lib/libdia.a \
	$(EFENCE) \
	$(APP_LIBS) $(XML_LIBS) $(GTK_LIBS) $(GNOME_LIBS) \
	$(LIBART_LIBS) $(PNG_LIBS) $(GDK_PIXBUF_LIBS) $(FREETYPE_LIBS) \
	$(UNICODE_LIBS) $(INTLLIBS) $(INTLOBJS)

## Bonobo embeddable dia. Commented out, because it's a bit raw.
## Especially the configure/make parts.

dia_embedd_SOURCES = \
	    $(dia_core_files) \
	    $(embed_extra_print_files) \
	    dia_embedd.c

dia_embedd_LDADD = ../lib/libdia.a \
	$(EFENCE) \
	$(APP_LIBS) $(XML_LIBS) $(BONOBO_LIBS) \
	$(LIBART_LIBS) $(PNG_LIBS) $(GDK_PIXBUF_LIBS) $(INTLLIBS) \
	$(FREETYPE_LIBS) $(UNICODE_LIBS)

LDFLAGS = -export-dynamic

EXTRA_DIST = \
	GNOME_Dia.oaf \
	render_gnomeprint.c \
	render_gnomeprint.h \
	paginate_gnomeprint.c \
	paginate_gnomeprint.h \
	paginate_psprint.c \
	paginate_psprint.h \
	pixmaps/arrow.xpm \
	pixmaps/default.xpm \
	pixmaps/delete.xpm \
	pixmaps/eye.xbm \
	pixmaps/lower.xpm \
	pixmaps/magnify.xpm \
	pixmaps/new.xpm \
	pixmaps/raise.xpm \
	pixmaps/scroll.xpm \
	pixmaps/swap.xpm \
	pixmaps/portrait.xpm \
	pixmaps/landscape.xpm \
	pixmaps/hand-open-data.xbm \
	pixmaps/hand-open-mask.xbm \
	pixmaps/hand-closed-data.xbm \
	pixmaps/hand-closed-mask.xbm \
	pixmaps/snap-to-grid.xpm \
	pixmaps/snap-to-grid-mask.xpm \
	pixmaps/magnify-plus-data.xbm \
	pixmaps/magnify-plus-mask.xbm \
	pixmaps/magnify-minus-data.xbm \
	pixmaps/magnify-minus-mask.xbm

run_dia.sh: Makefile.am
	echo "#!$(SHELL) " > run_dia.sh
	echo "export DIA_LIB_PATH DIA_SHAPE_PATH DIA_INT_SHAPE_PATH DIA_SHEET_PATH" >> run_dia.sh
	echo DIA_LIB_PATH=`pwd`/../objects/standard/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/custom/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/UML/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/ER/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/network/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/FS/.libs >> run_dia.sh
## echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/sybase/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/flowchart/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/SADT/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/GRAFCET/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/chronogram/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/cgm/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/pstricks/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/dxf/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/xfig/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/svg/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/shape/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/wmf/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/python/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/wpg/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../plug-ins/hpgl/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/EML/.libs >> run_dia.sh
	echo DIA_SHAPE_PATH=`pwd`/../shapes >> run_dia.sh
	echo DIA_INT_SHAPE_PATH=`pwd`/../objects >> run_dia.sh
	echo DIA_SHEET_PATH=`pwd`/../sheets >> run_dia.sh
	echo >> run_dia.sh
	echo "\$$DEBUGGER `pwd`/dia \$$*" >> run_dia.sh
	chmod a+x run_dia.sh

run_diaconv.sh: run_dia.sh
	cat run_dia.sh | sed -e "s%`pwd`/dia%`pwd`/diaconv%g" >run_diaconv.sh 
	chmod a+x run_diaconv.sh

