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
#include "dia_dirs.h"
#ifdef G_OS_WIN32
#include <windows.h>
#endif

gchar*
dia_get_data_directory (const gchar* subdir)
{
#ifdef G_OS_WIN32
  /*
   * Calulate from executable path
   */
  gchar sLoc [_MAX_PATH+1];
  HINSTANCE hInst = GetModuleHandle(NULL);

  if (0 != GetModuleFileName(hInst, sLoc, _MAX_PATH))
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
  gchar sLoc [_MAX_PATH+1];
  HINSTANCE hInst = GetModuleHandle(NULL);

  if (0 != GetModuleFileName(hInst, sLoc, _MAX_PATH))
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
  gchar *homedir;

  homedir = g_get_home_dir();
  if (!homedir) {
    homedir = g_get_tmp_dir(); /* put config stuff in /tmp -- not ideal, but
				* we should not reach this state */
  }
  return g_strconcat(homedir, G_DIR_SEPARATOR_S ".dia" G_DIR_SEPARATOR_S,
		     subfile, NULL);
}
