#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

PROJECT=dia
TEST_TYPE=-d
FILE=objects

DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile $PROJECT."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
}

(libtool --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile $PROJECT."
	echo "Get ftp://alpha.gnu.org/gnu/libtool-1.0h.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have automake installed to compile $PROJECT."
	echo "Get ftp://ftp.cygnus.com/pub/home/tromey/automake-1.2d.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}

if test "$DIE" -eq 1; then
	exit 1
fi

test $TEST_TYPE $FILE || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

case $CC in
*lcc | *lcc\ *) am_opt=--include-deps;;
esac

echo "Running gettextize...  Ignore non-fatal messages."
## Hmm, we specify --force here, since otherwise things don't
## get added reliably, but we don't want to overwrite intl
## while making dist.
#echo "no" | gettextize --copy --force
# finally, no, we don't try to force. Otherwise gettextize spends its time 
# telling its life in po/ChangeLog.
gettextize --copy

echo "Running xml-i18n-toolize"
xml-i18n-toolize --copy --force --automake
[ -f xml-i18n-update.in.kg ] && cp xml-i18n-update.in.kg xml-i18n-update.in
[ -f xml-i18n-merge.in.kg ] && cp xml-i18n-merge.in.kg xml-i18n-merge.in

echo "Running libtoolize"
libtoolize --copy --force

aclocal $ACLOCAL_FLAGS

autoheader$ACFLAVOUR

automake -a $am_opt
autoconf$ACFLAVOUR

cd $ORIGDIR

$srcdir/configure --enable-maintainer-mode --enable-db2html "$@"

echo 
echo "Now type 'make' to compile $PROJECT."
