## Process this file with automake to produce Makefile.in

if HAVE_GNOME_PRINT
print_files = \
	render_gnomeprint.c \
	render_gnomeprint.h \
	paginate_gnomeprint.c \
	paginate_gnomeprint.h
else
print_files = \
	paginate_psprint.c \
	paginate_psprint.h
endif

noinst_PROGRAMS = run_dia.sh
run_dia_sh_SOURCES = 
bin_PROGRAMS = dia
dia_SOURCES = \
	    connectionpoint_ops.h \
	    connectionpoint_ops.c \
	    diagram.c \
	    diagram.h \
	    commands.c \
	    commands.h \
	    app_procs.c \
	    app_procs.h \
	    preferences.c \
	    preferences.h \
	    load_save.c \
	    load_save.h \
	    disp_callbacks.c \
	    disp_callbacks.h \
	    layer_dialog.c \
	    layer_dialog.h \
	    display.c \
	    display.h \
	    color_area.c \
	    color_area.h \
	    linewidth_area.c \
	    linewidth_area.h \
	    lineprops_area.c \
	    lineprops_area.h \
	    grid.c \
	    grid.h \
	    group.c \
	    group.h \
	    handle_ops.c \
	    handle_ops.h \
	    interface.c \
	    interface.h \
	    main.c \
	    modify_tool.c \
	    modify_tool.h \
	    scroll_tool.c \
	    scroll_tool.h \
	    properties.c \
	    properties.h \
	    defaults.c \
	    defaults.h \
	    create_object.c \
	    create_object.h \
	    magnify.c \
	    magnify.h \
	    menus.c \
	    menus.h \
	    cut_n_paste.c \
	    cut_n_paste.h \
	    object_ops.c \
	    object_ops.h \
	    render_gdk.c \
	    render_gdk.h \
	    render_libart.c \
	    render_libart.h \
	    render_eps.c \
	    render_eps.h \
	    render_svg.c \
	    render_svg.h \
	    undo.c \
	    undo.h \
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
	    $(print_files)

EFENCE =

dia_LDADD = ../lib/libdia.a \
    $(EFENCE) \
    @APP_LIBS@ @XML_LIBS@ @GTK_LIBS@ @INTLLIBS@ @GNOME_LIBS@ @GDK_IMLIB_LIBS@ @LIBART_LIBS@

CPPFLAGS = \
	-DLOCALEDIR=\"$(localedir)\"

INCLUDES = \
	-I$(top_srcdir)/intl \
	-I$(srcdir)/../lib \
	@DEBUG_FLAGS@ \
	@XML_CFLAGS@ \
	@GTK_CFLAGS@ \
	@GDK_IMLIB_CFLAGS@ \
        @GNOME_CFLAGS@ \
	@LIBART_CFLAGS@

LDFLAGS = -export-dynamic

EXTRA_DIST = \
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
	pixmaps/hand-closed-mask.xbm

run_dia.sh: Makefile.am
	echo "#!$(SHELL) " > run_dia.sh
	echo "export DIA_LIB_PATH DIA_SHAPE_PATH DIA_INT_SHAPE_PATH DIA_SHEET_PATH" >> run_dia.sh
	echo DIA_LIB_PATH=`pwd`/../objects/standard/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/custom/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/UML/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/ER/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/network/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/FS/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/sybase/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/flowchart/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/SADT/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/GRAFCET/.libs >> run_dia.sh
	echo DIA_LIB_PATH=\$$DIA_LIB_PATH:`pwd`/../objects/chronogram/.libs >> run_dia.sh
	echo DIA_SHAPE_PATH=`pwd`/../shapes >> run_dia.sh
	echo DIA_INT_SHAPE_PATH=`pwd`/../objects >> run_dia.sh
	echo DIA_SHEET_PATH=`pwd`/../sheets >> run_dia.sh
	echo >> run_dia.sh
	echo "\$$DEBUGGER `pwd`/dia \$$*" >> run_dia.sh
	chmod a+x run_dia.sh

