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
dia_get_canonical_path (const gchar *path) 
{
  gchar  *ret = NULL;
  gchar **list = g_strsplit (path, G_DIR_SEPARATOR_S, -1);
  int i = 0, n = 0;

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

/** Returns an filename in UTF-8 encoding from filename in filesystem
 * encoding. 
 * The value returned is a pointer to static array.
 * Note: The string can be used AFTER the next call to this function
 *       Written like glib/gstrfuncs.c#g_strerror()
 * In GTK < 2.6, invalid sequences are not 
 */

const gchar *
dia_message_filename (const gchar *filename)
{
  gchar *tmp;
  GQuark msg_quark;

#if GLIB_CHECK_VERSION(2,6,0)
  tmp = g_filename_display_name(filename);
#else
  gsize num_read;
  tmp = g_filename_to_utf8(filename, -1, &num_read, NULL, NULL);
  if (tmp == NULL) {
    gchar *ellipsis;
    /* Best effort at displaying filename: Display as must as is readable */
    g_utf8_validate(filename, -1, &ellipsis);
    tmp = g_filename_to_utf8(filename, ellipsis-filename, NULL, NULL, NULL);
    ellipsis = g_strdup_printf(_("%s<illegal characters>..."), tmp);
    g_free(tmp);
    tmp = ellipsis;
  }
#endif
  /* Stick in the quark table so that we can return a static result
   */
  msg_quark = g_quark_from_string (tmp);
  g_free (tmp);
  tmp = (gchar *) g_quark_to_string (msg_quark);
  return tmp;
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
  if (g_path_is_absolute(filename)) return dia_get_canonical_path(filename);
  current_dir = g_get_current_dir();
  fullname = g_build_filename(current_dir, filename, NULL);
  g_free(current_dir);
  if (strchr(fullname, '.') == NULL) return fullname;
  canonical = dia_get_canonical_path(fullname);
  if (canonical == NULL) {
    message_warning(_("Too many ..'s in filename %s\n"),
                    dia_message_filename(filename));
    return g_strdup(filename);
  }
  g_free(fullname);
  return canonical;
}
