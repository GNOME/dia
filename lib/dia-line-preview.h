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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "dia-enums.h"

G_BEGIN_DECLS

#define DIA_TYPE_LINE_PREVIEW dia_line_preview_get_type ()
G_DECLARE_FINAL_TYPE (DiaLinePreview, dia_line_preview, DIA, LINE_PREVIEW, GtkWidget)

GtkWidget *dia_line_preview_new                  (DiaLineStyle          lstyle);
void       dia_line_preview_set_style            (DiaLinePreview       *line,
                                                  DiaLineStyle          lstyle);

G_END_DECLS
