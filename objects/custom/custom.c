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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <glib.h>

#include "custom.h"
#include "sheet.h"
#include "shape_info.h"
#include "dia_dirs.h"

static void load_shapes_from_tree(const gchar *directory)
{
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
    gchar *p;

    if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) {
      g_free(filename);
      continue;
    }

    if (stat(filename, &statbuf) < 0) {
      g_free(filename);
      continue;
    }
    if (S_ISDIR(statbuf.st_mode)) {
      load_shapes_from_tree(filename);
      g_free(filename);
      continue;
    }
    /* if it's not a directory, then it must be a .shape file */
    if (!S_ISREG(statbuf.st_mode) || (strlen(dirp->d_name) < 6)) {
      g_free(filename);
      continue;
    }
    

    p = dirp->d_name + strlen(dirp->d_name) - 6;
    if (0==strcmp(".shape",p)) {
      ObjectType *ot;
      SheetObject *so;

      if (!custom_object_load(filename,&ot,&so)) {
	g_warning("could not load shape file %s",filename);
      } else {
	g_assert(ot); 
	g_assert(so); 
	g_assert(so->user_data);
	/* maybe we ought to save so->user_data somewhere ? */
	g_free(so); /* that's a bit unfortunate, because 
		       we'll rebuild one soon */
	object_register_type(ot);
      }
    }
    g_free(filename);
  }
  closedir(dp);
}


void
custom_register_objects(void)
{
  char *shape_path;
  char *home_dir;

  home_dir = g_get_home_dir();
  if (home_dir) {
    home_dir = g_strconcat(home_dir, G_DIR_SEPARATOR_S, ".dia",
			   G_DIR_SEPARATOR_S, "shapes", NULL);
    
    load_shapes_from_tree(home_dir);
    g_free(home_dir);
  }

  shape_path = getenv("DIA_SHAPE_PATH");
  if (shape_path) {
    char **dirs = g_strsplit(shape_path, G_SEARCHPATH_SEPARATOR_S, 0);
    int i;

    for (i = 0; dirs[i] != NULL; i++)
      load_shapes_from_tree(dirs[i]);
    g_strfreev(dirs);
  } else {
    char *thedir = dia_get_data_directory("shapes");
    load_shapes_from_tree(thedir);
    g_free(thedir);
  }
}

void custom_object_new (ShapeInfo *info,
                        ObjectType **otype,
                        SheetObject **sheetobj);




gboolean
custom_object_load(gchar *filename, ObjectType **otype,
		   SheetObject **sheetobj)
{
  ShapeInfo *info;

  if (!filename)
    return FALSE;
  info = shape_info_load(filename);
  /*g_assert(info);*/
  if (!info) {
    *otype = NULL; 
    *sheetobj = NULL;
    return FALSE;
  }
  custom_object_new(info, otype, sheetobj);
  return TRUE;
}
