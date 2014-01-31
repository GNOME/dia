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
#ifndef DIA_FONT_SELECTOR_H
#define DIA_FONT_SELECTOR_H

#include <gtk/gtk.h>
#include "diatypes.h"

/* DiaFontSelector: */
#define DIAFONTSELECTOR(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, dia_font_selector_get_type (), DiaFontSelector)
#define DIAFONTSELECTOR_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, dia_font_selector_get_type (), DiaFontSelectorClass)
#define IS_DIAFONTSELECTOR(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, dia_font_selector_get_type ())


GType    dia_font_selector_get_type        (void);
GtkWidget* dia_font_selector_new             (void);
void       dia_font_selector_set_font        (DiaFontSelector *fs, DiaFont *font);
void       dia_font_selector_set_preview     (DiaFontSelector *fs, gchar *text);
DiaFont *     dia_font_selector_get_font        (DiaFontSelector *fs);

#endif /* DIA_FONT_SELECTOR_H */
