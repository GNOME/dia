#!/bin/sh
# Run this to generate all the stuff before configure (which only
# maintainers should generate).

PROJECT=dia

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile $PROJECT."
	echo "For more details please read INSTALL file."
	exit 1
}

(libtool --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile $PROJECT."
	echo "For more details please read INSTALL file."
	exit 1
}

(intltoolize --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have intltool installed to compile $PROJECT."
	echo "For more details please read INSTALL file."
	exit 1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have automake installed to compile $PROJECT."
	echo "For more details please read INSTALL file."
	exit 1
}


# Can use any file / dir to check that we are in top-level Dia.
test -f dia.desktop.in.in || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

case $CC in
*lcc | *lcc\ *) am_opt=--include-deps;;
esac

echo "Running gettextize..."
glib-gettextize --copy --force

echo "Running intltoolize"
intltoolize --copy --force --automake

echo "Running libtoolize"
libtoolize --copy --force

aclocal $ACLOCAL_FLAGS
autoheader
automake --add-missing $am_opt
autoconf
