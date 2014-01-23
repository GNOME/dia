/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * paginate_psprint.[ch] -- pagination code for the postscript backend
 * Copyright (C) 1999 James Henstridge
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PAGINATE_PSPRINT_H
#define _PAGINATE_PSPRINT_H

#include <stdio.h>
#include <glib.h>

#include "diagramdata.h"

void paginate_psprint (DiagramData *dia, FILE *file);

void diagram_print_ps (DiagramData *dia, const gchar *filename);

#endif
