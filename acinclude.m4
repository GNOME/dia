dnl Check if the C compiler accepts a certain C flag, and if so adds it to
dnl CFLAGS
AC_DEFUN(DIA_CHECK_CFLAG, [
  AC_MSG_CHECKING(if C compiler accepts $1)
  save_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS $1"
  AC_TRY_COMPILE([#include <stdio.h>], [printf("hello");],
    [AC_MSG_RESULT(yes)],dnl
    [AC_MSG_RESULT(no)
     CFLAGS="$save_CFLAGS"])
])

## ------------------------
## Python file handling
## From Andrew Dalke
## Updated by James Henstridge
## ------------------------

# AM_PATH_PYTHON([MINIMUM-VERSION])

# Adds support for distributing Python modules and packages.  To
# install modules, copy them to $(pythondir), using the python_PYTHON
# automake variable.  To install a package with the same name as the
# automake package, install to $(pkgpythondir), or use the
# pkgpython_PYTHON automake variable.

# The variables $(pyexecdir) and $(pkgpyexecdir) are provided as
# locations to install python extension modules (shared libraries).
# Another macro is required to find the appropriate flags to compile
# extension modules.

# If your package is configured with a different prefix to python,
# users will have to add the install directory to the PYTHONPATH
# environment variable, or create a .pth file (see the python
# documentation for details).

# If the MINIUMUM-VERSION argument is passed, AM_PATH_PYTHON will
# cause an error if the version of python installed on the system
# doesn't meet the requirement.  MINIMUM-VERSION should consist of
# numbers and dots only.


AC_DEFUN([AM_PATH_PYTHON],
 [
  dnl Find a version of Python.  I could check for python versions 1.4
  dnl or earlier, but the default installation locations changed from
  dnl $prefix/lib/site-python in 1.4 to $prefix/lib/python1.5/site-packages
  dnl in 1.5, and I don't want to maintain that logic.

  if test -z "$PYTHON"; then
     AC_PATH_PROGS(PYTHON, python python2.1 python2.0 python1.6 python1.5)
  fi

  dnl should we do the version check?
  ifelse([$1],[],,[
    AC_MSG_CHECKING(if Python version >= $1)
    changequote(<<, >>)dnl
    prog="
import sys, string
minver = '$1'
pyver = string.split(sys.version)[0]  # first word is version string
# split strings by '.' and convert to numeric
minver = map(string.atoi, string.split(minver, '.'))
if hasattr(sys, 'version_info'):
    pyver = sys.version_info[:3]
else:
    pyver = map(string.atoi, string.split(pyver, '.'))
# we can now do comparisons on the two lists:
if pyver >= minver:
        sys.exit(0)
else:
        sys.exit(1)"
    changequote([, ])dnl
    if $PYTHON -c "$prog" 1>&AC_FD_CC 2>&AC_FD_CC
    then
      AC_MSG_RESULT(okay)
    else
      AC_MSG_ERROR(too old)
    fi
  ])

  AC_MSG_CHECKING([local Python configuration])

  dnl Query Python for its version number.  Getting [:3] seems to be
  dnl the best way to do this; it's what "site.py" does in the standard
  dnl library.  Need to change quote character because of [:3]

  AC_SUBST(PYTHON_VERSION)
  changequote(<<, >>)dnl
  PYTHON_VERSION=`$PYTHON -c "import sys; print sys.version[:3]"`
  changequote([, ])dnl


  dnl Use the values of $prefix and $exec_prefix for the corresponding
  dnl values of PYTHON_PREFIX and PYTHON_EXEC_PREFIX.  These are made
  dnl distinct variables so they can be overridden if need be.  However,
  dnl general consensus is that you shouldn't need this ability.

  AC_SUBST(PYTHON_PREFIX)
  PYTHON_PREFIX='${prefix}'

  AC_SUBST(PYTHON_EXEC_PREFIX)
  PYTHON_EXEC_PREFIX='${exec_prefix}'

  dnl At times (like when building shared libraries) you may want
  dnl to know which OS platform Python thinks this is.

  AC_SUBST(PYTHON_PLATFORM)
  PYTHON_PLATFORM=`$PYTHON -c "import sys; print sys.platform"`


  dnl Set up 4 directories:

  dnl pythondir -- where to install python scripts.  This is the
  dnl   site-packages directory, not the python standard library
  dnl   directory like in previous automake betas.  This behaviour
  dnl   is more consistent with lispdir.m4 for example.
  dnl
  dnl Also, if the package prefix isn't the same as python's prefix,
  dnl then the old $(pythondir) was pretty useless.

  AC_SUBST(pythondir)
  pythondir=$PYTHON_PREFIX"/lib/python"$PYTHON_VERSION/site-packages

  dnl pkgpythondir -- $PACKAGE directory under pythondir.  Was
  dnl   PYTHON_SITE_PACKAGE in previous betas, but this naming is
  dnl   more consistent with the rest of automake.
  dnl   Maybe this should be put in python.am?

  AC_SUBST(pkgpythondir)
  pkgpythondir=\${pythondir}/$PACKAGE

  dnl pyexecdir -- directory for installing python extension modules
  dnl   (shared libraries)  Was PYTHON_SITE_EXEC in previous betas.

  AC_SUBST(pyexecdir)
  pyexecdir=$PYTHON_EXEC_PREFIX"/lib/python"$PYTHON_VERSION/site-packages

  dnl pkgpyexecdir -- $(pyexecdir)/$(PACKAGE)
  dnl   Maybe this should be put in python.am?

  AC_SUBST(pkgpyexecdir)
  pkgpyexecdir=\${pyexecdir}/$PACKAGE

  AC_MSG_RESULT([looks good])
])

dnl AM_CHECK_PYMOD(MODNAME [,SYMBOL [,ACTION-IF-FOUND [,ACTION-IF-NOT-FOUND]]])
dnl Check if a module containing a given symbol is visible to python.
AC_DEFUN(AM_CHECK_PYMOD,
[AC_REQUIRE([AM_PATH_PYTHON])
py_mod_var=`echo $1['_']$2 | sed 'y%./+-%__p_%'`
AC_MSG_CHECKING(for ifelse([$2],[],,[$2 in ])python module $1)
AC_CACHE_VAL(py_cv_mod_$py_mod_var, [
ifelse([$2],[], [prog="
import sys
try:
        import $1
except ImportError:
        sys.exit(1)
except:
        sys.exit(0)
sys.exit(0)"], [prog="
import $1
$1.$2"])
if $PYTHON -c "$prog" 1>&AC_FD_CC 2>&AC_FD_CC
  then
    eval "py_cv_mod_$py_mod_var=yes"
  else
    eval "py_cv_mod_$py_mod_var=no"
  fi
])
py_val=`eval "echo \`echo '$py_cv_mod_'$py_mod_var\`"`
if test "x$py_val" != xno; then
  AC_MSG_RESULT(yes)
  ifelse([$3], [],, [$3
])dnl
else
  AC_MSG_RESULT(no)
  ifelse([$4], [],, [$4
])dnl
fi
])

dnl a macro to check for ability to create python extensions
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES
AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for headers required to compile python extensions)
dnl deduce PYTHON_INCLUDES
py_prefix=`$PYTHON -c "import sys; print sys.prefix"`
py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
dnl check if the headers exist:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
AC_TRY_CPP([#include <Python.h>],dnl
[AC_MSG_RESULT(found)
$1],dnl
[AC_MSG_RESULT(not found)
$2])
CPPFLAGS="$save_CPPFLAGS"
])

AC_DEFUN([AM_CHECK_PYTHON_LIB],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_REQUIRE([AM_CHECK_PYTHON_HEADERS])

AC_MSG_CHECKING(for libpython${PYTHON_VERSION}.a)

py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`

py_makefile="${py_exec_prefix}/lib/python${PYTHON_VERSION}/config/Makefile"
if test -f "$py_makefile"; then
dnl extra required libs
  py_localmodlibs=`sed -n -e 's/^LOCALMODLIBS=\(.*\)/\1/p' $py_makefile`
  py_basemodlibs=`sed -n -e 's/^BASEMODLIBS=\(.*\)/\1/p' $py_makefile`
  py_other_libs=`sed -n -e 's/^LIBS=\(.*\)/\1/p' $py_makefile`

dnl now the actual libpython
  if test -e "${py_exec_prefix}/lib/python${PYTHON_VERSION}/config/libpython${PYTHON_VERSION}.a"; then
    PYTHON_LIBS="-L${py_exec_prefix}/lib/python${PYTHON_VERSION}/config -lpython${PYTHON_VERSION} $py_localmodlibs $py_basemodlibs $py_other_libs"
    AC_MSG_RESULT(found)
  else
    AC_MSG_RESULT(not found)
  fi
else
  AC_MSG_RESULT(not found)
fi
AC_SUBST(PYTHON_LIBS)])
