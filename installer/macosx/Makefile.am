## Process this file with automake to produce Makefile.in

EXTRA_DIST = dia Dia.icns osx-app.sh osx-dmg.sh

Info.plist: Info.plist.in
	sed -e 's/@VERSION/'$(VERSION)'/;' < $< > $@

