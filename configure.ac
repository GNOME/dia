dnl Process this -*- autoconf -*- file with autoconf to produce a 
dnl configure script.
AC_INIT(dia, 0.97+git, http://bugzilla.gnome.org/enter_bug.cgi?product=dia)
AC_CONFIG_SRCDIR(app/diagram.c)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME,AC_PACKAGE_VERSION)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PREREQ(2.50)
AC_REVISION($Revision: 1.288 $)

dnl Specify a header configuration file
AC_CONFIG_HEADERS(config.h)

AC_CANONICAL_HOST

case "$host" in
  *-*-mingw*)
    os_win32=yes
    NO_UNDEFINED=-no-undefined
    ;;
  *)
    os_win32=no
    NO_UNDEFINED=
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")
AC_SUBST(NO_UNDEFINED)

dnl do this check now, to keep autogen quiet.
AC_PROG_CC
dnl also c++
AC_PROG_CXX

dnl Initialize maintainer mode
AM_MAINTAINER_MODE

dnl Initialize libtool
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

dnl Build time sanity check
AM_SANITY_CHECK

dnl Checks for programs.
dnl AC_PROG_CC # already done 
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_ISC_POSIX

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(stddef.h fcntl.h unistd.h utime.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl Checks for library functions.
AC_CHECK_FUNCS(select strcspn strdup strtol)

dnl stuff for intltool
IT_PROG_INTLTOOL([0.35.0]) 

APP_LIBS=""
AC_SUBST(APP_LIBS)

dnl Checks for libraries

PKG_CHECK_MODULES(GMODULE, gmodule-2.0 gobject-2.0)
AC_SUBST(GMODULE_CFLAGS)
AC_SUBST(GMODULE_LIBS)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

GTK_MODULES="gtk+-2.0 >= 2.16.0 glib-2.0 >= 2.20.0 libxml-2.0 gdk-pixbuf-2.0 gthread-2.0 gmodule-2.0"
PKG_CHECK_MODULES(PANGOVERSION, pango >= 1.20.0 pangocairo, , 
                  AC_MSG_ERROR([Need Pango version 1.20.0 or higher]))

AC_ARG_WITH(freetype,
  AC_HELP_STRING([--without-freetype],[compile without FreeType support]),,with_freetype=yes)
if test "x$with_freetype" = "xyes"; then
PKG_CHECK_MODULES(PANGOFT2,pangoft2,have_pangoft2=true,have_pangoft2=false)
if test "$have_pangoft2" = "true"; then
  dnl On Solaris with Forte C, at least, need to link app/dia with -lfreetype.
  dnl It's not enough that -lpangoft2 implicitly pulls it in.
  have_freetype=false
  AC_CHECK_LIB(freetype,FT_Init_FreeType,have_freetype=true,have_freetype=false,`freetype-config --libs`)
  if test "$have_freetype" = "true"; then
    dnl Need 2.0.9, as a bug was fixed for us there.
    dnl However, freetype-config doesn't give a meaningful version, so we must
    dnl do it like this.
    AC_MSG_CHECKING([if FreeType version is 2.0.9 or higher])
    old_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS `freetype-config --cflags`"
    AC_TRY_CPP([#include <ft2build.h>
#include FT_FREETYPE_H
#if (FREETYPE_MAJOR*1000+FREETYPE_MINOR)*1000+FREETYPE_PATCH < 2000009
#error Freetype version too low.
#endif
],
        [AC_MSG_RESULT(yes)
	 FREETYPE_LIBS=`freetype-config --libs`
	 AC_SUBST(FREETYPE_LIBS)
	 FREETYPE_CFLAGS=`freetype-config --cflags`
	 AC_SUBST(FREETYPE_CFLAGS)
	 GTK_MODULES="$GTK_MODULES pangoft2"
	 AC_DEFINE(HAVE_FREETYPE,1,[Define if you have the FreeType2 library])]
    ,
	[AC_MSG_ERROR([Need FreeType library version 2.0.9 or higher])])
    CPPFLAGS="$old_CPPFLAGS"
  else
    AC_MSG_ERROR(Can't find FreeType library)
  fi
else
  AC_MSG_ERROR(Can't find PangoFT2 library)
fi
fi
AM_CONDITIONAL(WITH_FREETYPE, test "x$with_freetype" != "xno")

CFLAGS="$FREETYPE_CFLAGS $CFLAGS"

dnl Try for libart
PKG_CHECK_MODULES(LIBART,libart-2.0,have_libart=yes,have_libart=no)
if test "$have_libart" = "yes" ; then
   GTK_MODULES="$GTK_MODULES libart-2.0"
   AC_DEFINE(HAVE_LIBART,1,[Define if building with LIBART support])
fi

dnl Try for poppler
PKG_CHECK_MODULES(POPPLER,poppler,have_poppler=yes,have_poppler=no)
if test "$have_poppler" = "yes" ; then
   AC_DEFINE(HAVE_POPPLER,1,[Define if building with POPPLER support])
fi
AM_CONDITIONAL(HAVE_POPPLER, test "x$have_poppler" != "xno")

AC_MSG_CHECKING([whether GNOME specific code should be used])
AC_ARG_ENABLE(gnome,
	      AC_HELP_STRING([--enable-gnome],[enable gnome code]),,
              enable_gnome=no)
if test x"$enable_gnome" = "xyes" ; then
    GTK_MODULES="$GTK_MODULES libgnome-2.0 libgnomeui-2.0"
    AC_DEFINE(HAVE_GNOME,1,[Define if building with GNOME support])
    AC_DEFINE_UNQUOTED(GNOME_ICONDIR, "${prefix}/share/pixmaps", [GNOME icon directory])
    have_gnome=yes
else
    have_gnome=no
fi
AM_CONDITIONAL(HAVE_GNOME, [test "x$have_gnome" != "xno"])
AC_MSG_RESULT($GNOME)

dnl GTK2:     libgnomecanvas-2.0 ?

PKG_CHECK_MODULES(GTK,$GTK_MODULES)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

### --------------------------------------------------------------------------
### Check to see if GDK uses the quartz backend and if we can use 
### MacOSX integration
_gdk_tgt=`$PKG_CONFIG --variable=target gdk-2.0`
AM_CONDITIONAL([GDK_TARGET_QUARTZ], [test x$_gdk_tgt = xquartz])
if test "x$_gdk_tgt" = xquartz; then 
    PKG_CHECK_MODULES(GTK_MAC, gtk-mac-integration)
    AC_SUBST(GTK_MAC_LIBS)
    AC_SUBST(GTK_MAC_CFLAGS)
    AC_DEFINE(HAVE_MAC_INTEGRATION,1,[Define if building with gtk-mac-integration])
fi

AM_CONDITIONAL(HAVE_BONOBO,[test "true" = "false"])

dnl Look for Cairo
AC_ARG_WITH(cairo,
  AC_HELP_STRING([--with-cairo],[compile cairo plug-in]),,with_cairo=yes)
if test "x$with_cairo" != "xno"; then
  PKG_CHECK_MODULES(CAIROVERSION, cairo >= 1.6.0, , 
                    AC_MSG_ERROR([Need Cairo version 1.6.0 or higher]))
  PKG_CHECK_MODULES(CAIRO, cairo,have_cairo=yes,have_cairo=no)
  AC_SUBST(CAIRO_LIBS)
  AC_SUBST(CAIRO_CFLAGS)
  AC_DEFINE(HAVE_CAIRO,1,[Define if building with Cairo support])
fi
AM_CONDITIONAL(WITH_CAIRO, test "x$with_cairo" != "xno")

dnl with gtk+ >= 2.8 this is no longer optional
PKG_CHECK_MODULES(PANGOCAIRO,pangocairo, pangocairo_ok=yes, pangocairo_ok=no)
if test "x$pangocairo_ok" = "xyes"; then
  AC_DEFINE(HAVE_PANGOCAIRO_H,1,[Defined to render text with pangocairo not cairos toy API])
fi

dnl XSLT

# Check if XSLT is available at all
XSLT_DIR=""
PKG_CHECK_MODULES(XSLT, libxslt, have_xslt=true, have_xslt=false)
AM_CONDITIONAL(HAVE_XSLT, $have_xslt)
if test "$have_xslt" = "true"; then
  XSLT_DIR="xslt"
  XSLT_LIBS="`$PKG_CONFIG --libs libxslt`"
  XSLT_CFLAGS="`$PKG_CONFIG --cflags libxslt`"
fi

AC_SUBST(XSLT_DIR)
AC_SUBST(XSLT_LIBS)
AC_SUBST(XSLT_CFLAGS)

# Figure out a prefix for where to find libxslt at runtime.
AC_ARG_WITH(xslt-prefix, 
            AC_HELP_STRING([--with-xslt-prefix=DIR],[Specify where to find the optional XSLT libraries
                             (default=/usr/lib)]), 
	    libxslt_prefix=$withval,
	    libxslt_prefix=/usr/lib)
AC_DEFINE_UNQUOTED(LIBXSLT_PREFIX,"$libxslt_prefix", [Place to look for XSLT libs])

dnl
dnl libpng
dnl
dnl this likely already comes from GTK

dnl Something in here pollutes $LIBS with -lpng.  Make sure to
dnl avoid that.

old_LIBS="${LIBS}"

png_ok=no
AC_CHECK_HEADER(png.h, png_ok=yes, png_ok=no)
AC_CHECK_LIB(png, png_read_info, ,png_ok=no, -lz -lm)
if test "$png_ok" = yes; then
  AC_MSG_CHECKING([for png_structp in png.h])
  dnl Why can't AC_LINK_IFELSE take extra libs?
  old_LDFLAGS="${LDFLAGS}"
  LDFLAGS="${LDFLAGS} -lz -lm"
  AC_LINK_IFELSE([AC_LANG_SOURCE([
		  #include <png.h>
		  png_structp pp;
		  png_infop info; 
		  png_colorp cmap; 
		  main() { void*foo = png_create_read_struct; }])],
		  [png_ok=yes],
		  [png_ok=no])
  LDFLAGS="${old_LDFLAGS}"
  AC_MSG_RESULT($png_ok)
  if test "$png_ok" = yes; then
    PNG_LIBS='-lpng -lz -lm'
    AC_DEFINE(HAVE_LIBPNG,,[PNG library available])
  fi
fi
AC_SUBST(PNG_LIBS)

LIBS="${old_LIBS} ${PNG_LIBS}"

dnl AC_PROG_CXX does behave a bit strange, it is setting CXX=g++ as "last resort"
dnl but that does not solve the missing g++ issue. Make C++ optional:
cxx_ok=yes
if test "$CXX" = "g++"; then
  if test "$GXX" != "yes"; then
    cxx_ok=no
  fi
fi
AM_CONDITIONAL(HAVE_CPLUSPLUS, test "$cxx_ok" = yes)

dnl check for libEMF to provide Windows Metafile plug-in on Linux
AC_ARG_ENABLE([libemf],
	AS_HELP_STRING([--enable-libemf], [enable plugin for windows metafile support]),
	,
	enable_libemf=yes)

if test "$enable_libemf" = "yes"; then
	old_CPPFLAGS="${CPPFLAGS}"
	old_CFLAGS="${CFLAGS}"
	old_LIBS="${LIBS}"

	AC_CHECK_HEADER(libEMF/emf.h, emf_ok=yes, emf_ok=no)
	AC_CHECK_LIB(EMF, CreateEnhMetaFileWithFILEW, , emf_ok=no, -lstdc++)

	if test "$emf_ok" = yes; then
		LIBEMF_LIBS='-lEMF -lstdc++'
		AC_DEFINE(HAVE_LIBEMF,1,[libEMF library available])
	fi

	CPPFLAGS="${old_CPPFLAGS}"
	CFLAGS="${old_CFLAGS}"
	LIBS="${old_LIBS}"
else
	emf_ok=no
fi
AC_SUBST(LIBEMF_CFLAGS)
AC_SUBST(LIBEMF_LIBS)
AM_CONDITIONAL(WITH_LIBEMF, test "$emf_ok" = yes)

dnl Not necessarily installed somewhere in a typical place
AC_ARG_ENABLE([ogdf],
	AS_HELP_STRING([--enable-ogdf], [enable plugin using OGDF (www.ogdf.net)]),
	,
	enable_ogdf=yes)
if test "$enable_ogdf" = "yes"; then
	dnl FIXME: get prefix from command line
	ogdf_prefix=~/devel/OGDF
	CPPFLAGS="${CPPFLAGS} -I $ogdf_prefix"
	AC_LANG_PUSH([C++])
	AC_CHECK_HEADER(ogdf/basic/Graph.h, ogdf_ok=yes, ogdf_ok=no)
	AC_LANG_POP([C++])
	if test "$ogdf_ok" = yes; then
		OGDF_CFLAGS="-I ${ogdf_prefix}"
		if test "x$enable_debug" = "xno"; then
			OGDF_LIBS="-L${ogdf_prefix}/_release -lOGDF -lstdc++"
		else
			OGDF_LIBS="-L${ogdf_prefix}/_debug -lOGDF -lstdc++"
		fi
		AC_DEFINE(HAVE_OGDF,1,[OGDF library available])
	fi
else
	ogdf_ok=no
fi
AC_SUBST(OGDF_CFLAGS)
AC_SUBST(OGDF_LIBS)
AM_CONDITIONAL(HAVE_OGDF, test "$ogdf_ok" = yes)

dnl
dnl Debugging
dnl
AC_ARG_ENABLE(debug, 
  AC_HELP_STRING([--enable-debug=[no/minimum/yes]],[turn on debugging [default=minimum]]),
  ,
  enable_debug=minimum)

if test "x$enable_debug" = "xyes"; then
  DEBUG_FLAGS="-g -DENABLE_DEBUG"
else
  if test "x$enable_debug" = "xno"; then
    DEBUG_FLAGS=""
  else
    DEBUG_FLAGS=""
  fi
fi

dnl
dnl Deprecation
dnl
AC_ARG_ENABLE(deprecation, 
  AC_HELP_STRING([--enable-deprecation=[no,glib,pango,gtk,yes]],
		 [turn on glib/pango/gtk deprecations [default=no]]),
   ,
   enable_deprecation=no)
if test "x$enable_deprecation" != "xno"; then
  if test "x$enable_deprecation" = "xglib"; then 
    DEBUG_FLAGS="$DEBUG_FLAGS -DG_DISABLE_DEPRECATED"
  else
    if test "x$enable_deprecation" = "xpango"; then 
      DEBUG_FLAGS="$DEBUG_FLAGS -DPANGO_DISABLE_DEPRECATED"
    else
      if test "x$enable_deprecation" = "xgtk"; then 
        DEBUG_FLAGS="$DEBUG_FLAGS -DGTK_DISABLE_DEPRECATED"
      else
        DEBUG_FLAGS="$DEBUG_FLAGS -DG_DISABLE_DEPRECATED -DPANGO_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
      fi
    fi
  fi
  enable_deprecation=yes
fi

dnl Used for both
AC_SUBST(DEBUG_FLAGS)

dnl
dnl i18n
dnl

GETTEXT_PACKAGE=dia
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[Needed for glib gettext])

AC_CHECK_PROG(XGETTEXT, xgettext, xgettext, none)
if test "$XGETTEXT" = none ; then
   AC_MSG_ERROR([xgettext program not found: $XGETTEXT.])
fi

AM_GLIB_GNU_GETTEXT


dnl INTLOBJS doesn't seem to always get subst'ed, but I believe it's
dnl idempotent
AC_SUBST(INTLOBJS)

dnl
dnl Locate the gnome-xml library
dnl
dnl GTK2: remove support for libxml1. move check for libxml2 in 
dnl AM_PATH_GTK_2_0's fourth argument (module is libxml-2.0)

found_libxml=false
if test $found_libxml = false; then
  AC_CHECK_PROG(XML2_CONFIG, xml2-config, xml2-config)
  if test "x$XML2_CONFIG" != x ; then
    AC_MSG_CHECKING(for libxml >= 2.3.9)
    vers=`$XML2_CONFIG --version | sed -e "s/libxml //" | awk 'BEGIN {FS=".";} { printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`

    if test "$vers" -ge 2003009; then
      AC_MSG_RESULT(found)
      found_libxml=true
      XML_LIBS="`$XML2_CONFIG --libs`"
      XML_CFLAGS="`$XML2_CONFIG --cflags`"

      if test "$enable_gnome_print" = "yes"; then
          gnome-config --cflags print | \
          grep gnome-xml 2>&1 >/dev/null && \
          AC_MSG_ERROR(libxml2 found but gnome-print uses libxml1 ; this is a conflict.\nPlease do either provide libxml1 or disable gnome-print.)
      fi
      echo $GNOME_CFLAGS | \
      grep gnome-xml 2>&1 >/dev/null && \
      AC_MSG_ERROR(libxml2 found but one GNOME component uses libxml1 ; this is a conflict.\nPlease do either provide libxml1 or disable the offending GNOME component.)
        
    else
      AC_MSG_RESULT(not found)
    fi
  fi
fi

if test $found_libxml = false; then
AC_CHECK_PROG(XML_CONFIG, xml-config, xml-config)
if test "x$XML_CONFIG" != x ; then
  AC_MSG_CHECKING(for libxml >= 1.8.14)
  vers=`$XML_CONFIG --version | sed -e "s/libxml //" | awk 'BEGIN {FS=".";} { printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`

  dnl 1.18.14 is required for xmlUseNewParser()    
  if test "$vers" -ge 1008014; then
    AC_MSG_RESULT(found)
    found_libxml=true
    XML_LIBS="`$XML_CONFIG --libs`"
    XML_CFLAGS="`$XML_CONFIG --cflags`"
  else
    AC_MSG_RESULT(not found)
  fi
fi
fi

if test $found_libxml = false; then
    AC_MSG_ERROR(Neither libxml1 or libxml2 found. You must install one (libxml1, preferably))
fi

AC_SUBST(XML_LIBS)
AC_SUBST(XML_CFLAGS)

CFLAGS="$XML_CFLAGS $CFLAGS"

dnl LibZ
AC_CHECK_HEADERS(zlib.h,
    AC_CHECK_LIB(z, gzread,[
        AC_DEFINE(HAVE_LIBZ,, we have the libz compression library)
        if test "x${Z_DIR}" != "x"; then
            Z_CFLAGS="-I${Z_DIR}/include"
            Z_LIBS="-L${Z_DIR}/lib -lz"
        else
            Z_LIBS="-lz"
        fi]))

AC_SUBST(Z_LIBS)

CFLAGS="${CFLAGS} ${Z_CFLAGS}"
LIBS="${LIBS} ${Z_LIBS}"


dnl Path to UI resources
AC_SUBST(uidatadir, '${pkgdatadir}/ui')

dnl Platform-specific fixes.
dnl

if test "$os_win32" = "no"; then

  dnl Sun Forte C provides isinf() in the unbundled -lsunmath. Solaris
  dnl declares finite() in ieeefp.h. It's preferrable not to link
  dnl to -lsunmath as it's not present on all machines.
  have_isinf=no
  AC_CHECK_HEADERS(math.h,
          AC_CHECK_LIB(m,isinf,
                  have_isinf=yes))

  if test "$have_isinf" = "no"; then
      AC_CHECK_HEADER(ieeefp.h,
               [AC_CHECK_FUNC(finite,
                         AC_DEFINE(HAVE_IEEEFP_H,1,
                                 [Define if finite() is defined in ieeefp.h]),
                         AC_MSG_ERROR([Can't find a definition of neither
                                       finite nor isinf]))],
               [AC_MSG_ERROR(Can't find neither a definition of
	                     isinf nor ieeefp.h)])
  else
      AC_DEFINE([HAVE_ISINF], 1,
                [Define if the isinf() function is available])
  fi

fi

dnl Alpha needs -mieee or we get a segfault
dnl This shouldn't be needed but is here until a real solution is found
case "$host" in
  alpha*-*-linux*)
    CFLAGS="$CFLAGS -mieee"
    ;;
esac

if test "$GCC" = yes; then
  dnl Relieve register pressure on i386. Other architectures may become unable
  dnl to debug, and usually have more registers anyway.
  dnl case "$host" in 
  dnl  i?86-pc-*)
  dnl    DIA_CHECK_CFLAG(-fomit-frame-pointer)
  dnl    ;;
  dnl esac
  dnl (it looks like gdb is very confused by this now even on x86. We might be
  dnl getting poor stack traces with bug-buddy because of this)

  DIA_CHECK_CFLAG(-Wall)
  dnl DIA_CHECK_CFLAG(-Wunused)
  dnl On win32 we still need c89/c90 w/o gnu extension due to msvc6 use.
  dnl But --std=c89 is very strict, e.g. it prohibits single line comments.
  dnl The following -Werror= reject constructs which break the Windows build
  dnl DIA_CHECK_CFLAG(--std=c89)
  DIA_CHECK_CFLAG(-Werror=declaration-after-statement)
  dnl I'd rather like to enable this but it fails on isnan() with -std=c89
  DIA_CHECK_CFLAG(-Werror=implicit-function-declaration)
  dnl Premature optimization is the root of all evil.  Uncomment this if
  dnl you want ti figure out functions that should have been inlined but aint.
  dnl DIA_CHECK_CFLAG(-Winline)
  DIA_CHECK_CFLAG(-Wmissing-prototypes)
  DIA_CHECK_CFLAG(-Wmissing-declarations)
  DIA_CHECK_CFLAG(-finline-functions)
  DIA_CHECK_CFLAG(-fstrict-aliasing)
  dnl DIA_CHECK_CFLAG(-Wshadow)
  DIA_CHECK_CFLAG(-Wpointer-arith)
  dnl DIA_CHECK_CFLAG(-Wswitch-enum)
  dnl For Dia the following warning is producing pure noise, switch it off
  DIA_CHECK_CFLAG(-Wno-enum-compare)
  DIA_CHECK_CFLAG(-Winit-self)
  DIA_CHECK_CFLAG(-Wformat-nonliteral)
