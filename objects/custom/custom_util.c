/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Objects -- objects defined in XML rather than C.
 * Copyright (C) 1999 James Henstridge.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include "custom_util.h"

gchar *
custom_get_relative_filename(const gchar *current, const gchar *relative)
{
  gchar *dirname, *tmp;

  g_return_val_if_fail(current != NULL, NULL);
  g_return_val_if_fail(relative != NULL, NULL);

  if (g_path_is_absolute(relative))
    return g_strdup(relative);
  dirname = g_path_get_dirname(current);
  tmp = g_build_filename(dirname, relative, NULL);
  g_free(dirname);
  return tmp;
}
