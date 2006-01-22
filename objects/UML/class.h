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
 * File:    class.h
 *
 * Purpose: This is the interface file for the class icon and dialog. 
 */
 
/** \file objects/UML/class.h  Declaration of the 'UML - Class' type */
#ifndef CLASS_H
#define CLASS_H

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "widgets.h"

#include "uml.h"

#define DIA_OBJECT(x) (DiaObject*)(x)

/** The number of regular connectionpoints on the class (not cps for
 * attributes and operands and not the mainpoint). */
#define UMLCLASS_CONNECTIONPOINTS 8
/** default wrap length for member functions */
#define UMLCLASS_WRAP_AFTER_CHAR 40
/** default wrap length for comments */
#define UMLCLASS_COMMENT_LINE_LENGTH 40

/* The code behind the following preprocessor symbol should stay disabled until 
 * the dynamic relocation of connection points (caused by attribute and 
 * operation changes) is taken into account. It probably has other issues we are 
 * not aware of yet. Some more information maybe available at 
 * http://bugzilla.gnome.org/show_bug.cgi?id=303301
 *
 * Enabling 29/7 2005: Not known to cause any problems.
 * 7/11 2005: Still seeing problems after dialog update, needs work.  --LC
 * 18/1 2006: Can't make it break, enabling.
 */
#define UML_MAINPOINT 1


typedef struct _UMLClass UMLClass;
typedef struct _UMLClassDialog UMLClassDialog;

/** 
 * \brief The most complex object Dia has
 *
 * What should I say? Don't try this at home :)
 */
struct _UMLClass {
  Element element; /**< inheritance */

  /** static connection point storage,  the mainpoint must be behind the dynamics in Element::connections */
#ifdef UML_MAINPOINT
  ConnectionPoint connections[UMLCLASS_CONNECTIONPOINTS + 1];
#else
  ConnectionPoint connections[UMLCLASS_CONNECTIONPOINTS];
#endif

  /* Class info: */

  real font_height;
  real abstract_font_height;
  real polymorphic_font_height;
  real classname_font_height;
  real abstract_classname_font_height;
  real comment_font_height;

  DiaFont *normal_font;
  DiaFont *abstract_font;
  DiaFont *polymorphic_font;
  DiaFont *classname_font;
  DiaFont *abstract_classname_font;
  DiaFont *comment_font;
  
  char *name;
  char *stereotype; /**< NULL if no stereotype */
  char *comment; /**< Comments on the class */
  int abstract;
  int suppress_attributes; 
  int suppress_operations; 
  int visible_attributes; /**< ie. don't draw strings. */
  int visible_operations;
  int visible_comments;

  int wrap_operations; /**< wrap operations with many parameters */
  int wrap_after_char;
  int comment_line_length; /**< Maximum line length for comments */
  int comment_tagging; /**< bool: if the {documentation = }  tag should be used */
  
  Color line_color;
  Color fill_color;
  Color text_color;

  /** Attributes: aka member variables */
  GList *attributes;

  /** Operators: aka member functions */
  GList *operations;

  /** Template: if it's a template class */
  int template;
  /** Template parameters */
  GList *formal_params;

  /* Calculated variables: */
  
  real namebox_height;
  char *stereotype_string;
  
  real attributesbox_height;

  real operationsbox_height;
  GList *operations_wrappos;
  int max_wrapped_line_width;

  real templates_height;
  real templates_width;

  /* Dialog: */
  UMLClassDialog *properties_dialog;

  /** Until GtkList replaced by something better, set this when being
   * destroyed, and don't do umlclass_calculate_data when it is set.
   * This is to avoid a half-way destroyed list being updated.
   */
  gboolean destroyed; 
};

/**
 * \brief Very special user interface for UMLClass parametrization
 *
 * There is a (too)  tight coupling between the UMLClass and it's user interface. 
 * And the dialog is too huge in code as well as on screen.
 */
struct _UMLClassDialog {
  GtkWidget *dialog;

  GtkEntry *classname;
  GtkEntry *stereotype;
  GtkTextView *comment;

  GtkToggleButton *abstract_class;
  GtkToggleButton *attr_vis;
  GtkToggleButton *attr_supp;
  GtkToggleButton *op_vis;
  GtkToggleButton *op_supp;
  GtkToggleButton *comments_vis;
  GtkToggleButton *op_wrap;
  DiaFontSelector *normal_font;
  DiaFontSelector *abstract_font;
  DiaFontSelector *polymorphic_font;
  DiaFontSelector *classname_font;
  DiaFontSelector *abstract_classname_font;
  DiaFontSelector *comment_font;
  GtkSpinButton *normal_font_height;
  GtkSpinButton *abstract_font_height;
  GtkSpinButton *polymorphic_font_height;
  GtkSpinButton *classname_font_height;
  GtkSpinButton *abstract_classname_font_height;
  GtkSpinButton *comment_font_height;
  GtkSpinButton *wrap_after_char;  
  GtkSpinButton *comment_line_length;
  GtkToggleButton *comment_tagging;
  DiaColorSelector *text_color;
  DiaColorSelector *line_color;
  DiaColorSelector *fill_color;
  GtkLabel *max_length_label;
  GtkLabel *Comment_length_label;

  GList *disconnected_connections;
  GList *added_connections; 
  GList *deleted_connections; 

  GtkList *attributes_list;
  GtkListItem *current_attr;
  GtkEntry *attr_name;
  GtkEntry *attr_type;
  GtkEntry *attr_value;
  GtkTextView *attr_comment;
  GtkMenu *attr_visible;
  GtkOptionMenu *attr_visible_button;
  GtkToggleButton *attr_class_scope;
  
  GtkList *operations_list;
  GtkListItem *current_op;
  GtkEntry *op_name;
  GtkEntry *op_type;
  GtkEntry *op_stereotype;
  GtkTextView *op_comment;

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
  GtkTextView *param_comment;
  GtkMenu *param_kind;
  GtkOptionMenu *param_kind_button;
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

extern GtkWidget *umlclass_get_properties(UMLClass *umlclass, gboolean is_default);
extern ObjectChange *umlclass_apply_props_from_dialog(UMLClass *umlclass, GtkWidget *widget);
extern void umlclass_calculate_data(UMLClass *umlclass);
extern void umlclass_update_data(UMLClass *umlclass);

extern void umlclass_sanity_check(UMLClass *c, gchar *msg);

#endif /* CLASS_H */
