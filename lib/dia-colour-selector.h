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

#include "color.h"

G_BEGIN_DECLS

#define DIA_TYPE_COLOUR_SELECTOR dia_colour_selector_get_type ()
G_DECLARE_FINAL_TYPE (DiaColourSelector, dia_colour_selector, DIA, COLOUR_SELECTOR, GtkBox)

GtkWidget *dia_colour_selector_new                (void);
void       dia_colour_selector_set_use_alpha      (DiaColourSelector    *cs,
                                                   gboolean              use_alpha);
void       dia_colour_selector_get_colour         (DiaColourSelector    *cs,
                                                   Color                *color);
void       dia_colour_selector_set_colour         (DiaColourSelector    *cs,
                                                   const Color          *color);

G_END_DECLS
