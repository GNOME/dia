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

#include "intl.h"
#include "object.h"
#include "sheet.h"

#include "shape_info.h"

void custom_object_new (ShapeInfo *info,
			ObjectType **otype,
			SheetObject **sheetobj);

int get_version(void) {
  return 0;
}

/* create a shape sheet from a directory of XML shape descriptions */
static Sheet *
create_custom_sheet(const gchar *directory, const gchar *name)
{
  DIR *dp;
  struct dirent *dirp;
  Sheet *sheet = NULL;

  dp = opendir(directory);
  if (dp == NULL) {
    return;
  }
  while ( (dirp = readdir(dp)) ) {
    int len = strlen(dirp->d_name);
    if ((len > 4 && !strcmp(".xml", &dirp->d_name[len-4])) ||
	(len > 6 && !strcmp(".shape", &dirp->d_name[len-6]))) {
      gchar *filename = g_strconcat(directory, G_DIR_SEPARATOR_S,
				    dirp->d_name, NULL);
      ShapeInfo *info = shape_info_load(filename);
      ObjectType  *obj_type;
      SheetObject *sheet_obj;

      g_free(filename);
      if (!info)
	continue;

      shape_info_print(info); /* debugging ... */
      custom_object_new(info, &obj_type, &sheet_obj); /* create new type */

      object_register_type(obj_type);

      /* register the sheet object */
      if (!sheet)
	sheet = new_sheet(g_strdup(_(name)),
			  NULL);
      sheet_append_sheet_obj(sheet, sheet_obj);
    }
  }
  closedir(dp);
  return sheet;
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
    sheet = create_custom_sheet(filename, dirp->d_name);
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
