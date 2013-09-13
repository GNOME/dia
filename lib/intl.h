#ifndef INTL_H
#define INTL_H

#include <glib.h>

#ifdef ENABLE_NLS
#  include <glib/gi18n.h>
#else /* NLS is disabled */
#  define _(String) (String)
#  define N_(String) (String)
#  define NC_(Context, String) (String)
#  define gettext(String) (String)
#  define textdomain(Domain)
#  define bindtextdomain(Package, Directory)
#endif

int    intl_score_locale      (const gchar *locale);

#endif
