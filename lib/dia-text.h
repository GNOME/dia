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

#include "dia_xml.h" /* for AttributeNode */
#include "dia-object-change.h"
#include "dia-part.h"
#include "dia-text-line.h"
#include "diarenderer.h"
#include "diatypes.h"
#include "textattr.h"

G_BEGIN_DECLS

#define DIA_TYPE_TEXT_OBJECT_CHANGE dia_text_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaTextObjectChange,
                      dia_text_object_change,
                      DIA, TEXT_OBJECT_CHANGE,
                      DiaObjectChange)


#define DIA_TYPE_TEXT (dia_text_get_type ())
G_DECLARE_FINAL_TYPE (DiaText, dia_text, DIA, TEXT, DiaPart)


DiaText          *dia_text_new                 (const char       *string,
                                                DiaFont          *font,
                                                double            height,
                                                Point            *pos,
                                                DiaColour        *colour,
                                                DiaAlignment      align);
DiaText          *dia_text_new_default         (Point            *pos,
                                                DiaColour        *colour,
                                                DiaAlignment      align);
DiaText          *dia_text_copy                (DiaText          *self);
const char       *dia_text_get_line            (DiaText          *self,
                                                int               line);
char             *dia_text_get_string_copy     (DiaText          *self);
void              dia_text_set_string          (DiaText          *self,
                                                const char       *string);
void              dia_text_set_height          (DiaText          *self,
                                                double            height);
double            dia_text_get_height          (DiaText          *self);
void              dia_text_set_font            (DiaText          *self,
                                                DiaFont          *font);
DiaFont          *dia_text_get_font            (DiaText          *self);
void              dia_text_set_position        (DiaText          *self,
                                                Point            *pos);
void              dia_text_get_position        (DiaText          *self,
                                                Point            *pos);
void              dia_text_set_colour          (DiaText          *self,
                                                DiaColour        *colour);
void              dia_text_get_colour          (DiaText          *self,
                                                DiaColour        *colour);
void              dia_text_set_alignment       (DiaText          *self,
                                                DiaAlignment      align);
DiaAlignment      dia_text_get_alignment       (DiaText          *self);
double            dia_text_distance_from       (DiaText          *self,
                                                Point            *point);
void              dia_text_calc_boundingbox    (DiaText          *self,
                                                DiaRectangle     *box);
void              dia_text_draw                (DiaText          *self,
                                                DiaRenderer      *renderer);
void              dia_text_set_cursor          (DiaText          *self,
                                                Point            *clicked_point,
                                                DiaRenderer      *interactive_renderer);
void              dia_text_set_cursor_at_end   (DiaText          *self);
void              dia_text_grab_focus          (DiaText          *self,
                                                DiaObject        *object);
gboolean          dia_text_is_empty            (DiaText          *self);
gboolean          dia_text_delete_all          (DiaText          *self,
                                                DiaObjectChange **change,
                                                DiaObject        *obj);
void              dia_text_get_attributes      (DiaText          *self,
                                                TextAttributes   *attr);
void              dia_text_set_attributes      (DiaText          *self,
                                                TextAttributes   *attr);
DiaTextLine     **dia_text_get_lines           (DiaText          *self,
                                                size_t           *n_lines);
size_t            dia_text_get_n_lines         (DiaText          *self);
gboolean          dia_text_has_focus           (DiaText          *self);
double            dia_text_get_line_width      (DiaText          *self,
                                                int               line_no);
size_t            dia_text_get_line_strlen     (DiaText          *self,
                                                int               line_no);
double            dia_text_get_max_width       (DiaText          *self);
double            dia_text_get_ascent          (DiaText          *self);
double            dia_text_get_descent         (DiaText          *self);
void              dia_text_get_cursor          (DiaText          *self,
                                                int              *cursor_row,
                                                int              *cursor_pos);

void data_add_text(AttributeNode attr, DiaText *text, DiaContext *ctx);
DiaText *data_text(AttributeNode attr, DiaContext *ctx);

gboolean apply_textattr_properties(GPtrArray *props,
                                   DiaText *text, const char *textname,
                                   TextAttributes *attrs);
gboolean apply_textstr_properties(GPtrArray *props,
                                  DiaText *text, const char *textname,
                                  const char *str);

#define DIA_TEXT_FONT_OFFSET (0x0BAD0000 | (1 << 0))
#define DIA_TEXT_HEIGHT_OFFSET (0x0BAD0000 | (1 << 1))
#define DIA_TEXT_POSITION_OFFSET (0x0BAD0000 | (1 << 2))
#define DIA_TEXT_COLOUR_OFFSET (0x0BAD0000 | (1 << 3))
#define DIA_TEXT_ALIGNMENT_OFFSET (0x0BAD0000 | (1 << 4))

G_END_DECLS
