#ifndef DIAVAR_H
#define DIAVAR_H

#include <glib.h>

#ifdef G_OS_WIN32
#  ifdef LIBDIA_COMPILATION
#    define DIAVAR __declspec(dllexport)
#  else  /* !LIBDIA_COMPILATION */
#    define DIAVAR __declspec(dllimport)
#  endif /* !LIBDIA_COMPILATION */
#else  /* !G_OS_WIN32 */
#  /* DONT: define DIAVAR extern */
#  define DIAVAR /* empty */
#  /* extern and __declspec() are orthogonal - otherwise there wont be a difference between
#   * the header declared variable and the one defined in the implementation. At least clang-cl
#   * code generation would create mutliple definitions, which later prohibit linking.
#   */
#endif

#endif
