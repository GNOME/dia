#ifndef _DIA_CONFIG_H_
#define _DIA_CONFIG_H_
@TOP@

/* These get defined to the version numbers in configure.in */

#undef PACKAGE
#undef VERSION

/* Define to enable GNOME-specific code. */
#undef GNOME

#undef GNOME_PRINT

/* Define if your locale.h file contains LC_MESSAGES.  */
#undef HAVE_LC_MESSAGES

/* Define if you have the stpcpy function.  */
#undef HAVE_STPCPY

/* Define to 1 if NLS is requested.  */
#undef ENABLE_NLS

/* Define as 1 if you have catgets and don't want to use GNU gettext.  */
#undef HAVE_CATGETS

/* Define if you have the popt library (-lpopt).  */
#undef HAVE_LIBPOPT

/* Define if you have the libart_lgpg library (-lart_lgpl).  */
#undef HAVE_LIBART

/* define if you have libpng */
#undef HAVE_LIBPNG

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
#undef HAVE_GETTEXT

/* define to enable XIM support */
#undef USE_XIM

/* needed for intltool */
#undef GETTEXT_PACKAGE

@BOTTOM@
#endif /* !_DIA_CONFIG_H_ */

