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
#include <direct.h>
#define mkdir(s,a) _mkdir(s)
#else
#include <sys/stat.h>
#include <sys/types.h>
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
	exists = (mkdir(dir, 0755) == 0);
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

/** Remove all instances of . and .. from an absolute path.
 * This is not a cheap function.
 * Returns a newly allocated string, or NULL if too many ..'s were found */
static gchar *
dia_get_canonical_path (gchar *path) {
  if (g_str_has_suffix(path, "/..")) {
    gchar *prev = g_path_get_dirname(path);
    path = prev;
    prev = dia_get_canonical_path(path);
    g_free(path);
    if (prev == NULL) return NULL;
    path = prev;
    /* Snip! */
    prev = g_path_get_dirname(prev);
    if (!strcmp(path, prev)) {
      /* .. ran over / */
      return NULL;
    }
    g_free(path);
    return prev;
  } else if (g_str_has_suffix(path, "/.")) {
    gchar *prev = g_path_get_dirname(path);
    path = prev;
    prev = dia_get_canonical_path(path);
    g_free(path);
    return prev;
  } else {
    gchar *end = g_path_get_basename(path);
    gchar *prev = g_path_get_dirname(path);
    gchar *tmp;
    if (strcmp(path, prev)) { /* Something got removed */
      tmp = prev;
      prev = dia_get_canonical_path(tmp);
      if (prev == NULL) return NULL;
      g_free(tmp);
      tmp = prev;
      prev = g_build_filename(tmp, end, NULL);
      g_free(tmp);
      g_free(end);
      return prev;
    } else {
      g_free(end);
      g_free(prev);
      return g_strdup(path);
    }
  }
}

/** Return an absolute filename from an absolute or relative filename.
 * The value returned is newly allocated. 
 */
gchar *
dia_get_absolute_filename (const gchar *filename)
{
  gchar *current_dir;
  gchar *fullname;
  gchar *canonical;
  if (filename == NULL) return NULL;
  if (g_path_is_absolute(filename)) return g_strdup(filename);
  current_dir = g_get_current_dir();
  fullname = g_build_filename(current_dir, filename, NULL);
  g_free(current_dir);
  if (strchr(fullname, '.') == NULL) return fullname;
  canonical = dia_get_canonical_path(fullname);
  if (canonical == NULL) {
    message_warning("Too many ..'s in filename %s\n", filename);
    return filename;
  }
  free(fullname);
  return canonical;
}
