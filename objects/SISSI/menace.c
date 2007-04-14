/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * SISSI diagram -  adapted by Luc Cessieux
 * This class could draw the system security
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <glib.h>
#include "menace.h"

void create_new_properties_menace(gchar *label_menace, SISSI_Property_Menace *property)
{
/*********** label **********/  
  if (property->label != NULL)
    g_free(property->label);
  
  if (label_menace && label_menace[0])
    property->label = g_strdup (label_menace);
  else
    property->label = NULL;
    
/*********** comments **********/
  if (property->comments != NULL)
    g_free(property->comments);
  property->comments = g_strdup ("");

 property->action=property->detection=property->vulnerability=0.0f;
}

SISSI_Property_Menace *copy_menace(gchar *label_menace,gchar *comments_menace,float action,float detection,float vulnerability)
{
  SISSI_Property_Menace *property;
  property = g_new0(SISSI_Property_Menace, 1);

/********** gestion of label ************/
  if (property->label != NULL)
    g_free(property->label);
  if (label_menace && label_menace[0])
    property->label = g_strdup (label_menace);
  else
    property->label = NULL;

/********** gestion of comments ************/
  if (property->comments != NULL)
    g_free(property->comments);
  if (comments_menace && comments_menace[0])
    property->comments = g_strdup (comments_menace);
  else
    property->comments = NULL;

  property->action=action;
  property->detection=detection;
  property->vulnerability=vulnerability;
  
  return property;

}

GList *clear_list_property_menace (GList *list)
{  
  while (list != NULL) {
    SISSI_Property_Menace *property_menace = (SISSI_Property_Menace *) list->data;
    g_free(property_menace->label);
    g_free(property_menace->comments);

    g_free(property_menace);
    list = g_list_next(list);
  }
  g_list_free(list);
  return list;
}

GList *clear_list_property_menace_widget (GList *list)
{  
  while (list != NULL) {
    SISSI_Property_Menace_Widget *property_menace = (SISSI_Property_Menace_Widget *) list->data;
    gtk_widget_destroy(property_menace->label);
    gtk_widget_destroy(property_menace->comments);
    gtk_widget_destroy(property_menace->action);
    gtk_widget_destroy(property_menace->detection);
    gtk_widget_destroy(property_menace->vulnerability);
    g_free(property_menace);
    list = g_list_next(list);
  }
  g_list_free(list);
  return list;
}
