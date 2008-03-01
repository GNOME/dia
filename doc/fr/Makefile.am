figdir = graphics
docname = dia
lang = fr
omffile = dia-fr.omf
entities =
version = 0.5_1fr

include $(top_srcdir)/xmldocs.make
dist-hook: app-dist-hook

all: include-cmdline

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
	usage-quickstart.xml \
	custom-shapes.xml \
	dia-cmdline.xml

EXTRA_DIST += \
	dia.dbk \
	graphics/line_props.png \
	graphics/home_network.png \
	graphics/line_icon.png \
	graphics/greendots.png \
	graphics/color_selector.png

##missing	graphics/preferences-fig.png

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

#adding lang to man path
mandir = $(datadir)/man/$(lang)

#outputs and filter dia.dbk into documentation chapter
include-cmdline: dia.dbk
	sed -f dia-dbk-to-chapter.sed $< >dia-cmdline.xml

