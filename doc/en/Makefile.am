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
	graphics/line_props.png \
	graphics/home_network.png \
	graphics/line_icon.png \
	graphics/greendots.png

dia.1: dia.dbk
	xsltproc --nonet --novalid $(DB2MAN) dia.dbk >dia.1

man_MANS = dia.1
