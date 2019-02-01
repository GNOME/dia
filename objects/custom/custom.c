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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>

#include "sheet.h"
#include "shape_info.h"
#include "dia_dirs.h"
#include "intl.h"
#include "plug-ins.h"
#include "custom_object.h"


G_MODULE_EXPORT gboolean custom_object_load(gchar *filename, 
                                            DiaObjectType **otype);

/* Cannot be static, because we may use this fn later when loading
   a new shape via the sheets dialog */
   
G_MODULE_EXPORT gboolean
custom_object_load(gchar *filename, DiaObjectType **otype)
{
  ShapeInfo *info;

  if (!filename)
    return FALSE;
  info = shape_info_load(filename);
  /*g_assert(info);*/
  if (!info) {
    *otype = NULL; 
    return FALSE;
  }
  custom_object_new(info, otype);
  return TRUE;
}

static gboolean
custom_object_preload(gchar *filename, DiaObjectType **otype)
{
  ShapeInfo *info;
  
  info = g_new0 (ShapeInfo, 1);
  info->filename = g_strdup (filename);
  /* Just enough to register the type, not enough to create the object */
  if (!shape_typeinfo_load(info)) {
    /* there are currently 5 - out of ~700 shapes - which fail the size assumption 
     * (reading only the first 512 bytes of the shape).
     * Instead of not loading them at all, they are loaded  completely as a fallback.
     * Another way would be to increase the size to read for every shape, seems worse.
     */
     g_free (info->filename);
     g_free (info);
     if ((info = shape_info_load(filename)) == NULL)
       return FALSE;
  }
  shape_info_register (info);
  custom_object_new(info, otype);
  return TRUE;
}
static void 
load_shapes_from_tree(const gchar *directory)
{
  GDir *dp;
  const char *dentry;

  dp = g_dir_open(directory, 0, NULL);
  if (dp == NULL) {
    return;
  }

  while ( (dentry = g_dir_read_name(dp)) ) {
    gchar *filename = g_build_filename(directory, dentry, NULL);
    const gchar *p;

    if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
      load_shapes_from_tree(filename);
      g_free(filename);
      continue;
    }
    /* if it's not a directory, then it must be a .shape file */
    if (   !g_file_test(filename, G_FILE_TEST_IS_REGULAR)
        || (strlen(dentry) < 6)) {
      g_free(filename);
      continue;
    }
    
    p = dentry + strlen(dentry) - 6;
    if (0==strcmp(".shape",p)) {
      DiaObjectType *ot;

      if (!custom_object_preload(filename, &ot)) {
        g_warning("could not load shape file %s",filename);
      } else {
        g_assert(ot); 
        g_assert(ot->default_user_data);
        object_register_type(ot);
      }
    }
    g_free(filename);
  }
  g_dir_close(dp);
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  char *shape_path;
  const char *home_dir;

  if (!dia_plugin_info_init(info, _("Custom"), _("Custom XML shapes loader"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  home_dir = g_get_home_dir();
  if (home_dir) {
    home_dir = dia_config_filename("shapes");
    load_shapes_from_tree(home_dir);
    g_free((char *)home_dir);
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

  return DIA_PLUGIN_INIT_OK;
}
