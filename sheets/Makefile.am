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
        Logic.sheet

CLEANFILES = sheet-translation-report

sheet-translation-report: $(SHEETS)
	echo "*** Sheet translation report : ***" >sheet-translation-report
	echo "(Absence of a language code means 0% translation for that language)" >>sheet-translation-report
	echo "(Help for translations (and/or much more) is of course welcome !)" >>sheet-translation-report
	-(./checktrans.py $(SHEETS) || \
	   echo "E:checktrans failed to run. Check that python and python-xml are installed on the build system.") \
		 >> sheet-translation-report
	cat sheet-translation-report

# EXTRA_DIST = $(SHEETS) 

sheet_DATA = $(SHEETS) sheet-translation-report

