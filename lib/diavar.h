#ifndef DIAVAR_H
#define DIAVAR_H

#include <glib.h>

#ifdef G_OS_WIN32
#  ifdef LIBDIA_COMPILATION
#    define DIAVAR __declspec(dllexport)
#  else  /* !LIBDIA_COMPILATION */
#    define DIAVAR extern __declspec(dllimport)
#  endif /* !LIBDIA_COMPILATION */
#else  /* !G_OS_WIN32 */
#  define DIAVAR extern
#endif

#endif
