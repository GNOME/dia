/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <glib.h>

#include "load_sheet.h"
#include "shape_info.h"

int get_version(void) {
  return 0;
}

GList *sheets = NULL;

static void
load_sheets_from_dir(const gchar *directory) {
  DIR *dp;
  struct dirent *dirp;
  struct stat statbuf;
  
  dp = opendir(directory);
  if (dp == NULL) {
    return;
  }
  while ( (dirp = readdir(dp)) ) {
    gchar *filename = g_strconcat(directory, G_DIR_SEPARATOR_S,
				  dirp->d_name, NULL);
    Sheet *sheet = NULL;
  
    if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) {
      g_free(filename);
      continue;
    }
    /* filter out non directories ... */
    if (stat(filename, &statbuf) < 0) {
      g_free(filename);
      continue;
    }
    if (!S_ISDIR(statbuf.st_mode)) {
      g_free(filename);
      continue;
    }
    sheet = load_custom_sheet(filename, dirp->d_name);
    g_free(filename);
    if (sheet)
      sheets = g_list_append(sheets, sheet);
  }
  closedir(dp);
}

void register_objects(void) {
  char *shape_path;
  char *home_dir;

  home_dir = g_get_home_dir();
  if (home_dir) {
    home_dir = g_strconcat(home_dir, G_DIR_SEPARATOR_S, ".dia_shapes", NULL);
    load_sheets_from_dir(home_dir);
    g_free(home_dir);
  }

  shape_path = getenv("DIA_SHAPE_PATH");
  if (shape_path) {
    char **dirs = g_strsplit(shape_path, G_SEARCHPATH_SEPARATOR_S, 0);
    int i;

    for (i = 0; dirs[i] != NULL; i++)
      load_sheets_from_dir(dirs[i]);
    g_strfreev(dirs);
  } else {
    load_sheets_from_dir(DIA_SHAPEDIR);
  }
}

void register_sheets(void) {
  GList *tmp;

  for (tmp = sheets; tmp; tmp = tmp->next)
    register_sheet((Sheet *)tmp->data);
}
