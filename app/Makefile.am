## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

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
	    render_eps.c \
	    render_eps.h \
	    tool.c \
	    tool.h \
	    pixmaps.h 

dia_LDADD = ../lib/libdia.a @XML_LIBS@ @GTK_LIBS@ @INTLLIBS@ @GNOME_LIBS@ @GDK_IMLIB_LIBS@

INCLUDES = \
	-DLOCALEDIR=\"$(localedir)\" \
	-DLIBDIR=\"$(libdir)\" \
	-I../intl \
	-I$(srcdir)/../lib \
	@DEBUG_FLAGS@ \
	@XML_CFLAGS@ \
	@GTK_CFLAGS@ \
        @GNOME_CFLAGS@

LDFLAGS = -export-dynamic



run_dia.sh: Makefile.am
	 echo "#!$(SHELL) " > run_dia.sh
	 echo "export DIA_LIB_PATH" >> run_dia.sh
	 echo DIA_LIB_PATH=`pwd`/../objects/standard/.libs:`pwd`/../objects/UML/.libs:`pwd`/../objects/ER/.libs:`pwd`/../objects/network/.libs >> run_dia.sh
	 echo >> run_dia.sh
	 echo "`pwd`/dia \$$*" >> run_dia.sh
	 chmod a+x run_dia.sh

