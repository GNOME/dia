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
#include "intl.h"
#include "message.h"
#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif
#include <glib/gstdio.h>

#if defined(G_OS_WIN32) && defined(PREFIX)
static gchar *
replace_prefix (const gchar *runtime_prefix,
                const gchar *configure_time_path)
{
  if (runtime_prefix &&
      strncmp (configure_time_path, PREFIX "/",
               strlen (PREFIX) + 1) == 0) {
    return g_strconcat (runtime_prefix,
                        configure_time_path + strlen (PREFIX) + 1,
                        NULL);
  } else
    return g_strdup (configure_time_path);
}
#endif

#ifdef G_OS_WIN32
/*
 * Calulate the module directory from executable path
 * Convert to UTF-8 to be compatible with GLib's filename encoding.
 */
static gchar *
_dia_get_module_directory (void)
{
  wchar_t wsLoc [MAX_PATH+1];
  HINSTANCE hInst = GetModuleHandle(NULL);
  gchar *sLoc = NULL;

  if (0 != GetModuleFileNameW(hInst, wsLoc, MAX_PATH))
    {
      sLoc = g_utf16_to_utf8 (wsLoc, -1, NULL, NULL, NULL);
      /* strip the name */
      if (strrchr(sLoc, G_DIR_SEPARATOR))
        strrchr(sLoc, G_DIR_SEPARATOR)[0] = 0;
      /* and one dir (bin) */
      if (strrchr(sLoc, G_DIR_SEPARATOR))
        strrchr(sLoc, G_DIR_SEPARATOR)[1] = 0;
    }
  return sLoc;
}
#endif

/** Get the name of a subdirectory of our data directory.
 *  This function does not create the subdirectory, just make the correct name.
 * @param subdir The name of the directory desired.
 * @returns The full path to the directory.  This string should be freed
 *          after use.
 */
gchar *
dia_get_data_directory(const gchar* subdir)
{
#ifdef G_OS_WIN32
  gchar *tmpPath = NULL;
  gchar *returnPath = NULL;
  /*
   * Calculate from executable path
   */
  gchar *sLoc = _dia_get_module_directory ();
#  if defined(PREFIX) && defined(DATADIR)
  tmpPath = replace_prefix(sLoc, DATADIR);
  if (strlen (subdir) == 0)
    returnPath = g_strdup(tmpPath);
  else
    returnPath = g_build_path(G_DIR_SEPARATOR_S, tmpPath, subdir, NULL);
  g_free(tmpPath);
#  else
  returnPath = g_strconcat (sLoc , subdir, NULL);
#  endif
  g_free (sLoc);
  return returnPath;
#else
  if (strlen (subdir) == 0)
    return g_strconcat (DATADIR, NULL);
  else
    return g_strconcat (DATADIR, G_DIR_SEPARATOR_S, subdir, NULL);
#endif
}

/** Get a subdirectory of our lib directory.  This does not create the
 *  directory, merely the name of the full path.
 * @param subdir The name of the subdirectory wanted.
 * @return The full path of the named directory.  The string should be
 *          freed after use.
 */
gchar*
dia_get_lib_directory(const gchar* subdir)
{
#ifdef G_OS_WIN32
  gchar *sLoc = _dia_get_module_directory ();
  gchar *returnPath = NULL;
#  if defined(PREFIX) && defined(LIBDIR)
  {
    gchar *tmpPath = replace_prefix(sLoc, LIBDIR);
    returnPath = g_build_path(G_DIR_SEPARATOR_S, tmpPath, subdir, NULL);
    g_free(tmpPath);
  }
#  else
  returnPath = g_strconcat (sLoc , subdir, NULL);
#  endif
  g_free (sLoc);
  return returnPath;
#else
  return g_strconcat (LIBDIR, G_DIR_SEPARATOR_S, subdir, NULL);
#endif
}

gchar*
dia_get_locale_directory(void)
{
#ifdef G_OS_WIN32
#  if defined(PREFIX) && defined(LOCALEDIR)
  gchar *ret;
  gchar *sLoc = _dia_get_module_directory ();
  ret = replace_prefix(sLoc, LOCALEDIR);
  g_free (sLoc);
  return  ret;
#  else
  return dia_get_lib_directory ("locale");
#  endif
#else
  return g_strconcat (LOCALEDIR, G_DIR_SEPARATOR_S, "", NULL);
#endif
}

/** Get the name of a file under the Dia config directory.  If no home
 *  directory can be found, uses a temporary directory.
 * @param subfile Name of the subfile.
 * @returns A string with the full path of the desired file.  This string
 *          should be freed after use.
 */
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
 * @param filename A file that we want the parent directory of to exist.
 * @returns TRUE if the directory existed or has been created, FALSE if
 *          it cannot be created.
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
	exists = (g_mkdir(dir, 0755) == 0);
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
 * @param path String to canonicalize.
 * @returns A newly allocated string, or NULL if too many ..'s were found
 */
