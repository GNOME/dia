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

#ifndef DIA_ERROR_H
#define DIA_ERROR_H

#include <glib.h>

/* the domain of the error */
#define DIA_ERROR dia_error_quark ()

/* some more detailed error code */
typedef enum
{
  DIA_ERROR_FORMAT,    /* something wrong with our xml format */
  DIA_ERROR_FILE,      /*  to be got from inport filters? */
  DIA_ERROR_DIRECTORY, /*  directory missing */
} DiaError;

GQuark dia_error_quark (void);

#endif /* DIA_ERROR_H */
