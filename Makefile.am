## Process this file with automake to produce Makefile.in

SUBDIRS = lib objects shapes intl app samples po

pkgdata_DATA = dia_logo.png
man_MANS = dia.1

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = dia_gnome_icon.png

Applicationsdir = $(datadir)/gnome/apps/Applications
Applications_DATA = dia.desktop

EXTRA_DIST = dia.xpm dia_gnome_icon.png dia_gnome_menu_icon.png \
	dia.desktop $(pkgdata_DATA) $(man_MANS)
