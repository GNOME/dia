figdir=graphics
docname=dia
lang=en
omffile=
entities=

include $(top_srcdir)/xmldocs.make
dist-hook: app-dist-hook

EXTRA_DIST += \
	dia.1 \
	dia.dbk \
	authors.xml \
	dia.xml \
	entities.xml \
	intro.xml \
	license.xml \
	usage-canvas.xml \
	usage-customization.xml \
	usage-layers.xml \
	usage-loadsave.xml \
	usage-objects-basic.xml \
	usage-objects-selecting.xml \
	usage-objects-special.xml \
	usage-objects.xml \
	usage-quickstart.xml \
	graphics/line_props.png \
	graphics/home_network.png \
	graphics/line_icon.png \
	graphics/greendots.png

if HAVE_DB2MAN
dia.1: dia.dbk
	xsltproc --nonet --novalid $(DB2MAN) dia.dbk >dia.1
endif

man_MANS = dia.1
