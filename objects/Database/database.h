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

#ifndef DATABASE_H
#define DATABASE_H

#include <gtk/gtk.h>
#include "widgets.h"
#include "element.h"
#include "connectionpoint.h"
#include "orth_conn.h"

#define IS_NOT_EMPTY(str) (((str) != NULL) && ((str)[0] != '\0'))

#define TABLE_CONNECTIONPOINTS 12

typedef struct _Table Table;
typedef struct _TableAttribute TableAttribute;
typedef struct _TablePropDialog TablePropDialog;
typedef struct _TableReference TableReference;
typedef struct _TableState TableState;
typedef struct _TableChange TableChange;
typedef struct _Disconnect Disconnect;

struct _Table {
  Element element; /**< inheritance */

  /** static connection point storage */
  ConnectionPoint connections[TABLE_CONNECTIONPOINTS];

  /* data */

  gchar * name;
  gchar * comment;
  gint visible_comment;
  gint tagging_comment;
  gint underline_primary_key;
  gint bold_primary_key;
  GList * attributes; /**< the attributes of this database table */

  /* fonts */

  real normal_font_height;
  DiaFont * normal_font;

  real primary_key_font_height;
  DiaFont * primary_key_font;

  real name_font_height;
  DiaFont * name_font;

  real comment_font_height;
  DiaFont * comment_font;

  /* colors */

  Color line_color;
  Color fill_color;
  Color text_color;

  real border_width;

  /* computed variables */

  gboolean destroyed;

  real namebox_height;
  real attributesbox_height;
  real maxwidth_attr_name;

  /* the property dialog pointer */

  TablePropDialog * prop_dialog;
};

struct _TableAttribute {
  gchar * name; /* column name */
  gchar * type; /* the type of the values in this column */
  gchar * comment;
  gint primary_key;
  gint nullable;
  gint unique;

  ConnectionPoint * left_connection;
  ConnectionPoint * right_connection;
};

struct _Disconnect {
  ConnectionPoint *cp;
  DiaObject *other_object;
  Handle *other_handle;
};

struct _TableState {
  gchar * name;
  gchar * comment;
  gint visible_comment;
  gint tagging_comment;
  gint underline_primary_key;
  gint bold_primary_key;
  real border_width;

  GList * attributes;
};

struct _TableChange {
  ObjectChange obj_change;

  Table * obj;

  GList * added_cp;
  GList * deleted_cp;
  GList * disconnected;

  gint applied;

  TableState * saved_state;
};

struct _TablePropDialog {
  GtkWidget * dialog;

  /* general page */

  GtkEntry * table_name;
  GtkTextView * table_comment;
  GtkToggleButton * comment_visible;
  GtkToggleButton * comment_tagging;
  GtkToggleButton * underline_primary_key;
  GtkToggleButton * bold_primary_key;

  DiaColorSelector * text_color;
  DiaColorSelector * line_color;
  DiaColorSelector * fill_color;

  DiaFontSelector *normal_font;
  GtkSpinButton *normal_font_height;

  DiaFontSelector *name_font;
  GtkSpinButton *name_font_height;

  DiaFontSelector *comment_font;
  GtkSpinButton *comment_font_height;

  GtkSpinButton * border_width;

  /* attributes page */

  GtkList *     attributes_list;
  GtkEntry *    attribute_name;
  GtkEntry *    attribute_type;
  GtkTextView * attribute_comment;
  GtkToggleButton * attribute_primary_key;
  GtkToggleButton * attribute_nullable;
  GtkToggleButton * attribute_unique;

  GtkListItem * cur_attr_list_item;
  GList * added_connections;
  GList * deleted_connections;
  GList * disconnected_connections;
};

struct _TableReference {
  OrthConn orth; /* inheritance */

  real line_width;
  real dashlength;
  LineStyle line_style;
  Color line_color;
  Color text_color;

  gchar * start_point_desc;
  gchar * end_point_desc;
  Arrow end_arrow;
  real corner_radius;

  DiaFont * normal_font;
  real normal_font_height;

  /* computed data */

  real sp_desc_width;           /* start-point */
  Point sp_desc_pos;            /* start-point */
  Alignment sp_desc_text_align; /* start-point */
  real ep_desc_width;           /* end-point */
  Point ep_desc_pos;            /* end-point */
  Alignment ep_desc_text_align; /* end-point */
};

/* in table_dialog.c */
extern GtkWidget * table_get_properties_dialog (Table *, gboolean);
/* in table_dialog.c */
extern ObjectChange * table_dialog_apply_changes (Table *, GtkWidget *);

/* in table.c */
extern TableAttribute * table_attribute_new (void);
/* in table.c */
extern void table_attribute_free (TableAttribute *);
/* in table.c */
extern TableAttribute * table_attribute_copy (TableAttribute *);
/* in table.c */
extern void table_attribute_ensure_connection_points (TableAttribute *,
                                                      DiaObject *);
/* in table.c */
extern void table_update_connectionpoints (Table *);
/* in table.c */
extern void table_update_positions (Table *);
/* in table.c */
extern void table_compute_width_height (Table *);
/* in table_dialog.c */
extern TableState * table_state_new (Table *);
/* in table_dialog.c */
extern TableChange * table_change_new (Table *, TableState *,
                                       GList *, GList *, GList *);
/* in table.c */
extern void table_update_primary_key_font (Table *);

#endif /* DATABASE_H */
