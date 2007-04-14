/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
#ifndef MENACE_H
#define DIA_IMAGE_H

#include "diatypes.h"
#include <gdk/gdktypes.h>
#include <gtk/gtk.h>
#include "geometry.h"


typedef struct _SISSI_Property_Menace SISSI_Property_Menace;
typedef struct _SISSI_Property_Menace_Widget SISSI_Property_Menace_Widget;

struct _SISSI_Property_Menace {
  gchar 	*label,*comments;
  float action,detection,vulnerability;
};

struct _SISSI_Property_Menace_Widget {
  GtkWidget 	*label,*comments,*action,*detection,*vulnerability;
};

extern void create_new_properties_menace(gchar *label_menace, SISSI_Property_Menace *property);
extern GList *clear_list_property_menace(GList *list);
extern GList *clear_list_property_menace_widget (GList *list);
extern SISSI_Property_Menace *copy_menace(gchar *label_menace,gchar *comments_menace,float action,float detection,float vulnerability);

#endif /* MENACE_H */


