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

#include "element.h"
#include "connectionpoint.h"
#include "orth_conn.h"

G_BEGIN_DECLS

#define DIA_DB_TYPE_TABLE_OBJECT_CHANGE dia_db_table_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaDBTableObjectChange,
                      dia_db_table_object_change,
                      DIA_DB, TABLE_OBJECT_CHANGE,
                      DiaObjectChange)


#define DIA_DB_TYPE_COMPOUND_OBJECT_CHANGE dia_db_compound_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaDBCompoundObjectChange,
                      dia_db_compound_object_change,
                      DIA_DB, COMPOUND_OBJECT_CHANGE,
                      DiaObjectChange)


#define DIA_DB_TYPE_COMPOUND_MOUNT_OBJECT_CHANGE dia_db_compound_mount_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaDBCompoundMountObjectChange,
                      dia_db_compound_mount_object_change,
                      DIA_DB, COMPOUND_MOUNT_OBJECT_CHANGE,
                      DiaObjectChange)


#define IS_NOT_EMPTY(str) (((str) != NULL) && ((str)[0] != '\0'))

#define TABLE_CONNECTIONPOINTS 12

typedef struct _Table Table;
typedef struct _TableAttribute TableAttribute;
typedef struct _TableReference TableReference;
typedef struct _TableState TableState;
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
};

struct _TableAttribute {
  gchar * name;          /* column name */
  gchar * type;          /* the type of the values in this column */
  gchar * default_value; /* optional default column value */
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


struct _TableReference {
  OrthConn orth; /* inheritance */

  double line_width;
  double dashlength;
  DiaLineStyle line_style;
  Color line_color;
  Color text_color;

  char *start_point_desc;
  char *end_point_desc;
  Arrow end_arrow;
  double corner_radius;

  DiaFont *normal_font;
  double normal_font_height;

  /* computed data */

  double sp_desc_width;         /* start-point */
  Point sp_desc_pos;            /* start-point */
  DiaAlignment sp_desc_text_align; /* start-point */
  double ep_desc_width;         /* end-point */
  Point ep_desc_pos;            /* end-point */
  DiaAlignment ep_desc_text_align; /* end-point */
};

G_END_DECLS
