docname = dia
lang = en
figdir = graphics
omffile = dia-C.omf
entities =
version = 0.5
progname = dia
progfiles_extension = dia
PAPERSIZE = Legal
## document encoding
DOCUMENT_ENCODING = ISO-8859-1
## postscript output resolution (dpi)
RESOLUTION = 600
## dblatex tex document charset
ENCODING = utf8
UNICODE = 1
## TeX papersize/sides
LATEX_CLASS_OPTIONS = -P 'latex.class.options=oneside'


include $(top_srcdir)/xmldocs.make

dist-hook: app-dist-hook

nodist_include_EXTRAS = dia.1


xml_files += \
	authors.xml \
	dia.xml \
	dia-cmdline.xml \
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
	dia-1.xml


EXTRA_DIST += \
	dia.dbk \
	dia-1.xml \
	html.xsl

##	graphics/line_props.png \
##	graphics/home_network.png \
##	graphics/line_icon.png \
##	graphics/greendots.png

pngfigures = \
	graphics/adjacent.png \
	graphics/flowchart-shapes.png \
	graphics/after-hor-spread.png \
	graphics/greendots.png \
	graphics/after-vert-spread.png \
	graphics/home_network.png \
	graphics/align-bottom.png \
	graphics/line-arrows.png \
	graphics/align-center.png \
	graphics/line-buttons.png \
	graphics/align-left.png \
	graphics/align-menu.png \
	graphics/line-connect.png \
	graphics/align-middle.png \
	graphics/line-handles.png \
	graphics/align-right.png \
	graphics/line-styles.png \
	graphics/line-caps.png \
	graphics/align-top.png \
	graphics/align-connected.png \
	graphics/align-connected-before.png \
	graphics/line-width.png \
	graphics/arrow-style.png \
	graphics/line_icon.png \
	graphics/assorted-shapes.png \
	graphics/line_props.png \
	graphics/before-hor-spread.png \
	graphics/misc-shapes.png \
	graphics/before-left.png \
	graphics/network-shapes.png \
	graphics/before-top.png \
	graphics/object-group.png \
	graphics/before-vert-spread.png \
	graphics/polyline-rightclick.png \
	graphics/bezierline-rightclick.png \
	graphics/canvas-zoom.png \
	graphics/preferences-gridlines.png \
	graphics/cisco-computer-shapes.png \
	graphics/preferences_fig.png \
	graphics/cisco-misc-shapes.png \
	graphics/properties-uml-class1.png \
	graphics/cisco-network-shapes.png \
	graphics/reverse-colors.png \
	graphics/cisco-switch-shapes.png \
	graphics/ruler-arrows.png \
	graphics/cisco-telephony-shapes.png \
	graphics/sample-flowchart.png \
	graphics/color-dropdown.png \
	graphics/select-area.png \
	graphics/color-selector1.png \
	graphics/select-menu.png \
	graphics/color_selector.png \
	graphics/selected-object.png \
	graphics/connect-fixed.png \
	graphics/sheets-objects.png \
	graphics/connect-middle.png \
	graphics/snaptogrid-button.png \
	graphics/default-colors.png \
	graphics/stacked.png \
	graphics/defaults-box.png \
	graphics/standard-line-properties.png \
	graphics/standard-image-properties.png \
	graphics/defaults-text.png \
	graphics/toolbox-basic.png \
	graphics/toolbox-image.png \
	graphics/dia-startup.png \
	graphics/toolbox-lower.png \
	graphics/diagram-properties-grid.png \
	graphics/toolbox-magnify.png \
	graphics/diagram-properties.png \
	graphics/toolbox-modify.png \
	graphics/toolbox-scroll.png \
	graphics/diagram-tree.png \
	graphics/toolbox-special.png \
	graphics/edit-text.png \
	graphics/toolbox.png \
	graphics/file-preferences-diagram.png \
	graphics/file-preferences-favorites.png \
	graphics/uml-shapes.png \
	graphics/view-menu.png \
	graphics/file-preferences-ui.png \
	graphics/view-zoom.png \
	graphics/file-preferences-view.png \
	graphics/zigzagline-rightclick.png


epsfigures = \
	ps/graphics/color_selector.eps \
	ps/graphics/preferences_fig.eps

htmlstyle = \
	html.xsl


metadata = \
	dia-C.omf \
	topic.dat

examples = \
	../shape.dtd \
	../diagram.dtd \
	../sheet.dtd


DISTCLEANFILES = 

if HAVE_DB2MAN
## Anyone out there who understands automake?
DISTCLEANFILES += dia.1
## If we want misconfigured XML catalogs to not try net access, use
## --nonet.  But I don't think that's going to solve more problems
## than it causes.
dia.1: dia.dbk
	xsltproc --novalid $(DB2MAN) $< >dia.1

man_MANS = dia.1
endif

