/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "geometry.h"
#include "color.h"
#include "diagram_tree_window.h"

struct DiaPreferences {
  struct {
    int visible;
    int snap;
    real x;
    real y;
    Color colour;
    int solid;
  } grid;
  
  struct {
    int width;
    int height;
    real zoom;
    int use_menu_bar;
  } new_view;

  struct {
    gchar *papertype;
    gfloat tmargin, bmargin, lmargin, rmargin;
    gboolean is_portrait;
    gfloat scaling;
    gboolean fitto;
    gint fitwidth, fitheight;
  } new_diagram;
  
  int show_cx_pts;
  int reset_tools_after_create;
  int compress_save;
  int undo_depth;
  int reverse_rubberbanding_intersects;
  
  struct {
    int visible;
    Color colour;
    int solid;
  } pagebreak;

  int render_bounding_boxes;
  int pretty_formated_xml;
  int prefer_psprint;
  DiagramTreeConfig dia_tree;
};

extern struct DiaPreferences prefs;

void prefs_show(void);
void prefs_set_defaults(void);
void prefs_save(void);
void prefs_load(void);

#endif /* DIA_IMAGE_H */


