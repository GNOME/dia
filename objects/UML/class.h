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
#ifndef CLASS_H
#define CLASS_H

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "widgets.h"

#include "uml.h"

typedef struct _UMLClass UMLClass;
typedef struct _UMLClassDialog UMLClassDialog;

struct _UMLClass {
  Element element;

  ConnectionPoint connections[8];

  real font_height;
  real classname_font_height;
  DiaFont *normal_font;
  DiaFont *abstract_font;
  DiaFont *classname_font;
  DiaFont *abstract_classname_font;
  
  /* Class info: */
  utfchar *name;
  utfchar *stereotype; /* NULL if no stereotype */
  int abstract;
  int suppress_attributes; /* ie. don't draw strings. */
  int suppress_operations; /* ie. don't draw strings. */
  int visible_attributes;
  int visible_operations;
  Color color_foreground;
  Color color_background;

  /* Attributes: */
  GList *attributes;

  /* Operators: */
  GList *operations;

  /* Template: */
  int template;
  GList *formal_params;

  /* Calculated variables: */
  real font_ascent;
  
  real namebox_height;
  utfchar *stereotype_string;
  
  real attributesbox_height;
  int num_attributes;
  utfchar **attributes_strings;
  
  real operationsbox_height;
  int num_operations;
  utfchar **operations_strings;

  real templates_height;
  real templates_width;
  int num_templates;
  utfchar **templates_strings;

  /* Dialog: */
  UMLClassDialog *properties_dialog;
};

struct _UMLClassDialog {
  GtkWidget *dialog;

  GtkEntry *classname;
  GtkEntry *stereotype;
  GtkToggleButton *abstract_class;
  GtkToggleButton *attr_vis;
  GtkToggleButton *attr_supp;
  GtkToggleButton *op_vis;
  GtkToggleButton *op_supp;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;

  GList *disconnected_connections;
  GList *added_connections; 
  GList *deleted_connections; 

  GtkList *attributes_list;
  GtkListItem *current_attr;
  GtkEntry *attr_name;
  GtkEntry *attr_type;
  GtkEntry *attr_value;
  GtkMenu *attr_visible;
  GtkOptionMenu *attr_visible_button;
  GtkToggleButton *attr_class_scope;
  
  GtkList *operations_list;
  GtkListItem *current_op;
  GtkEntry *op_name;
  GtkEntry *op_type;
  GtkMenu *op_visible;
  GtkOptionMenu *op_visible_button;
  GtkToggleButton *op_class_scope;
  GtkMenu *op_inheritance_type;
  GtkOptionMenu *op_inheritance_type_button;
  GtkToggleButton *op_query;  

  
  GtkList *parameters_list;
  GtkListItem *current_param;
  GtkEntry *param_name;
  GtkEntry *param_type;
  GtkEntry *param_value;
  GtkWidget *param_new_button;
  GtkWidget *param_delete_button;
  GtkWidget *param_up_button;
  GtkWidget *param_down_button;
  
  GtkList *templates_list;
  GtkListItem *current_templ;
  GtkToggleButton *templ_template;
  GtkEntry *templ_name;
  GtkEntry *templ_type;
};

extern GtkWidget *umlclass_get_properties(UMLClass *umlclass);
extern ObjectChange *umlclass_apply_properties(UMLClass *umlclass);
extern void umlclass_calculate_data(UMLClass *umlclass);
extern void umlclass_update_data(UMLClass *umlclass);

#endif /* CLASS_H */
