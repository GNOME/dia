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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <glib/gstdio.h>
#include <glib.h>

#include "dia_dirs.h"
#include "message.h"


/**
 * dia_get_data_directory:
 * @subdir: The name of the directory desired.
 *
 * Get the name of a subdirectory of our data directory.
 *
 * This function does not create the subdirectory, just make the correct name.
 *
 * Returns: The full path to the directory. This string should be freed
 *          after use.
 *
 * Since: dawn-of-time
 */
char *
dia_get_data_directory (const char* subdir)
{
#ifdef G_OS_WIN32
  char *returnPath = NULL;
  /*
   * Calculate from executable path
   */
  char *sLoc = g_win32_get_package_installation_directory_of_module (NULL);

  returnPath = g_build_filename (sLoc, "share", "dia", subdir, NULL);

  g_clear_pointer (&sLoc, g_free);

  return returnPath;
#else
  char *base = g_strdup (PKGDATADIR);
  char *ret;
  if (g_getenv ("DIA_BASE_PATH") != NULL) {
    g_clear_pointer (&base, g_free);
    /* a small hack cause the final destination and the local path differ */
    base = g_build_filename (g_getenv ("DIA_BASE_PATH"), "data", NULL);
  }
  if (strlen (subdir) == 0) {
    ret = g_strconcat (base, NULL);
  } else {
    ret = g_strconcat (base, G_DIR_SEPARATOR_S, subdir, NULL);
  }
  g_clear_pointer (&base, g_free);
  return ret;
#endif
}


/**
 * dia_get_lib_directory:
 *
 * Get a subdirectory of our lib directory. This does not create the
 * directory, merely the name of the full path.
 *
 * Return: The full path of the directory. The string should be freed after
 *         use.
 */
char *
dia_get_lib_directory (void)
{
#ifdef G_OS_WIN32
  char *sLoc = g_win32_get_package_installation_directory_of_module (NULL);
  char *returnPath = g_build_filename (sLoc, "lib", "dia", NULL);

  g_clear_pointer (&sLoc, g_free);

  return returnPath;
#else
  return g_build_filename (DIALIBDIR, "", NULL);
#endif
}


char *
dia_get_locale_directory (void)
{
#ifdef G_OS_WIN32
  char *sLoc = g_win32_get_package_installation_directory_of_module (NULL);
  char *ret = g_build_filename (sLoc, "share", "locale", NULL);

  g_clear_pointer (&sLoc, g_free);

  return ret;
#else
  return g_strdup (LOCALEDIR);
#endif
}


/**
 * dia_config_filename:
 * @subfile: Name of the subfile.
 *
 * Get the name of a file under the Dia config directory. If no home
 * directory can be found, uses a temporary directory.
 *
 * Returns: A string with the full path of the desired file. This string
 *          should be freed after use.
 *
 * Since: dawn-of-time
 */
char *
dia_config_filename (const char *subfile)
{
  const char *homedir;

  homedir = g_get_home_dir ();
  if (!homedir) {
    homedir = g_get_tmp_dir (); /* put config stuff in /tmp -- not ideal, but
                                 * we should not reach this state */
  }
  return g_build_filename (homedir, ".dia", subfile, NULL);
}


/**
 * dia_config_ensure_dir:
 * @filename: A file that we want the parent directory of to exist.
 *
 * Ensure that the directory part of `filename' exists.
 *
 * Returns: %TRUE if the directory existed or has been created, %FALSE if
 *          it cannot be created.
 *
 * Since: dawn-of-time
 */
gboolean
dia_config_ensure_dir (const char *filename)
{
  char *dir = g_path_get_dirname (filename);
  gboolean exists;
  if (dir == NULL) {
    return FALSE;
  }
  if (strcmp (dir, ".")) {
    if (g_file_test (dir, G_FILE_TEST_IS_DIR)) {
      exists = TRUE;
    } else {
      if (dia_config_ensure_dir (dir)) {
        exists = (g_mkdir(dir, 0755) == 0);
      } else {
        exists = FALSE;
      }
    }
  } else {
    exists = FALSE;
  }
  g_clear_pointer (&dir, g_free);
  return exists;
}


/**
 * dia_message_filename:
 * @filename: A filename string as gotten from the filesystem.
 *
 * Returns an filename in UTF-8 encoding from filename in filesystem encoding.
 *
 * The value returned is a pointer to static array.
 *
 * Note: The string can be used AFTER the next call to this function
 *       Written like g_strerror()
 *
 * Returns: UTF-8 encoded copy of the filename.
 *
 * Since: dawn-of-time
 */
const char *
dia_message_filename(const gchar *filename)
{
  char *tmp;
  GQuark msg_quark;

  tmp = g_filename_display_name (filename);

  /* Stick in the quark table so that we can return a static result */
  msg_quark = g_quark_from_string (tmp);

  g_clear_pointer (&tmp, g_free);

  return g_quark_to_string (msg_quark);
}


/**
 * dia_relativize_filename:
 * @master: The main filename
 * @slave: A filename to become relative to the master
 *
 * Calculate a filename relative to the basepath of a master file
 *
 * Returns: Relative filename or %NULL if there is no common base path
 *
 * Since: dawn-of-time
 */
char *
dia_relativize_filename (const char *master, const char *slave)
{
  char *bp1;
  char *bp2;
  char *rel = NULL;

  if (!g_path_is_absolute (master) || !g_path_is_absolute (slave)) {
    return NULL;
  }

  bp1 = g_path_get_dirname (master);
  bp2 = g_path_get_dirname (slave);

  /* the slave path has to be included in master to become relative */
  if (g_str_has_prefix (bp2, bp1)) {
    char *p;
    /* We have do deal with the special meaning of Windows drives, where 'c:' is
     * a reference to the current directory, but c:\ is the root path. For other
     * directory names the trailing backslash is stripped by g_path_get_dirname().
     * So only advance (+1) in slave when there is no trailing backslash.
     */
    rel = g_strdup (slave + strlen (bp1) +
                    (g_str_has_suffix (bp1, G_DIR_SEPARATOR_S) ? 0 : 1));
    /* flip backslashes */
    for (p = rel; *p != '\0'; p++) {
      if (*p == '\\') {
        *p = '/';
      }
    }
  }

  g_clear_pointer (&bp1, g_free);
  g_clear_pointer (&bp2, g_free);

  return rel;
}


char *
dia_absolutize_filename (const char *master, const char *slave)
{
  char *path = g_path_get_dirname (master);
  char *result = g_build_path (G_DIR_SEPARATOR_S, path, slave, NULL);

  g_clear_pointer (&path, g_free);

  return result;
}
