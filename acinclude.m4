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
