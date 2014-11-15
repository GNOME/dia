#!/bin/bash
#
# USAGE
# osx-app [-s] [-l /path/to/libraries] -py /path/to/python/modules [-l /path/to/libraries] -b /path/to/bin/dia -p /path/to/Info.plist
#
# This script attempts to build an Dia.app package for OS X, resolving
# dynamic libraries, etc.
# 
# If the '-s' option is given, then the libraries and executable are stripped.
# 
# The Info.plist file can be found in the base dia directory once
# configure has been run.
#
# AUTHORS
#		 Steffen Macke <sdteffen@sdteffen.de>
#		 Kees Cook <kees@outflux.net>
#		 Michael Wybrow <mjwybrow@users.sourceforge.net>
#		 Jean-Olivier Irisson <jo.irisson@gmail.com>
# 
# Copyright (C) 2005 Kees Cook
# Copyright (C) 2005-2009 Michael Wybrow
# Copyright (C) 2007-2009 Jean-Olivier Irisson
# Copyright (C) 2010-2012 Steffen Macke
#
# Released under GNU GPL, read the file 'COPYING' for more information
#
# Thanks to GNUnet's "build_app" script for help with library dep resolution.
#		https://gnunet.org/svn/GNUnet/contrib/OSX/build_app
# 

# Defaults
strip=false

# If LIBPREFIX is not already set (by osx-build.sh for example) set it to blank (one should use the command line argument to set it correctly)
if [ -z $LIBPREFIX ]; then
	LIBPREFIX=""
fi


