dnl Check checks whether dlsym (if present) requires a leading underscore.
dnl Written by Dan Hagerty <hag@ai.mit.edu> for scsh-0.5.0.
AC_DEFUN(DIA_DLSYM_USCORE, [
  AC_MSG_CHECKING(for underscore before symbols)
  AC_CACHE_VAL(dia_cv_uscore,[
    echo "main(){int i=1;}
    fnord(){int i=23; int ltuae=42;}" > conftest.c
    ${CC} conftest.c > /dev/null
    if (nm a.out | grep _fnord) > /dev/null; then
      dia_cv_uscore=yes
    else
      dia_cv_uscore=no
    fi])
  AC_MSG_RESULT($dia_cv_uscore)
  rm -f conftest.c a.out

  if test $dia_cv_uscore = yes; then
    AC_DEFINE(USCORE)

    if test $ac_cv_func_dlopen = yes -o $ac_cv_lib_dl_dlopen = yes ; then
	AC_MSG_CHECKING(whether dlsym always adds an underscore for us)
	AC_CACHE_VAL(dia_cv_dlsym_adds_uscore,AC_TRY_RUN( [
#include <dlfcn.h>
#include <stdio.h>
fnord() { int i=42;}
main() { void *self, *ptr1, *ptr2; self=dlopen(NULL,RTLD_LAZY);
    if(self) { ptr1=dlsym(self,"fnord"); ptr2=dlsym(self,"_fnord");
    if(ptr1 && !ptr2) exit(0); } exit(1); } 
], [dia_cv_dlsym_adds_uscore=yes
	AC_DEFINE(DLSYM_ADDS_USCORE) ], dia_cv_dlsym_adds_uscore=no,
	dia_cv_dlsym_adds_uscore=no))

        AC_MSG_RESULT($dia_cv_dlsym_adds_uscore)
    fi
  fi
])
