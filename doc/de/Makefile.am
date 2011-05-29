docname = dia
lang = de
figdir = graphics
entities =
version = 0.97de
progname = dia
progfiles_extension = dia
PAPERSIZE = a4
##postscript output resolution (dpi)
RESOLUTION = 600
## document encoding
DOCUMENT_ENCODING = UTF-8
## dblatex tex document charset
ENCODING = utf8
UNICODE = 1
## TeX papersize/sides
LATEX_CLASS_OPTIONS = -P 'latex.class.options=a4paper,oneside'


include $(top_srcdir)/xmldocs.make

dist-hook: app-dist-hook

nodist_include_EXTRAS = dia.1

xml_files += \
	authors.xml \
	dia.xml \
	intro.xml \
	license.xml \
	license-documentation.xml \
	usage-canvas.xml \
	usage-loadsave.xml \
	usage-objects-basic.xml \
	usage-objects-selecting.xml \
	usage-objects-special.xml \
	usage-objects.xml \
	usage-quickstart.xml 

EXTRA_DIST += \
	html.xsl

pngfigures = \
	graphics/andere-objektboegen.png \
	graphics/angrenzende-ausrichtung.png \
	graphics/ansicht-menue.png \
	graphics/arrow-style.png \
	graphics/assorted-shapes.png \
	graphics/ausgewaehltes-objekt.png \
	graphics/ausrichtung-mitte.png \
	graphics/ausrichtung-oben.png \
	graphics/ausrichtung-unten.png \
	graphics/bearbeiten-text-menue.png \
	graphics/beispiel-flussdiagramm.png \
	graphics/beispiel-netzwerkdiagramm.png \
	graphics/bezierlinie-kontextmenue.png \
	graphics/cisco-computer-shapes.png \
	graphics/cisco-misc-shapes.png \
	graphics/cisco-network-shapes.png \
	graphics/cisco-switch-shapes.png \
	graphics/cisco-telephony-shapes.png \
	graphics/connect-fixed.png \
	graphics/connect-middle.png \
	graphics/default-colors.png \
	graphics/diagrammbaum.png \
	graphics/diagrammbaum-sortieren.png \
	graphics/diagrammeigenschaften-farben.png \
	graphics/diagrammeigenschaften-gitter.png \
	graphics/dia-start.png \
	graphics/eigenschaften-uml-klasse.png \
	graphics/einrasten-am-gitter-knopf.png \
	graphics/farbauswahldialog.png \
	graphics/farb-dropdown.png \
	graphics/flowchart-shapes.png \
	graphics/gestapelt.png \
	graphics/gruppierung.png \
	graphics/horizontal-verteilen.png \
	graphics/lineal-pfeile.png \
	graphics/line-connect.png \
	graphics/line-handles.png \
	graphics/line_icon.png \
	graphics/line-width.png \
	graphics/linieneigenschaften.png \
	graphics/linienfarben.png \
	graphics/linienstil.png \
	graphics/linienzug-kontextmenue.png \
	graphics/linksbuendig-ausgerichtet.png \
	graphics/menue-auswahl.png \
	graphics/network-shapes.png \
	graphics/objektboegen-objekte.png \
	graphics/objekte-ausrichten-menue.png \
	graphics/pfeilarten.png \
	graphics/rechtsbuendig-ausgerichtet.png \
	graphics/reverse-colors.png \
	graphics/select-area.png \
	graphics/toolbox-basic.png \
	graphics/toolbox-image.png \
	graphics/toolbox-lower.png \
	graphics/toolbox-magnify.png \
	graphics/toolbox-modify.png \
	graphics/toolbox-modify-text.png \
	graphics/toolbox-scroll.png \
	graphics/uml-shapes.png \
	graphics/vertikal-verteilen.png \
	graphics/vor-ausrichtung-oben.png \
	graphics/vorgaben-rechteck.png \
	graphics/vorgaben-text.png \
	graphics/vor-horizontal-verteilen.png \
	graphics/vor-linksbuendig.png \
	graphics/vor-vertikal-verteilen.png \
	graphics/werkzeugkasten.png \
	graphics/zeichenflaeche-zoomen.png \
	graphics/zentriert.png \
	graphics/zickzacklinie-kontextmenue.png \
	graphics/zoom-menue.png

htmlstyle = \
	html.xsl

examples = \
	../shape.dtd \
	../diagram.dtd \
	../sheet.dtd

DISTCLEANFILES = 



