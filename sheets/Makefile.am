SUBDIRS = UML ER GRAFCET

sheetdir = $(pkgdatadir)/sheets

SHEETS = \
	UML.sheet  \
	ER.sheet \
	sybase.sheet \
	FS.sheet \
	network.sheet \
	Flowchart.sheet \
	Circuit.sheet \
	Contact.sheet \
	GRAFCET.sheet \
	chronogram.sheet \
	SADT.sheet \
	Pneumatic.sheet \
	Electric.sheet \
	civil.sheet \
	jigsaw.sheet \
	MSE.sheet \
	SDL.sheet \
        Logic.sheet \
	EML.sheet

CLEANFILES = sheet-translation-report

sheet-translation-report: $(SHEETS) checktrans checktrans.py
	-(cd $(srcdir) $$ ./checktrans $(SHEETS))

EXTRA_DIST = sheet-translation-report $(SHEETS) checktrans checktrans.py

sheet_DATA = $(SHEETS)

all-local: sheet-translation-report