figdir=graphics
docname=dia
lang=en
omffile=
entities=

include $(top_srcdir)/xmldocs.make
dist-hook: app-dist-hook

nodist_include_EXTRAS = dia.1

xml_files += \
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
	usage-quickstart.xml

EXTRA_DIST += \
	dia.dbk
#	graphics/line_props.png \
#	graphics/home_network.png \
#	graphics/line_icon.png \
#	graphics/greendots.png

if HAVE_DB2MAN
# Anyone out there who understands automake?
DISTCLEANFILES = dia.1
# If we want misconfigured XML catalogs to not try net access, use
# --nonet.  But I don't think that's going to solve more problems
# than it causes.
dia.1: dia.dbk
	xsltproc --novalid $(DB2MAN) $< >dia.1
endif

man_MANS = dia.1
