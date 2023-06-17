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

#pragma once
#include <glib.h>

G_BEGIN_DECLS

char        *dia_get_data_directory    (const char *subdir);
char        *dia_get_lib_directory     (void);
char        *dia_get_locale_directory  (void);
char        *dia_config_filename       (const char *file);
gboolean     dia_config_ensure_dir     (const char *filename);
char        *dia_relativize_filename   (const char *master,
                                        const char *slave);
char        *dia_absolutize_filename   (const char *master,
                                        const char *slave);

G_END_DECLS
