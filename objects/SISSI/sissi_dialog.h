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

#define SISSI_DIALOG_H
#ifndef SISSI_DIALOG_H

#include <stdio.h>
#include <assert.h>
#include <glib.h>

#include "diatypes.h"
#include <gdk/gdktypes.h>

#include "geometry.h"
#include "polyshape.h"

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "widgets.h"
#include "dia_image.h"

#include "plug-ins.h"

static Propriete *propriete_copy(Propriete *Prop);
static ObjectChange *new_sissi_change(ObjetSISSI *object_sissi, SISSIState *state, GList *added, GList *deleted, GList *disconnected);

static GList *clear_list_propriete_widget (GList *list);
static GList *clear_list_url_doc_widget (GList *list);
static void properties_menaces_read_from_dialog(ObjetSISSI *object_sissi, SISSIDialog *prop_dialog);
static void properties_menaces_create_page(GtkNotebook *notebook,  ObjetSISSI *object_sissi);
static void properties_others_read_from_dialog(ObjetSISSI *object_sissi, SISSIDialog *prop_dialog);
static void properties_others_create_page(GtkNotebook *notebook,  ObjetSISSI *object_sissi);
static void switch_page_callback(GtkNotebook *notebook, GtkNotebookPage *page);
static void destroy_properties_dialog (GtkWidget* widget, gpointer user_data);
static void fill_in_dialog(ObjetSISSI *object_sissi);
static void  prorpietes_read_from_dialog(ObjetSISSI *object_sissi,SISSIDialog *prop_dialog); 

static void create_dialog_pages(GtkNotebook *notebook, ObjetSISSI *object_sissi);
static void object_sissi_free_state(SISSIState *state);
/*/static SISSIState *object_sissi_get_state(ObjetSISSI *object_sissi);*/
static void object_sissi_change_revert(SISSIChange *change, DiaObject *obj);
static void object_sissi_change_free(SISSIChange *change);

#endif /* SISSI_DIALOG_H */