fi

AM_CONDITIONAL(HAVE_GCC, test "$GCC" = yes)

dnl Do we have the db2html script to build the documentation?
AC_ARG_ENABLE(db2html,
  AC_HELP_STRING([--enable-db2html],[don't rebuild documentation]),,enable_db2html=no)
AC_PATH_PROG(DB2HTML, db2html, no)
AM_CONDITIONAL(HAVE_DB2HTML, 
               test "x$enable_db2html" != xno && test "x$DB2HTML" != xno)

dnl Only enable man page generation if we have db2man (in maintainer mode)


dnl Have to have the nwalsh docbook.xsl, which does manpages.
dnl RedHat installs it in a version-numbered place.  Grumpf.
AC_PATH_PROG(XSLTPROC, xsltproc, no)
AM_CONDITIONAL(HAVE_DB2MAN, test "x$XSLTPROC" != xno)
if test "x$XSLTPROC" != "xno" ; then
  xsltproc=true
else
  xsltproc=false
fi
AM_CONDITIONAL(HAVE_XSLTPROC, $xsltproc)
AC_SUBST(DB2MAN, "http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl")

dnl Handling for Python
AC_ARG_WITH(python,
  AC_HELP_STRING([--with-python],[compile python plug-in]),,with_python=no)

if test "x$with_python" != xno; then
  AM_PATH_PYTHON(2.3.0)
  AM_CHECK_PYMOD(gtk,,,[AC_MSG_RESULT(could not find pygtk module)])
  AM_CHECK_PYTHON_HEADERS
  AM_CHECK_PYTHON_LIB

  if test -z "$PYTHON_INCLUDES" || test -z "$PYTHON_LIBS"; then
    AC_MSG_ERROR([could not find files required to build python plugin])
  fi
fi
AM_CONDITIONAL(WITH_PYTHON, test "x$with_python" != "xno")

dnl Handling of Python bindings with SWIG
AC_ARG_WITH(swig,
  AC_HELP_STRING([--with-swig],[generate python bindings with swig]),,with_swig=no)

if test "x$with_swig" != "xno"; then
  AC_PROG_SWIG(1.3.31)
  if test -z "$SWIG_LIB"; then
    AC_MSG_ERROR([could not find SWIG required to generate the bindings])
  else
    AC_MSG_RESULT([SWIG at $SWIG_LIB])
  fi
fi
AM_CONDITIONAL(WITH_SWIG, test "x$with_swig" != "xno")



dnl
dnl html, pdf and postscript documentation
dnl
AC_ARG_WITH(hardbooks,
  AC_HELP_STRING([--with-hardbooks],[compile html, pdf and postscript documentation]),,with_hardbooks=no)
AC_PATH_PROG(DBLATEX,dblatex,no)

dnl
dnl use jw only if we don't find dblatex
dnl
AC_PATH_PROG(JW,jw,no)
if  test "x$dblatex" = "xno"; then
 AM_CONDITIONAL(WITH_JW, test "x$jw" != "xno")	
else
 AM_CONDITIONAL(WITH_JW, test "xno" != "xno")
fi
hardbook_ok=no
xsltproc_ok=no
if test "x$with_hardbooks" = "xno"; then
  AM_CONDITIONAL(WITH_JW, test "xno" != "xno")
  AM_CONDITIONAL(WITH_HTMLDOC, test "xno" != "xno")
  AM_CONDITIONAL(WITH_PDFDOC, test "xno" != "xno")
  AM_CONDITIONAL(WITH_PSDOC, test "xno" != "xno")
else
  AM_CONDITIONAL(WITH_HTMLDOC, test "x$xsltproc" != "xno")
  AM_CONDITIONAL(WITH_PDFDOC, test "x$dblatex" != "xno")
  AM_CONDITIONAL(WITH_PSDOC, test "x$dblatex" != "xno")
  if test "x$dblatex" != "xno" ; then
    hardbook_ok=yes
  else 
    if test "x$jw" != "xno" ; then
      hardbook_ok=yes
    fi
  fi
  if test "x$xsltproc" != "xno" ; then
    xsltproc_ok=yes
  fi
fi


AC_OUTPUT(
Makefile
lib/Makefile
lib/pixmaps/Makefile
po/Makefile.in
objects/Makefile
objects/AADL/Makefile
objects/chronogram/Makefile
objects/custom/Makefile
objects/custom_lines/Makefile
objects/ER/Makefile
objects/flowchart/Makefile
objects/FS/Makefile
objects/GRAFCET/Makefile
objects/Istar/Makefile
objects/Jackson/Makefile
objects/KAOS/Makefile
objects/Misc/Makefile
objects/network/Makefile
objects/SADT/Makefile
objects/standard/Makefile
objects/UML/Makefile
objects/Database/Makefile
shapes/Makefile
shapes/Assorted/Makefile
shapes/BPMN/Makefile
shapes/ChemEng/Makefile
shapes/Circuit/Makefile
shapes/Cisco/Makefile
shapes/Civil/Makefile
shapes/Contact/Makefile
shapes/Cybernetics/Makefile
shapes/Electric/Makefile
shapes/flowchart/Makefile
shapes/Gane_and_Sarson/Makefile
shapes/jigsaw/Makefile
shapes/Lights/Makefile
shapes/Logic/Makefile
shapes/Map/Makefile
shapes/Map/Isometric/Makefile
shapes/Misc/Makefile
shapes/MSE/Makefile
shapes/network/Makefile
shapes/Pneumatic/Makefile
shapes/SDL/Makefile
shapes/Shape_Design/Makefile
shapes/sybase/Makefile
app/Makefile
app/pixmaps/Makefile
bindings/Makefile
samples/Makefile
sheets/Makefile
sheets/ER/Makefile
sheets/GRAFCET/Makefile
sheets/Istar/Makefile
sheets/Jackson/Makefile
sheets/KAOS/Makefile
sheets/UML/Makefile
plug-ins/Makefile
plug-ins/cgm/Makefile
plug-ins/cairo/Makefile
plug-ins/drs/Makefile
plug-ins/dxf/Makefile
plug-ins/hpgl/Makefile
plug-ins/layout/Makefile
plug-ins/libart/Makefile
plug-ins/metapost/Makefile
plug-ins/pixbuf/Makefile
plug-ins/postscript/Makefile
plug-ins/pstricks/Makefile
plug-ins/pgf/Makefile
plug-ins/pdf/Makefile
plug-ins/python/Makefile
plug-ins/shape/Makefile
plug-ins/stress/Makefile
plug-ins/svg/Makefile
plug-ins/vdx/Makefile
plug-ins/wmf/Makefile
plug-ins/wpg/Makefile
plug-ins/xfig/Makefile
plug-ins/xslt/Makefile
tests/Makefile
tests/exports/Makefile
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/16x16/apps/Makefile
data/icons/22x22/Makefile
data/icons/22x22/apps/Makefile
data/icons/24x24/Makefile
data/icons/24x24/apps/Makefile
data/icons/32x32/Makefile
data/icons/32x32/apps/Makefile
data/icons/48x48/Makefile
data/icons/48x48/apps/Makefile
data/icons/scalable/Makefile
data/icons/scalable/apps/Makefile
doc/Makefile
doc/de/Makefile
doc/en/Makefile
doc/eu/Makefile
doc/pl/Makefile
doc/fr/Makefile
installer/Makefile
installer/macosx/Makefile
installer/macosx/Resources/Makefile
installer/macosx/Resources/etc/Makefile
installer/macosx/Resources/etc/fonts/Makefile
installer/macosx/Resources/themes/Makefile
installer/macosx/Resources/themes/CL/Makefile
installer/macosx/Resources/themes/CL/gtk-2.0/Makefile
installer/macosx/Resources/themes/CL/gtk-2.0/Scrollbars/Makefile
installer/macosx/Resources/themes/CL/gtk-2.0/Scrollbars_1/Makefile
installer/macosx/Resources/themes/CL/gtk-2.0/Scrollbars_6/Makefile
installer/win32/Makefile
installer/win32/locale/Makefile
dia.keys
)

dnl GTK2: simplify this, we will have much less options.
echo "

Configuration:
	Source code location:	${srcdir}
	Compiler:		${CC}
	can C++:		${cxx_ok}

	Gnome support:				 ${have_gnome}
	Python support:				 ${with_python}
	Libart support (PNG export):		 ${have_libart}
	Cairo support (PNG, PDF, print):	 ${with_cairo}
	Dia Python bindings with SWIG		 ${with_swig}
	PDF import with poppler (experimental)   ${have_poppler}
	WMF plug-in with libEMF:                 ${emf_ok}
	Layout plug-in with OGDF:                ${ogdf_ok}
	html doc:                                ${xsltproc_ok}
	pdf and ps doc:                          ${hardbook_ok}
	Enable deprecations (disable deprecated) ${enable_deprecation}
"

echo "Now type make to build dia..."
