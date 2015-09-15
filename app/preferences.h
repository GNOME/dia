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

#include "diatypes.h"
#include "diagramdata.h"

#define DEFAULT_GRID_COLOR { 0.85, .90, .90, 1.0 }
#define DEFAULT_PAGEBREAK_COLOR { 0.0, 0.0, 0.6, 1.0 }

struct DiaPreferences {
  struct {
    int visible;
    int snap;
    gboolean dynamic;
    real x;
    real y;
    int vis_x;
    int vis_y;
    int major_lines;
    int hex;
    real w;
  } grid;
  
  struct {
    int width;
    int height;
    real zoom;
    int use_menu_bar;
  } new_view;

  NewDiagramData new_diagram;

  int show_cx_pts;
  int snap_object; /* mainpoint_magnetism : the whole object is the connection point */
  int view_antialiased;

  int reset_tools_after_create;
  int undo_depth;
  int reverse_rubberbanding_intersects;
  guint recent_documents_list_size;

  gchar* length_unit;
  gchar* fontsize_unit;
  
  struct {
    int visible;
    int solid;
  } pagebreak;

  int fixed_icon_size;

  int toolbox_on_top;

  int use_integrated_ui;
  
  /* a dedicated filter name or NULL */
  struct {
    char *png;
    char *svg;
    char *ps;
    char *wmf;
    char *emf;
    char *print;
  } favored_filter;
};

extern struct DiaPreferences prefs;

void prefs_show(void);
void prefs_set_defaults(void);
void prefs_save(void);
void prefs_init(void);

#endif /* PREFERENCES_H */