# Help message
#----------------------------------------------------------
help()
{
echo -e "
Create an app bundle for OS X

\033[1mUSAGE\033[0m
	$0 [-s] [-l /path/to/libraries] -py /path/to/python/modules -b /path/to/bin/dia -p /path/to/Info.plist

\033[1mOPTIONS\033[0m
	\033[1m-h,--help\033[0m 
		display this help message
	\033[1m-s\033[0m
		strip the libraries and executables from debugging symbols
	\033[1m-py,--with-python\033[0m
		add python modules (numpy, lxml) from given directory
		inside the app bundle
	\033[1m-l,--libraries\033[0m
		specify the path to the librairies Dia depends on
		(typically /sw or /opt/local)
	\033[1m-b--binary\033[0m
		specify the path to Dia's binary. By default it is in
		Build/bin/ at the base of the source code directory
	\033[1m-p,--plist\033[0m
		specify the path to Info.plist. Info.plist can be found
		in the base directory of the source code once configure
		has been run

\033[1mEXAMPLE\033[0m
	$0 -s -py ~/python-modules -l /opt/local -b ../../Build/bin/dia -p ../../Info.plist
"
}


# Parse command line arguments
#----------------------------------------------------------
while [ "$1" != "" ]
do
	case $1 in
		-py|--with-python)
			add_python=true
			python_dir="$2"
			shift 1 ;;
		-s)
			strip=true ;;
		-l|--libraries)
			LIBPREFIX="$2"
			shift 1 ;;
		-b|--binary)
			binary="$2"
			shift 1 ;;
		-p|--plist)
			plist="$2"
			shift 1 ;;
		-h|--help)
			help
			exit 0 ;;
		*)
			echo "Invalid command line option: $1" 
			exit 2 ;;
	esac
	shift 1
done

echo -e "\n\033[1mCREATE DIA APP BUNDLE\033[0m\n"

# Safety tests

if [ "x$binary" == "x" ]; then
	echo "Dia binary path not specified." >&2
	exit 1
fi

if [ ! -x "$binary" ]; then
	echo "Dia executable not not found at $binary." >&2
	exit 1
fi

if [ "x$plist" == "x" ]; then
	echo "Info.plist file not specified." >&2
	exit 1
fi

if [ ! -f "$plist" ]; then
	echo "Info.plist file not found at $plist." >&2
	exit 1
fi


if [ ! -e "$LIBPREFIX" ]; then
	echo "Cannot find the directory containing the libraries: $LIBPREFIX" >&2
	exit 1
fi

#if ! pkg-config --exists gtk-engines-2; then
#	echo "Missing gtk-engines2 -- please install gtk-engines2 and try again." >&2
#	exit 1
#fi


# Handle some version specific details.
VERSION=`/usr/bin/sw_vers | grep ProductVersion | cut -f2 -d'.'`
if [ "$VERSION" -ge "4" ]; then
	# We're on Tiger (10.4) or later.
	# XCode behaves a little differently in Tiger and later.
	XCODEFLAGS="-configuration Deployment"
	SCRIPTEXECDIR="ScriptExec/build/Deployment/ScriptExec.app/Contents/MacOS"
	EXTRALIBS=""
else
	# Panther (10.3) or earlier.
	XCODEFLAGS="-buildstyle Deployment"
	SCRIPTEXECDIR="ScriptExec/build/ScriptExec.app/Contents/MacOS"
	EXTRALIBS=""
fi


# Package always has the same name. Version information is stored in
# the Info.plist file which is filled in by the configure script.
package="Dia.app"

# Remove a previously existing package if necessary
if [ -d $package ]; then
	echo "Removing previous Dia.app"
	rm -Rf $package
fi


# Set the 'macosx' directory, usually the current directory.
resdir=`pwd`


# Prepare Package
#----------------------------------------------------------
pkgexec="$package/Contents/MacOS"
pkgbin="$package/Contents/Resources/bin"
pkglib="$package/Contents/Resources/lib"
pkgdata="$package/Contents/Resources/data"
pkgdia="$package/Contents/Resources/dia"
pkglocale="$package/Contents/Resources/share/locale"
pkgpython="$package/Contents/Resources/python/site-packages/"
pkgresources="$package/Contents/Resources"

mkdir -p "$pkgexec"
mkdir -p "$pkgbin"
mkdir -p "$pkgdata"
mkdir -p "$pkgdia"
mkdir -p "$pkglib"
mkdir -p "$pkglocale"
mkdir -p "$pkgpython"

# Copy all files into the bundle
#----------------------------------------------------------
echo -e "\n\033[1mFilling app bundle...\033[0m\n"

binary_name=`basename "$binary"`
binary_dir=`dirname "$binary"`

# Dia's binary
binpath="$pkgbin/dia-bin"
cp -v "$binary" "$binpath"
cp "dia" "$pkgbin/dia"
cp "../../data/dia-splash.png" "$pkgresources/"

# Share files
rsync -av "$binary_dir/../share/$binary_name"/* "$pkgresources/"
cp "$plist" "$package/Contents/Info.plist"
rsync -av "$binary_dir/../share/locale"/* "$pkgresources/share/locale"

rsync -av "$binary_dir/../lib/dia"/* "$pkgdia/"
rsync -av "$binary_dir/../share/dia/ui"/* "$pkgdata/"

cp -rp "$binary_dir/../lib/gdk-pixbuf-2.0" "$pkglib"
cp -rp "$binary_dir/../lib/pango" "$pkglib"

# Copy GTK shared mime information
mkdir -p "$pkgresources/share"
cp -rp "$binary_dir/../share/mime" "$pkgresources/share/"

# Icons and the rest of the script framework
rsync -av --exclude ".svn" "$resdir"/Resources/* "$pkgresources/"

# PkgInfo must match bundle type and creator code from Info.plist
echo "APPL????" > $package/Contents/PkgInfo

# Pull in extra requirements for Pango and GTK
pkgetc="$package/Contents/Resources/etc"
mkdir -p $pkgetc/pango
# Need to adjust path and quote in case of spaces in path.
sed -e "s,$LIBPREFIX,\"\${CWD},g" -e 's,\.so ,.so" ,g' $binary_dir/../etc/pango/pango.modules > $pkgetc/pango/pango.modules

cat > $pkgetc/pango/pangorc <<END_PANGO
[Pango]
ModuleFiles=\${HOME}/.dia-etc/pango.modules
END_PANGO

# We use a modified fonts.conf file so only need the dtd
mkdir -p $pkgetc/fonts
cp $binary_dir/../etc/fonts/fonts.dtd $pkgetc/fonts/
cp $binary_dir/../etc/fonts/fonts.conf $pkgetc/fonts/
cp -r $binary_dir/../etc/fonts/conf.avail $pkgetc/fonts/
cp -r $binary_dir/../etc/fonts/conf.d $pkgetc/fonts/

mkdir -p $pkgetc/gtk-2.0
sed -e "s,$LIBPREFIX,\${CWD},g" $binary_dir/../etc/gtk-2.0/gdk-pixbuf.loaders > $pkgetc/gtk-2.0/gdk-pixbuf.loaders
sed -e "s,$LIBPREFIX,\${CWD},g" $binary_dir/../etc/gtk-2.0/gtk.immodules > $pkgetc/gtk-2.0/gtk.immodules

pango_version=`pkg-config --variable=pango_module_version pango`
mkdir -p $pkglib/pango/$pango_version/modules
cp $binary_dir/../lib/pango/$pango_version/modules/*.so $pkglib/pango/$pango_version/modules/

gtk_version=`pkg-config --variable=gtk_binary_version gtk+-2.0`
mkdir -p $pkglib/gtk-2.0/$gtk_version/{engines,immodules,loaders,printbackends}
cp -r $binary_dir/../lib/gtk-2.0/$gtk_version/* $pkglib/gtk-2.0/$gtk_version/


# Find out libs we need from fink, darwinports, or from a custom install
# (i.e. $LIBPREFIX), then loop until no changes.
a=1
nfiles=0
endl=true
while $endl; do
	echo -e "\033[1mLooking for dependencies.\033[0m Round" $a
	libs="`otool -L $pkglib/gtk-2.0/$gtk_version/{engines,immodules,loaders,printbackends}/*.{dylib,so} $pkglib/gdk-pixbuf-2.0/$gtk_version/loaders/* $pkglib/pango/$pango_version/modules/* $pkglib/gnome-vfs-2.0/modules/* $package/Contents/Resources/lib/* $binary 2>/dev/null | fgrep compatibility | cut -d\( -f1 | grep $LIBPREFIX | sort | uniq`"
	cp -f $libs $package/Contents/Resources/lib
	let "a+=1"	
	nnfiles=`ls $package/Contents/Resources/lib | wc -l`
	if [ $nnfiles = $nfiles ]; then
		endl=false
	else
		nfiles=$nnfiles
	fi
done

# Add extra libraries of necessary
for libfile in $EXTRALIBS
do
	cp -f $libfile $package/Contents/Resources/lib
done


# Strip libraries and executables if requested
#----------------------------------------------------------
if [ "$strip" = "true" ]; then
	echo -e "\n\033[1mStripping debugging symbols...\033[0m\n"
	chmod +w "$pkglib"/*.dylib
	strip -x "$pkglib"/*.dylib
	strip -ur "$binpath"
fi

# NOTE: This works for all the dylibs but causes GTK to crash at startup.
#				Instead we leave them with their original install_names and set
#				DYLD_LIBRARY_PATH within the app bundle before running Dia.
#
fixlib () {
	libPath="`echo $2 | sed 's/.*Resources//'`"
	pkgPath="`echo $2 | sed 's/Resources\/.*/Resources/'`"
	# Fix a given executable or library to be relocatable
	if [ ! -d "$1" ]; then
		libs="`otool -L $1 | fgrep compatibility | cut -d\( -f1`"
		for lib in $libs; do
			echo "	$lib"
			base=`echo $lib | awk -F/ '{print $NF}'`
			first=`echo $lib | cut -d/ -f1-3`
			relative=`echo $lib | cut -d/ -f4-`
			to=@executable_path/../$relative
			if [ $first != /usr/lib -a $first != /usr/X11 -a $first != /System/Library ]; then
				/usr/bin/install_name_tool -change $lib $to $1
				if [ "`echo $lib | fgrep libcrypto`" = "" ]; then
					/usr/bin/install_name_tool -id $to $1
					for ll in $libs; do
						base=`echo $ll | awk -F/ '{print $NF}'`
						first=`echo $ll | cut -d/ -f1-3`
						relative=`echo $ll | cut -d/ -f4-`
						to=@executable_path/../$relative
						if [ $first != /usr/lib -a $first != /usr/X11 -a $first != /System/Library -a "`echo $ll | fgrep libcrypto`" = "" ]; then
							/usr/bin/install_name_tool -change $ll $to $pkgPath/$relative
						fi
					done
				fi
			fi
		done
	fi
}

rewritelibpaths () {
	# 
	# Fix package deps
	(cd "$package/Contents/Resources/lib/gtk-2.0/$gtk_version/loaders"
	for file in *.so; do
		echo "Rewriting dylib paths for $file..."
		fixlib "$file" "`pwd`"
	done
	)
	(cd "$package/Contents/Resources/lib/gtk-2.0/$gtk_version/engines"
	for file in *.so; do
		echo "Rewriting dylib paths for $file..."
		fixlib "$file" "`pwd`"
	done
	)
	(cd "$package/Contents/Resources/lib/gtk-2.0/$gtk_version/immodules"
	for file in *.so; do
		echo "Rewriting dylib paths for $file..."
		fixlib "$file" "`pwd`"
	done
	)
	(cd "$package/Contents/Resources/lib/gtk-2.0/$gtk_version/printbackends"
	for file in *.so; do
		echo "Rewriting dylib paths for $file..."
		fixlib "$file" "`pwd`"
	done
	)
	(cd "$package/Contents/Resources/lib/gdk-pixbuf-2.0/$gtk_version/loaders"
	for file in *.so; do
		echo "Rewriting dylib paths for $file..."
		fixlib "$file" "`pwd`"
	done
	)
	(cd "$package/Contents/Resources/lib/pango/$pango_version/modules"
	for file in *.so; do
		echo "Rewriting dylib paths for $file..."
		fixlib "$file" "`pwd`"
	done
	)
	(cd "$package/Contents/Resources/bin"
	for file in *; do
		echo "Rewriting dylib paths for $file..."
		fixlib "$file" "`pwd`"
	done
	cd ../lib
	for file in *.dylib; do
		echo "Rewriting dylib paths for $file..."
		fixlib "$file" "`pwd`"
	done
	)
}

PATHLENGTH=`echo $LIBPREFIX | wc -c`
if [ "$PATHLENGTH" -ge "50" ]; then
	# If the LIBPREFIX path is long enough to allow 
	# path rewriting, then do this.
	rewritelibpaths
fi

exit 0
