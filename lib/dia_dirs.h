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

#ifndef DIA_DIRS_H
#define DIA_DIRS_H

#include <glib.h>

gchar *dia_get_data_directory (const gchar* subdir);
gchar *dia_get_lib_directory  (void);
gchar *dia_get_locale_directory (void);
gchar *dia_config_filename    (const gchar* file);
gboolean dia_config_ensure_dir  (const gchar* filename);
gchar *dia_get_absolute_filename (const gchar *filename);
gchar *dia_relativize_filename (const gchar *master, const gchar *slave);
gchar *dia_absolutize_filename (const gchar *master, const gchar *slave);
gchar *dia_get_canonical_path (const gchar *path);
const gchar *dia_message_filename (const gchar *filename);

#endif /* DIA_DIRS_H */
