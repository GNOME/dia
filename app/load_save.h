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
#ifndef LOAD_SAVE_H
#define LOAD_SAVE_H

#include "diagram.h"
#include "filter.h"

int diagram_save(Diagram *dia, const char *filename, DiaContext *ctx);
void diagram_autosave(Diagram *dia);
void diagram_cleanup_autosave(Diagram *dia);

extern DiaExportFilter dia_export_filter;
extern DiaImportFilter dia_import_filter;

#endif /* LOAD_SAVE_H */
