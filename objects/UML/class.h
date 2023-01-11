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

#pragma once

#include <glib.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"

#include "uml.h"

G_BEGIN_DECLS

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

  real line_width;
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

  gboolean allow_resizing;
  /* Calculated variables: */

  real namebox_height;
  char *stereotype_string;

  real attributesbox_height;

  real operationsbox_height;

  int max_wrapped_line_width;
  /*! chached for resizing */
  real min_width;

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

void umlclass_dialog_free (UMLClassDialog *dialog);
GtkWidget *umlclass_get_properties(UMLClass *umlclass, gboolean is_default);
DiaObjectChange *umlclass_apply_props_from_dialog(UMLClass *umlclass, GtkWidget *widget);
void umlclass_calculate_data(UMLClass *umlclass);
void umlclass_update_data(UMLClass *umlclass);

void umlclass_sanity_check(UMLClass *c, gchar *msg);

G_END_DECLS