gchar *
dia_get_canonical_path(const gchar *path)
{
  gchar  *ret = NULL;
  gchar **list;
  int i = 0, n = 0;

  /* shortcut for nothing to do (also keeps UNC path intact */
  if (!strstr(path, "..") && !strstr(path, "." G_DIR_SEPARATOR_S))
    return g_strdup(path);

  list = g_strsplit (path, G_DIR_SEPARATOR_S, -1);
  while (list[i] != NULL) {
    if (0 == strcmp (list[i], ".")) {
      /* simple, just remove it */
      g_free (list[i]);
      list[i] = g_strdup ("");
    }
    else if  (0 == strcmp (list[i], "..")) {
      /* need to 'remove' the previous non empty part too */
      n = i;
      g_free (list[i]);
      list[i] = g_strdup ("");
      while (n >= 0) {
        if (0 != strlen(list[n])) {
          /* remove it */
          g_free (list[n]);
          list[n] = g_strdup ("");
          break;
        }
        n--;
      }
      /* we haven't found an entry to remove for '..' */
      if (n < 0)
        break;
    }
    i++;
  }
  if (n >= 0) {
    /* cant use g_strjoinv () cause it would stumble about empty elements */
    GString *str = g_string_new (NULL);

    i = 0;
    while (list[i] != NULL) {
      if (strlen(list[i]) > 0) {

        /* win32 filenames usually don't start with a dir separator but
         * with <drive>:\ 
         */
	if (i != 0 || list[i][1] != ':')
          g_string_append (str, G_DIR_SEPARATOR_S);
        g_string_append (str, list[i]);
      }
      i++;
    }
    ret = g_string_free (str, FALSE);
  }

  g_strfreev(list);

  return ret;
}

/** Returns an filename in UTF-8 encoding from filename in filesystem encoding.
 * @param filename A filename string as gotten from the filesystem.
 * @returns UTF-8 encoded copy of the filename.
 * The value returned is a pointer to static array.
 * Note: The string can be used AFTER the next call to this function
 *       Written like glib/gstrfuncs.c#g_strerror()
 */
const gchar *
dia_message_filename(const gchar *filename)
{
  gchar *tmp;
  GQuark msg_quark;

  tmp = g_filename_display_name(filename);
  /* Stick in the quark table so that we can return a static result
   */
  msg_quark = g_quark_from_string (tmp);
  g_free (tmp);
  tmp = (gchar *) g_quark_to_string (msg_quark);
  return tmp;
}

/** Return an absolute filename from an absolute or relative filename.
 * @param filename A relative or absolute filename.
 * @return Absolute and canonicalized filename as a newly allocated string.
 */
gchar *
dia_get_absolute_filename (const gchar *filename)
{
  gchar *current_dir;
  gchar *fullname;
  gchar *canonical;
  if (filename == NULL) return NULL;
  if (g_path_is_absolute(filename)) return dia_get_canonical_path(filename);
  current_dir = g_get_current_dir();
  fullname = g_build_filename(current_dir, filename, NULL);
  g_free(current_dir);
  if (strchr(fullname, '.') == NULL) return fullname;
  canonical = dia_get_canonical_path(fullname);
  if (canonical == NULL) {
    message_warning(_("Too many \"..\"s in filename %s\n"),
                    dia_message_filename(filename));
    return g_strdup(filename);
  }
  g_free(fullname);
  return canonical;
}

/** Calculate a filename relative to the basepath of a master file
 * @param master The main filename
 * @param slave  A filename to become relative to the master
 * @return Relative filename or NULL if there is no common base path
 */
gchar *
dia_relativize_filename (const gchar *master, const gchar *slave)
{
  gchar *bp1;
  gchar *bp2;
  gchar * rel = NULL;

  if (!g_path_is_absolute (master) || !g_path_is_absolute (slave))
    return NULL;

  bp1 = g_path_get_dirname (master);
  bp2 = g_path_get_dirname (slave);

  /* the slave path has to be included in master to become relative */
  if (g_str_has_prefix (bp2, bp1)) {
    gchar *p;
    /* We have do deal with the special meaning of Windows drives, where 'c:' is
     * a reference to the current directory, but c:\ is the root path. For other
     * directory names the trailing backslash is stripped by g_path_get_dirname().
     * So only advance (+1) in slave when there is no trailing backslash.
     */
    rel = g_strdup (slave + strlen (bp1)
			  + (g_str_has_suffix (bp1, G_DIR_SEPARATOR_S) ? 0 : 1));
    /* flip backslashes */
    for (p = rel; *p != '\0'; p++)
      if (*p == '\\') *p = '/';    
  }
  g_free (bp1);
  g_free (bp2);
  
  return rel;
}

gchar *
dia_absolutize_filename (const gchar *master, const gchar *slave)
{
  gchar *path = g_path_get_dirname (master);
  gchar *result = g_build_path (G_DIR_SEPARATOR_S, path, slave, NULL);

  g_free (path);
  return result;
}
