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

#include <config.h>

#include <string.h> /* strlen() */

#include "dia_dirs.h"
#ifdef G_OS_WIN32
#include <windows.h>
#endif

gchar*
dia_get_data_directory (const gchar* subdir)
{
#ifdef G_OS_WIN32
  /*
   * Calculate from executable path
   */
  gchar sLoc [MAX_PATH+1];
  HINSTANCE hInst = GetModuleHandle(NULL);

  if (0 != GetModuleFileName(hInst, sLoc, MAX_PATH))
    {
	/* strip the name */
      if (strrchr(sLoc, G_DIR_SEPARATOR))
        strrchr(sLoc, G_DIR_SEPARATOR)[0] = 0;
      /* and one dir (bin) */
      if (strrchr(sLoc, G_DIR_SEPARATOR))
        strrchr(sLoc, G_DIR_SEPARATOR)[1] = 0;
    }
  return g_strconcat (sLoc , subdir, NULL); 

#else
  if (strlen (subdir) == 0)		
    return g_strconcat (DATADIR, NULL);
  else
    return g_strconcat (DATADIR, G_DIR_SEPARATOR_S, subdir, NULL);
#endif
}


gchar*
dia_get_lib_directory (const gchar* subdir)
{
#ifdef G_OS_WIN32
  /*
   * Calulate from executable path
   */
  gchar sLoc [MAX_PATH+1];
  HINSTANCE hInst = GetModuleHandle(NULL);

  if (0 != GetModuleFileName(hInst, sLoc, MAX_PATH))
    {
	/* strip the name */
      if (strrchr(sLoc, G_DIR_SEPARATOR))
        strrchr(sLoc, G_DIR_SEPARATOR)[0] = 0;
      /* and one dir (bin) */
      if (strrchr(sLoc, G_DIR_SEPARATOR))
        strrchr(sLoc, G_DIR_SEPARATOR)[1] = 0;
    }
  return g_strconcat (sLoc , subdir, NULL); 

#else
  return g_strconcat (LIBDIR, G_DIR_SEPARATOR_S, subdir, NULL);
#endif
}

gchar *
dia_config_filename(const gchar *subfile)
{
  const gchar *homedir;

  homedir = g_get_home_dir();
  if (!homedir) {
    homedir = g_get_tmp_dir(); /* put config stuff in /tmp -- not ideal, but
				* we should not reach this state */
  }
  return g_strconcat(homedir, G_DIR_SEPARATOR_S ".dia" G_DIR_SEPARATOR_S,
		     subfile, NULL);
}

/** Ensure that the directory part of `filename' exists.
 * Returns TRUE if the directory existed or has been created.
 */
gboolean
dia_config_ensure_dir(const gchar *filename)
{
  gchar *dir = g_path_get_dirname(filename);
  gboolean exists;
  if (dir == NULL) return FALSE;
  if (strcmp(dir, ".")) {
    if (g_file_test(dir, G_FILE_TEST_IS_DIR)) {
      exists = TRUE;
    } else {
      if (dia_config_ensure_dir(dir)) {
	exists = (mkdir(dir) == 0);
      } else {
	exists = FALSE;
      }
    }
  } else {
    exists = FALSE;
  }
  g_free(dir);
  return exists;
}
