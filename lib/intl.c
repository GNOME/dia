#include <config.h>

#include "intl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Compare the given locale (name) with the locale from the
 * envoronment, and return a number on how preferable the
 * given locale is.
 *
 * Lower numbers are better.
 *
 * Example:
 *
 * If the environment has set "de_CH" (swiss flavor
 * of the german locale) as locale then intl_score_locale("C")
 * will return a score of 2, because most prefered would be
 * "de_CH" (= 0), next best would be de (= 1) and "C" is the
 * least favored (= 2).
 */
int
intl_score_locale(const gchar *locale)
{
  const gchar * const *names = g_get_language_names ();
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
