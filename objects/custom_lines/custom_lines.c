/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Lines -- line shapes defined in XML rather than C.
 * Based on the original Custom Objects plugin.
 * Copyright (C) 1999 James Henstridge.
 * Adapted for Custom Lines plugin by Marcel Toele.
 * Modifications (C) 2007 Kern Automatiseringsdiensten BV.
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

#include <glib/gi18n-lib.h>

#include <glib.h>

#include "sheet.h"
#include "line_info.h"
#include "dia_dirs.h"
#include "plug-ins.h"
#include "custom_linetypes.h"

char* custom_lines_string_plus( char* lhs, char* mid, char* rhs );

G_MODULE_EXPORT gboolean custom_linefile_load (char *filename, LineInfo **info);

/* Cannot be static, because we may use this fn later when loading
   a new shape via the sheets dialog */

G_MODULE_EXPORT gboolean custom_linefile_load (char *filename, LineInfo **info)
{
  if (!filename)
    return FALSE;

  *info = line_info_load(filename);

  /*g_assert(info);*/
  if (!(*info)) {
    return FALSE;
  }

  return TRUE;
}

char* custom_lines_string_plus( char* lhs, char* mid, char* rhs )
{
	char* res = g_new0(char, strlen(lhs) + strlen(rhs) + strlen( mid ) + 1);

	sprintf( res, "%s%s%s", lhs, mid, rhs );

	return( res );
}

void custom_linetype_create_and_register( LineInfo* info )
{
  DiaObjectType* otype = NULL;

  if (info->type == CUSTOM_LINETYPE_ALL ) {
    int i = 0;
    for( i = 0; i < CUSTOM_LINETYPE_ALL; i++ )
    {
      LineInfo* cloned_info = line_info_clone( info );
      cloned_info->type = i;
      cloned_info->name = custom_lines_string_plus( info->name, " - ", custom_linetype_strings[i] );

      if (cloned_info->icon_filename) {
        /* split at the file extension */
        char** chunks = g_strsplit( info->icon_filename, ".png", 0 );
        char buf[20];

        sprintf( buf, "_%s", custom_linetype_strings[i] );
        cloned_info->icon_filename =
          custom_lines_string_plus( chunks[0], buf, ".png" );
      }

      custom_linetype_new(cloned_info, &otype);
      g_assert(otype);
      g_assert(otype->default_user_data);
      object_register_type(otype);
    }
  } else {
    custom_linetype_new(info, &otype);
    g_assert(otype);
    g_assert(otype->default_user_data);
    object_register_type(otype);
  }
}

static void load_linefiles_from_tree (const char *directory)
{
  GDir *dp;
  const char *dentry;

  dp = g_dir_open(directory, 0, NULL);
  if (dp == NULL) {
    return;
  }

  while ((dentry = g_dir_read_name (dp))) {
    char *filename = g_build_filename (directory, dentry, NULL);
    const char *p;

    if (g_file_test(filename, G_FILE_TEST_IS_DIR)) {
      load_linefiles_from_tree(filename);
      g_clear_pointer (&filename, g_free);
      continue;
    }
    /* if it's not a directory, then it must be a .line file */
    if (   !g_file_test(filename, G_FILE_TEST_IS_REGULAR)
        || (strlen(dentry) < strlen( ".line" ))) {
      g_clear_pointer (&filename, g_free);
      continue;
    }

    p = dentry + strlen(dentry) - strlen( ".line" );
    if (0==strcmp(".line",p)) {
      LineInfo *info;

      if (!custom_linefile_load(filename, &info)) {
        g_warning("could not load line file %s",filename);
      } else {
        custom_linetype_create_and_register( info );
      }
    }
    g_clear_pointer (&filename, g_free);
  }
  g_dir_close(dp);
}

DIA_PLUGIN_CHECK_INIT


PluginInitResult
dia_plugin_init (PluginInfo *info)
{
  char *line_path;
  char *home_dir;

  if (!dia_plugin_info_init(info, _("CustomLines"), _("Custom XML lines loader"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  if (g_get_home_dir ()) {
    home_dir = dia_config_filename ("lines");
    load_linefiles_from_tree (home_dir);
    g_clear_pointer (&home_dir, g_free);
  }

  line_path = getenv("DIA_LINE_PATH");
  if (line_path) {
    char **dirs = g_strsplit (line_path, G_SEARCHPATH_SEPARATOR_S, 0);
    int i;

    for (i = 0; dirs[i] != NULL; i++) {
      load_linefiles_from_tree (dirs[i]);
    }
    g_strfreev(dirs);
  } else {
    char *thedir = dia_get_data_directory("lines");
    load_linefiles_from_tree (thedir);
    g_clear_pointer (&thedir, g_free);
  }

  return DIA_PLUGIN_INIT_OK;
}

