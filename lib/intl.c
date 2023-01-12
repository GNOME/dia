#include "config.h"

#include <glib.h>

#include "intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* low numbers are better */
int
intl_score_locale (const char *locale)
{
  const char *const *names = g_get_language_names ();
  int i = 0;

  /* NULL is same as C locale */
  if (!locale) {
    while (names[i] != NULL)
      ++i;
    return i;
  }
  while (names[i] != NULL) {
    if (strcmp(names[i], locale) == 0)
      break;
    ++i;
  }
  if (names[i] == NULL) /* not found */
    i = G_MAXINT;
  return i;
}
