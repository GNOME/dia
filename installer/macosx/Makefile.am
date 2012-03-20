## Process this file with automake to produce Makefile.in

SUBDIRS = Resources

EXTRA_DIST = dia osx-app.sh osx-dmg.sh

Info.plist: Info.plist.in
	sed -e 's/@VERSION/'$(VERSION)'/;' < $< > $@

