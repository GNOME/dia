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

#include <glib-object.h>

#include "dia-enums.h"
#include "dia-part.h"
#include "font.h"

G_BEGIN_DECLS

#define DIA_TYPE_TEXT_LINE (dia_text_line_get_type ())
G_DECLARE_FINAL_TYPE (DiaTextLine, dia_text_line, DIA, TEXT_LINE, DiaPart)


DiaTextLine  *dia_text_line_new                      (const char       *string,
                                                      DiaFont          *font,
                                                      double            height);
DiaTextLine  *dia_text_line_copy                     (DiaTextLine      *text);
void          dia_text_line_set_string               (DiaTextLine      *text,
                                                      const char       *string);
void          dia_text_line_set_height               (DiaTextLine      *text,
                                                      double            height);
void          dia_text_line_set_font                 (DiaTextLine      *text,
                                                      DiaFont          *font);
const char   *dia_text_line_get_string               (DiaTextLine      *text);
DiaFont      *dia_text_line_get_font                 (DiaTextLine      *text);
double        dia_text_line_get_height               (DiaTextLine      *text);
double        dia_text_line_get_width                (DiaTextLine      *text);
double        dia_text_line_get_ascent               (DiaTextLine      *text);
double        dia_text_line_get_descent              (DiaTextLine      *text);
double        dia_text_line_get_alignment_adjustment (DiaTextLine      *text_line,
                                                      DiaAlignment      alignment);

G_END_DECLS
