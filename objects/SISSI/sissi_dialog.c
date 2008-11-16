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

#include <assert.h>
#undef GTK_DISABLE_DEPRECATED /* GtkOptionMenu, gtk_object_get_user_data, ... */
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "object.h"
#include "objchange.h"
#include "intl.h"
#include "sissi.h"

#include "sissi_dialog.h"

/************ function of menace list copy*******/
extern GList *menace_list_copy(GList *list_old, GList *list_new)
{
  unsigned int i;
  const gchar *s; 

  for (i=0;i<g_list_length(list_old);i++)
  {

   SISSI_Property_Menace *new_property_menace;
   new_property_menace=g_new0(SISSI_Property_Menace,1);
		/*Label*/
   s = ((SISSI_Property_Menace *)(g_list_nth(list_old,i)->data))->label;
   if (s && s[0])
      new_property_menace->label = g_strdup (s);
   else
      new_property_menace->label = NULL;
		/* comments */
   s = ((SISSI_Property_Menace *)(g_list_nth(list_old,i)->data))->comments;
   if (s && s[0])
      new_property_menace->comments = g_strdup (s);
   else
      new_property_menace->comments = NULL;

   new_property_menace->action = ((SISSI_Property_Menace *)(g_list_nth(list_old,i)->data))->action;
   
   new_property_menace->detection = ((SISSI_Property_Menace *)(g_list_nth(list_old,i)->data))->detection;

   new_property_menace->vulnerability = ((SISSI_Property_Menace *)(g_list_nth(list_old,i)->data))->vulnerability;
   
   list_new=g_list_append(list_new,new_property_menace);
  }
  return list_new;
}

/************ copy function of menace list in dailog box *******/
extern void menace_list_copy_gtk(GList *list_old, GList *list_dialog)
{
/*  */
}

static GList *clear_list_property_widget(GList *list)
{  
  while (list != NULL) {
    SISSI_Property_Widget *property = (SISSI_Property_Widget *) list->data;
    gtk_widget_destroy((GtkWidget *)property->label);
    gtk_widget_destroy((GtkWidget *)property->value);
    gtk_widget_destroy((GtkWidget *)property->description);
    g_free(property);
    list = g_list_next(list);
  }
  g_list_free(list);
  list=NULL;
  return list;
}

static GList *clear_list_url_doc_widget(GList *list)
{  
  while (list != NULL) {
    Url_Docs_Widget *url_doc = (Url_Docs_Widget *) list->data;
    gtk_widget_destroy((GtkWidget *)url_doc->label);
    gtk_widget_destroy((GtkWidget *)url_doc->url);
    gtk_widget_destroy((GtkWidget *)url_doc->description);
    g_free(url_doc);
    list = g_list_next(list);
  }
  g_list_free(list);
  list=NULL;
  return list;
}

static void
object_sissi_set_state(ObjetSISSI *object_sissi, SISSIState *state)
{
  g_free(state);
  object_sissi_update_data(object_sissi, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static void
object_sissi_change_apply(SISSIChange *change, DiaObject *obj)
{
/*    fprintf(stderr,"espion object_sissi_change_apply\n");     */
}

static void object_sissi_change_revert(SISSIChange *change, DiaObject *obj)
{
/*
    fprintf(stderr,"espion object_sissi_change_revert\n");*/
}

static void object_sissi_change_free(SISSIChange *change)
{
/*
  g_list_free(free_list);
*/
}

static ObjectChange *new_sissi_change(ObjetSISSI *object_sissi, SISSIState *state, GList *added, GList *deleted, GList *disconnected)
{
  SISSIChange *change;

  change = g_new0(SISSIChange, 1);
  
  change->obj_change.apply =
    (ObjectChangeApplyFunc) object_sissi_change_apply;
  change->obj_change.revert =
    (ObjectChangeRevertFunc) object_sissi_change_revert;
  change->obj_change.free =
    (ObjectChangeFreeFunc) object_sissi_change_free;

  return (ObjectChange *)change;
}

/********************************************************
 ******************** menace  part **********************
 ********************************************************/

static void properties_menaces_read_from_dialog(ObjetSISSI *object_sissi, SISSIDialog *prop_dialog)
{

  unsigned int i;
/************ copy the list of menaces *******/
  for (i=0;i<g_list_length(object_sissi->properties_menaces);i++)
  {

     ((SISSI_Property_Menace *)(g_list_nth(object_sissi->properties_menaces,i)->data))->comments = g_strdup(gtk_entry_get_text((GtkEntry *)(((SISSI_Property_Menace_Widget *)(g_list_nth(prop_dialog->properties_menaces,i)->data))->comments)));

     ((SISSI_Property_Menace *)(g_list_nth(object_sissi->properties_menaces,i)->data))->action = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(((SISSI_Property_Menace_Widget *)(g_list_nth(prop_dialog->properties_menaces,i)->data))->action));
     ((SISSI_Property_Menace *)(g_list_nth(object_sissi->properties_menaces,i)->data))->detection = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(((SISSI_Property_Menace_Widget *)(g_list_nth(prop_dialog->properties_menaces,i)->data))->detection));
     ((SISSI_Property_Menace *)(g_list_nth(object_sissi->properties_menaces,i)->data))->vulnerability = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(((SISSI_Property_Menace_Widget *)(g_list_nth(prop_dialog->properties_menaces,i)->data))->vulnerability));
  }
/*  fprintf(stderr,"espion properties_menaces_read_from_dialog fin\n");*/
}

static void properties_menaces_create_page(GtkNotebook *notebook,  ObjetSISSI *object_sissi)
{
  unsigned int i;
  SISSIDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *pScroll;
  GtkWidget *vbox;
  GtkWidget *table;
  prop_dialog = object_sissi->properties_dialog;
  
  /* Class page:*/
  page_label = gtk_label_new_with_mnemonic (_("_Menace"));
  
  vbox = gtk_vbox_new(FALSE, 0);


  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

pScroll = gtk_scrolled_window_new(NULL, NULL);
/*gtk_window_set_default_size(GTK_WINDOW(pScroll), 300, 200);*/
gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScroll), vbox);

  
  table = gtk_table_new (43, 8, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /********** label of board **********/
  label = gtk_label_new (_("Menace"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  label = gtk_label_new (_("P Action"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
  label = gtk_label_new (_("P Detection"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
  label = gtk_label_new (_("Vulnerability"));
  
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 0, 1);
  label = gtk_label_new (_("Menace"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 5, 0, 1);
  label = gtk_label_new (_("P Action"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 5, 6, 0, 1);
  label = gtk_label_new (_("P Detection"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 6, 7, 0, 1);
  label = gtk_label_new (_("Vulnerability"));
  gtk_table_attach_defaults (GTK_TABLE (table), label, 7, 8, 0, 1);

  /************* values of board *********/
/*********   GtkObject *adj; **********/
  for (i=0;i<g_list_length(object_sissi->properties_menaces);i++)
  {
   SISSI_Property_Menace_Widget *property_menace_widget;
   property_menace_widget=g_malloc0(sizeof(SISSI_Property_Menace_Widget));

   property_menace_widget->label = gtk_label_new (NULL);
   gtk_label_set_text(GTK_LABEL(property_menace_widget->label), _(((SISSI_Property_Menace *)(g_list_nth(object_sissi->properties_menaces,i)->data))->label));
   

property_menace_widget->comments = gtk_entry_new();

/*   adj = gtk_adjustment_new( umlclass->wrap_after_char, 0.0, 200.0, 1.0, 5.0, 0);
//   prop_dialog->wrap_after_char = GTK_SPIN_BUTTON(gtk_spin_button_new( GTK_ADJUSTMENT( adj), 0.1, 0));
//   gtk_spin_button_set_numeric( GTK_SPIN_BUTTON( prop_dialog->wrap_after_char), TRUE);
//   gtk_spin_button_set_snap_to_ticks( GTK_SPIN_BUTTON( prop_dialog->wrap_after_char), TRUE);*/
   
   property_menace_widget->action = gtk_spin_button_new_with_range(0, 3, 1); 
   property_menace_widget->detection = gtk_spin_button_new_with_range(0, 3, 1);
   property_menace_widget->vulnerability = gtk_spin_button_new_with_range(0, 3, 1);
         
   gtk_table_attach_defaults (GTK_TABLE (table), property_menace_widget->label, (i%2)*4, (i%2)*4+1, i*2+1-2*(i%2), i*2+2-2*(i%2));
   gtk_table_attach_defaults (GTK_TABLE (table), property_menace_widget->action, (i%2)*4+1, (i%2)*4+2, i*2+1-2*(i%2), i*2+2-2*(i%2));
   gtk_table_attach_defaults (GTK_TABLE (table), property_menace_widget->detection, (i%2)*4+2, (i%2)*4+3, i*2+1-2*(i%2), i*2+2-2*(i%2));
   gtk_table_attach_defaults (GTK_TABLE (table), property_menace_widget->vulnerability, (i%2)*4+3,(i%2)*4+4, i*2+1-2*(i%2), i*2+2-2*(i%2));
   gtk_table_attach_defaults (GTK_TABLE (table), property_menace_widget->comments, (i%2)*4, (i%2)*4+4, i*2+2-2*(i%2), i*2+3-2*(i%2));

   /* after the création of the elements, we put there in the widget list */
   prop_dialog->properties_menaces=g_list_append(prop_dialog->properties_menaces,property_menace_widget);
  }
  
  gtk_widget_show_all (vbox);
gtk_widget_show_all (pScroll);
  gtk_widget_show (page_label);
  gtk_notebook_append_page(notebook, pScroll, page_label);
}


static void properties_others_read_from_dialog(ObjetSISSI *object_sissi, SISSIDialog *prop_dialog)
{
  /* GList *list; */
  unsigned int i;
     /************** classification list box *************/
  if (GTK_IS_OPTION_MENU(object_sissi->properties_dialog->confidentiality)) {
   object_sissi->confidentiality = g_strdup((char *)(gtk_object_get_user_data(
           GTK_OBJECT(GTK_OPTION_MENU(object_sissi->properties_dialog->confidentiality)->menu_item))));
  } else {
    object_sissi->confidentiality = g_strdup((char *)strtol(gtk_entry_get_text(GTK_ENTRY(object_sissi->properties_dialog->confidentiality)), NULL, 0));
  }

  if (GTK_IS_OPTION_MENU(object_sissi->properties_dialog->integrity)) {
   object_sissi->integrity = g_strdup((char *)(gtk_object_get_user_data(
           GTK_OBJECT(GTK_OPTION_MENU(object_sissi->properties_dialog->integrity)->menu_item))));
  } else {
    object_sissi->integrity = g_strdup((char *)strtol(gtk_entry_get_text(GTK_ENTRY(object_sissi->properties_dialog->integrity)), NULL, 0));
  }
  
  if (GTK_IS_OPTION_MENU(object_sissi->properties_dialog->disponibility_level)) {
   object_sissi->disponibility_level = g_strdup((char *)(gtk_object_get_user_data(
           GTK_OBJECT(GTK_OPTION_MENU(object_sissi->properties_dialog->disponibility_level)->menu_item))));
  } else {
    object_sissi->disponibility_level = g_strdup((char *)strtol(gtk_entry_get_text(GTK_ENTRY(object_sissi->properties_dialog->disponibility_level)), NULL, 0));
  }
  
  object_sissi->disponibility_value = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(object_sissi->properties_dialog->disponibility_value));
  
  
    /************ Entity list box*******************/
  if (GTK_IS_OPTION_MENU(object_sissi->properties_dialog->entity)) {
   object_sissi->entity = g_strdup((char *)(gtk_object_get_user_data(
           GTK_OBJECT(GTK_OPTION_MENU(object_sissi->properties_dialog->entity)->menu_item))));
  } else {
    object_sissi->entity = g_strdup((char *)strtol(gtk_entry_get_text(GTK_ENTRY(object_sissi->properties_dialog->entity)), NULL, 0));
  }

  
    /* to copy the properties box list of dialog to propreties object */
  
  object_sissi->properties_others=clear_list_property(object_sissi->properties_others);
   for (i=0;i<g_list_length(prop_dialog->properties_others);i++)
   {
      SISSI_Property *properties_others;
      properties_others=g_malloc0(sizeof(SISSI_Property));
      if (i < object_sissi->nb_others_fixes)
      {
        properties_others->label = g_strdup(gtk_label_get_text(GTK_LABEL(((SISSI_Property_Widget *)(g_list_nth(prop_dialog->properties_others,i)->data))->label)));
        properties_others->description = g_strdup(gtk_label_get_text(GTK_LABEL(((SISSI_Property_Widget *)(g_list_nth(prop_dialog->properties_others,i)->data))->description)));
      }
      else
      {
        properties_others->label = g_strdup(gtk_entry_get_text((GtkEntry *)(((SISSI_Property_Widget *)(g_list_nth(prop_dialog->properties_others,i)->data))->label)));
        properties_others->description = g_strdup(gtk_entry_get_text((GtkEntry *)(((SISSI_Property_Widget *)(g_list_nth(prop_dialog->properties_others,i)->data))->description)));      
      }
      properties_others->value = g_strdup(gtk_entry_get_text((GtkEntry *)(((SISSI_Property_Widget *)(g_list_nth(prop_dialog->properties_others,i)->data))->value)));
      if((strcmp(properties_others->label,"")==0) && (strcmp(properties_others->value,"")==0) && (strcmp(properties_others->description,"")==0))
      {
	g_free(properties_others->label);
	g_free(properties_others->value);
	g_free(properties_others->description);
	g_free(properties_others);
      }
      else
      {
      	object_sissi->properties_others=g_list_append(object_sissi->properties_others, properties_others);
      }
   }
  object_sissi->name = g_strdup(gtk_entry_get_text((GtkEntry *)prop_dialog->name));
}

static void
others_new_callback(GtkWidget *button, ObjetSISSI *object_sissi)
{
    unsigned int i;
    SISSI_Property_Widget *property_other;
    SISSIDialog *prop_dialog;
  /* DiaObject *obj; */
  	prop_dialog = object_sissi->properties_dialog;    
	i=prop_dialog->nb_properties_others;
	
	property_other=g_malloc0(sizeof(SISSI_Property_Widget));
	
	property_other->label = gtk_entry_new();
	property_other->value = gtk_entry_new();
	property_other->description= gtk_entry_new();
	
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), (GtkWidget *)property_other->label, 0, 1, i+5, i+6);
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), (GtkWidget *)property_other->value, 1,2,i+5,i+6);
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), (GtkWidget *)property_other->description, 2,3,i+5,i+6);
	/* after creation we put elemnts in widget list */
	prop_dialog->properties_others=g_list_append(prop_dialog->properties_others,property_other);
	
	gtk_widget_show (prop_dialog->table_others);
	gtk_widget_show_all (prop_dialog->vbox_others);
  	gtk_widget_show (prop_dialog->page_label_others);	
	prop_dialog->nb_properties_others=i+1;
}


static void properties_others_create_page(GtkNotebook *notebook,  ObjetSISSI *object_sissi)
{
  SISSIDialog *prop_dialog;
  GtkWidget *label,*button;
  GtkWidget *pScroll_others;


  GtkWidget *menu_classification, *menu_integrity, *menu_disponibility_level/* , *disponibility_value */;  
  GtkWidget *menu_entity;
  unsigned int j;  
  
  prop_dialog = object_sissi->properties_dialog;
  
  /* Other properties of page: */
  prop_dialog->page_label_others = gtk_label_new_with_mnemonic (_("Other properties"));

  prop_dialog->vbox_others = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (prop_dialog->vbox_others), 0);


pScroll_others = gtk_scrolled_window_new(NULL, NULL);
gtk_scrolled_window_set_policy(pScroll_others,GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
/*gtk_window_set_default_size(GTK_WINDOW(pScroll_others), 300, 200);*/

gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pScroll_others), prop_dialog->vbox_others);

  prop_dialog->table_others = gtk_table_new (43, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (prop_dialog->vbox_others), prop_dialog->table_others, FALSE, FALSE, 0);
  
  /**** Selection menu of classification  ****/

  object_sissi->properties_dialog->confidentiality = gtk_option_menu_new();
  menu_classification = gtk_menu_new();
  object_sissi->properties_dialog->integrity= gtk_option_menu_new();
  menu_integrity = gtk_menu_new();
  object_sissi->properties_dialog->disponibility_level = gtk_option_menu_new();
  menu_disponibility_level = gtk_menu_new();
  
  for (j = 0; property_classification_data[j].label != NULL; j++) {
      GtkWidget *item = gtk_menu_item_new_with_label(_(property_classification_data[j].label));
      gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_classification_data[j].value));
      gtk_container_add(GTK_CONTAINER(menu_classification), item);
      gtk_widget_show(item);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(object_sissi->properties_dialog->confidentiality), menu_classification);
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), object_sissi->properties_dialog->confidentiality, 0, 2, 0, 1);
 /************* end of menu **********/
  for (j = 0; property_integrity_data[j].label != NULL; j++) {
      GtkWidget *item = gtk_menu_item_new_with_label(_(property_integrity_data[j].label));
      gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_integrity_data[j].value));
      gtk_container_add(GTK_CONTAINER(menu_integrity), item);
      gtk_widget_show(item);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(object_sissi->properties_dialog->integrity), menu_integrity);
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), object_sissi->properties_dialog->integrity, 2, 4, 0, 1);

  object_sissi->properties_dialog->disponibility_value = gtk_spin_button_new_with_range(0, 100, 1);
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), object_sissi->properties_dialog->disponibility_value, 0, 1, 1, 2);

  for (j = 0; property_disponibility_level_data[j].label != NULL; j++) {
      GtkWidget *item = gtk_menu_item_new_with_label(_(property_disponibility_level_data[j].label));
      gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_disponibility_level_data[j].value));
      gtk_container_add(GTK_CONTAINER(menu_disponibility_level), item);
      gtk_widget_show(item);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(object_sissi->properties_dialog->disponibility_level), menu_disponibility_level);
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), object_sissi->properties_dialog->disponibility_level, 1, 2, 1, 2);

 /*********************** Selection of entity menu ***************************/
  object_sissi->properties_dialog->entity = gtk_option_menu_new();
  menu_entity = gtk_menu_new();
  /****** Choice of list in function of object type ***********/
  if (0==strcmp(object_sissi->entity_type,"MATERIAL"))
  {  
    for (j = 0; property_material_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_material_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_material_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"SOFTWARE"))
  {  
    for (j = 0; property_logiciel_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_logiciel_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_logiciel_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"NETWORK"))
  {  
    for (j = 0; property_reseau_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_reseau_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_reseau_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PERSON"))
  {  
    for (j = 0; property_personnel_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_personnel_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_personnel_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHYSIC"))
  {  
    for (j = 0; property_physic_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_DETECTION_THERMIC"))
  {  
    for (j = 0; property_physic_detecTHERMIC_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_detecTHERMIC_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_detecTHERMIC_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_DETECTION_FIRE"))
  {  
    for (j = 0; property_physic_detecFIRE_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_detecFIRE_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_detecFIRE_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_DETECTION_WATER"))
  {  
    for (j = 0; property_physic_detecWATER_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_detecWATER_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_detecWATER_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_DETECTION_AIR"))
  {  
    for (j = 0; property_physic_detecAIR_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_detecAIR_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_detecAIR_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_DETECTION_ENERGY"))
  {  
    for (j = 0; property_physic_detecENERGY_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_detecENERGY_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_detecENERGY_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_DETECTION_INTRUSION"))
  {  
    for (j = 0; property_physic_detecINTRUSION_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_detecINTRUSION_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_detecINTRUSION_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_ACTION_THERMIC"))
  {  
    for (j = 0; property_physic_actionTHERMIC_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_actionTHERMIC_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_actionTHERMIC_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_ACTION_FIRE"))
  {  
    for (j = 0; property_physic_actionFIRE_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_actionFIRE_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_actionFIRE_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_ACTION_WATER"))
  {  
    for (j = 0; property_physic_actionWATER_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_actionWATER_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_actionWATER_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_ACTION_AIR"))
  {  
    for (j = 0; property_physic_actionAIR_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_actionAIR_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_actionAIR_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_ACTION_ENERGY"))
  {  
    for (j = 0; property_physic_actionENERGY_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_actionENERGY_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_actionENERGY_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"PHY_ACTION_INTRUSION"))
  {  
    for (j = 0; property_physic_actionINTRUSION_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_physic_actionINTRUSION_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_physic_actionINTRUSION_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"ORGANIZATION"))
  {  
    for (j = 0; property_organisation_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_organisation_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_organisation_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }
  if (0==strcmp(object_sissi->entity_type,"SYSTEM"))
  {  
    for (j = 0; property_system_data[j].label != NULL; j++) {
        GtkWidget *item = gtk_menu_item_new_with_label(_(property_system_data[j].label));
        gtk_object_set_user_data(GTK_OBJECT(item),GUINT_TO_POINTER(property_system_data[j].value));
        gtk_container_add(GTK_CONTAINER(menu_entity), item);
      	gtk_widget_show(item);
    }
  }

  gtk_option_menu_set_menu(GTK_OPTION_MENU(object_sissi->properties_dialog->entity), menu_entity);
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), object_sissi->properties_dialog->entity, 2, 4, 1, 2);
/********** selection menu of entity *********/




  /********** Label of board **********/
  label = gtk_label_new (_("Label"));
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), label, 0, 1, 2, 3);
  label = gtk_label_new (_("value"));
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), label, 1, 2, 2, 3);
  label = gtk_label_new (_("Description"));
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), label, 2, 3, 2, 3);

  /**** board of value ***************/
  label = gtk_label_new (_("Name"));
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), label, 0, 1, 3, 4);
  object_sissi->properties_dialog->name=gtk_entry_new();
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), (GtkWidget *)object_sissi->properties_dialog->name, 1,2,3,4);

  button = gtk_button_new_with_mnemonic (_("_New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(others_new_callback),
		      object_sissi);
 
 gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), button, 2, 3, 3, 4);


  object_sissi->properties_dialog->nb_properties_others=4;

  gtk_widget_show_all (prop_dialog->vbox_others);
  gtk_widget_show (prop_dialog->page_label_others);
  gtk_widget_show_all (pScroll_others);

  gtk_notebook_append_page(notebook, pScroll_others, prop_dialog->page_label_others);

}

/******************************************
*             List of documents         *
*******************************************/

static void
document_new_callback(GtkWidget *button, ObjetSISSI *object_sissi)
{
    unsigned int i;
    Url_Docs_Widget *doc_widget;
    SISSIDialog *prop_dialog;

  	prop_dialog = object_sissi->properties_dialog;    
	i=prop_dialog->nb_doc +1;
	
	doc_widget=g_malloc0(sizeof(Url_Docs_Widget));
	
	doc_widget->label = gtk_entry_new();
	doc_widget->url = gtk_entry_new();
	doc_widget->description= gtk_entry_new();
	
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), (GtkWidget *)doc_widget->label, 0, 1, i+1, i+2);
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), (GtkWidget *)doc_widget->url, 1,2,i+1,i+2);
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), (GtkWidget *)doc_widget->description, 2,3,i+1,i+2);
	/* after create we add element in the wideget list */
	prop_dialog->url_docs=g_list_append(prop_dialog->url_docs,doc_widget);
	
	gtk_widget_show (prop_dialog->table_document);
	gtk_widget_show_all (prop_dialog->vbox_document);
  	gtk_widget_show (prop_dialog->page_label_document);	
	prop_dialog->nb_doc=i+1;
}

static void
document_fill_in_dialog(ObjetSISSI *object_sissi)
{
  /* GList *list; */
  SISSIDialog *prop_dialog;
  unsigned int i;
  
  prop_dialog = object_sissi->properties_dialog;
   prop_dialog->url_docs=clear_list_url_doc_widget(prop_dialog->url_docs);

   g_list_free (prop_dialog->url_docs);
   prop_dialog->url_docs = NULL;  
  
   for (i=0;i<g_list_length(object_sissi->url_docs);i++)
   {
	Url_Docs_Widget *doc_widget;
	doc_widget=g_malloc0(sizeof(Url_Docs_Widget));
	doc_widget->label = gtk_entry_new();
	gtk_entry_set_text((GtkEntry *)doc_widget->label, (((Url_Docs *)(g_list_nth(object_sissi->url_docs,i)->data))->label));
	doc_widget->url = gtk_entry_new();
	gtk_entry_set_text((GtkEntry *)doc_widget->url, (((Url_Docs *)(g_list_nth(object_sissi->url_docs,i)->data))->url));
	doc_widget->description= gtk_entry_new();
	gtk_entry_set_text((GtkEntry *)doc_widget->description, (((Url_Docs *)(g_list_nth(object_sissi->url_docs,i)->data))->description));
	
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), (GtkWidget *)doc_widget->label, 0, 1, i+2, i+3);
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), (GtkWidget *)doc_widget->url, 1,2,i+2, i+3);
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), (GtkWidget *)doc_widget->description, 2,3,i+2, i+3);
	/************ idem ************/
	prop_dialog->url_docs=g_list_append(prop_dialog->url_docs,doc_widget);
   }
   prop_dialog->nb_doc=i;
   gtk_widget_show (prop_dialog->table_document);
   gtk_widget_show_all (prop_dialog->vbox_document);
   gtk_widget_show (prop_dialog->page_label_document);
}

static void document_read_from_dialog(ObjetSISSI *object_sissi, SISSIDialog *prop_dialog)
{
  /* GList *list; */
  unsigned int i;
  
  object_sissi->url_docs=clear_list_url_doc(object_sissi->url_docs);
   for (i=0;i<g_list_length(prop_dialog->url_docs);i++)
   {
      Url_Docs *doc;
      doc=g_malloc0(sizeof(Url_Docs));
      doc->label = g_strdup(gtk_entry_get_text((GtkEntry *)(((Url_Docs_Widget *)(g_list_nth(prop_dialog->url_docs,i)->data))->label)));
      doc->url = g_strdup(gtk_entry_get_text((GtkEntry *)(((Url_Docs_Widget *)(g_list_nth(prop_dialog->url_docs,i)->data))->url)));
      doc->description = g_strdup(gtk_entry_get_text((GtkEntry *)(((Url_Docs_Widget *)(g_list_nth(prop_dialog->url_docs,i)->data))->description)));
      if((strcmp(doc->label,"")==0)&&(strcmp(doc->url,"")==0)&&(strcmp(doc->description,"")==0))
      {
	g_free(doc->label);
	g_free(doc->url);
	g_free(doc->description);
	g_free(doc);
      }
      else
      {
      	object_sissi->url_docs=g_list_append(object_sissi->url_docs, doc);
      }
   }  
}

static void document_create_page(GtkNotebook *notebook, ObjetSISSI *object_sissi)
{
  SISSIDialog *prop_dialog;
  GtkWidget *label,*button;
  /* GtkWidget *menu_classification, *menu_integrity, *menu_disponibility_level, *disponibility_value;  
  GtkWidget *menu_entity; */
 
  prop_dialog = object_sissi->properties_dialog;
  
  /* Other properties of page */
  prop_dialog->page_label_document = gtk_label_new_with_mnemonic (_("Documents"));
  
  prop_dialog->vbox_document = gtk_vbox_new(FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (prop_dialog->vbox_document), 0);
  
  prop_dialog->table_document = gtk_table_new (43, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (prop_dialog->vbox_document), prop_dialog->table_document, FALSE, FALSE, 0);
  
  
  button = gtk_button_new_with_mnemonic (_("_New"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(document_new_callback),
		      object_sissi);
 
 gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), button, 0, 1, 0, 1);
    
  label = gtk_label_new (_("Document title"));
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), label, 0, 1, 1, 2);
  label = gtk_label_new (_("URL"));
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), label, 1, 2, 1, 2);
  label = gtk_label_new (_("Description"));
  gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_document), label, 2, 3, 1, 2);

  gtk_widget_show_all (prop_dialog->vbox_document);
  gtk_widget_show (prop_dialog->page_label_document);
  gtk_notebook_append_page(notebook, prop_dialog->vbox_document, prop_dialog->page_label_document);
  prop_dialog->nb_doc=0;
}


static void switch_page_callback(GtkNotebook *notebook, GtkNotebookPage *page)
{
  ObjetSISSI *object_sissi;
  SISSIDialog *prop_dialog;

  object_sissi = (ObjetSISSI *)
    gtk_object_get_user_data(GTK_OBJECT(notebook));

  prop_dialog = object_sissi->properties_dialog;

  if (prop_dialog != NULL) {
  }
}

static void destroy_properties_dialog (GtkWidget* widget, gpointer user_data)
{

  ObjetSISSI *object_sissi = (ObjetSISSI *)user_data;
  object_sissi->properties_dialog->properties_menaces=clear_list_property_menace_widget(object_sissi->properties_dialog->properties_menaces);
  object_sissi->properties_dialog->properties_others=clear_list_property_widget(object_sissi->properties_dialog->properties_others);
  object_sissi->properties_dialog->url_docs=clear_list_url_doc_widget(object_sissi->properties_dialog->url_docs);
  gtk_widget_destroy(object_sissi->properties_dialog->confidentiality);
  gtk_widget_destroy(object_sissi->properties_dialog->integrity);
  gtk_widget_destroy(object_sissi->properties_dialog->disponibility_level);
  gtk_widget_destroy(object_sissi->properties_dialog->disponibility_value);
  gtk_widget_destroy(object_sissi->properties_dialog->entity);
  gtk_widget_destroy((GtkWidget *)object_sissi->properties_dialog->name);
  
  g_free(object_sissi->properties_dialog);
}

static void fill_in_dialog(ObjetSISSI *object_sissi)
{
  unsigned int i,pos=0;
  SISSIDialog   *prop_dialog;

  prop_dialog = object_sissi->properties_dialog;
/********* update the classification ***********/
  if (object_sissi->confidentiality!=NULL){
    for (i = 0; property_classification_data[i].label != NULL; i++) { 
      if (0==strcmp(property_classification_data[i].value,object_sissi->confidentiality)) {
	pos = i;
        break;
      }
    }
  }
  gtk_option_menu_set_history(GTK_OPTION_MENU(prop_dialog->confidentiality), pos);
  
  if (object_sissi->integrity!=NULL){
    for (i = 0; property_integrity_data[i].label != NULL; i++) { 
      if (0==strcmp(property_integrity_data[i].value,object_sissi->integrity)) {
	pos = i;
        break;
      }
    }
  }
  gtk_option_menu_set_history(GTK_OPTION_MENU(prop_dialog->integrity), pos);
  
  if (object_sissi->disponibility_level!=NULL){
    for (i = 0; property_disponibility_level_data[i].label != NULL; i++) { 
      if (0==strcmp(property_disponibility_level_data[i].value,object_sissi->disponibility_level)) {
	pos = i;
        break;
      }
    }
  }
  gtk_option_menu_set_history(GTK_OPTION_MENU(prop_dialog->disponibility_level), pos);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(prop_dialog->disponibility_value),object_sissi->disponibility_value);
  
 /****** update the entity ******/
  if (object_sissi->entity!=NULL){
    /*** choice of list by rntite type */
    if (0==strcmp(object_sissi->entity_type,"SYSTEM")){
      for (i = 0; property_system_data[i].label != NULL; i++) { 
        if (0==strcmp(property_system_data[i].value,object_sissi->entity)) {
	  pos = i;
          break;
	}
      }
    }
    if (0==strcmp(object_sissi->entity_type,"ORGANIZATION")){
      for (i = 0; property_organisation_data[i].label != NULL; i++) { 
        if (0==strcmp(property_organisation_data[i].value,object_sissi->entity)) {
	  pos = i;
          break;
	}
      }
    }
    if (0==strcmp(object_sissi->entity_type,"PHYSIC")){
      for (i = 0; property_physic_data[i].label != NULL; i++) { 
        if (0==strcmp(property_physic_data[i].value,object_sissi->entity)) {
	  pos = i;
          break;
	}
      }
    }
    if (0==strcmp(object_sissi->entity_type,"PERSON")){
      for (i = 0; property_personnel_data[i].label != NULL; i++) { 
        if (0==strcmp(property_personnel_data[i].value,object_sissi->entity)) {
	  pos = i;
          break;
	}
      }
    }
    if (0==strcmp(object_sissi->entity_type,"NETWORK")){
      for (i = 0; property_reseau_data[i].label != NULL; i++) { 
        if (0==strcmp(property_reseau_data[i].value,object_sissi->entity)) {
	  pos = i;
          break;
	}
      }
    }
    if (0==strcmp(object_sissi->entity_type,"SOFTWARE")){
      for (i = 0; property_logiciel_data[i].label != NULL; i++) { 
        if (0==strcmp(property_logiciel_data[i].value,object_sissi->entity)) {
	  pos = i;
          break;
	}
      }
    }
    if (0==strcmp(object_sissi->entity_type,"MATERIAL")){
      for (i = 0; property_material_data[i].label != NULL; i++) { 
        if (0==strcmp(property_material_data[i].value,object_sissi->entity)) {
	  pos = i;
          break;
	}
      }
    }
  }
  gtk_option_menu_set_history(GTK_OPTION_MENU(prop_dialog->entity), pos);
  
/*********** update the properties list **********/
  for (i=0;i<g_list_length(object_sissi->properties_menaces);i++)
  {
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(((SISSI_Property_Menace_Widget *)(g_list_nth(prop_dialog->properties_menaces,i)->data))->action), (((SISSI_Property_Menace *)(g_list_nth(object_sissi->properties_menaces,i)->data))->action));
	/**********************************/
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(((SISSI_Property_Menace_Widget *)(g_list_nth(prop_dialog->properties_menaces,i)->data))->detection), (((SISSI_Property_Menace *)(g_list_nth(object_sissi->properties_menaces,i)->data))->detection));
	/**********************************/
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(((SISSI_Property_Menace_Widget *)(g_list_nth(prop_dialog->properties_menaces,i)->data))->vulnerability), (((SISSI_Property_Menace *)(g_list_nth(object_sissi->properties_menaces,i)->data))->vulnerability));
	gtk_entry_set_text((GtkEntry *)((SISSI_Property_Menace_Widget *)(g_list_nth(prop_dialog->properties_menaces,i)->data))->comments, (((SISSI_Property_Menace *)(g_list_nth(object_sissi->properties_menaces,i)->data))->comments));
			/*fprintf(stderr,"espion fill 2\n");*/
  }

  if (object_sissi->name != NULL)
    gtk_entry_set_text((GtkEntry *)prop_dialog->name,(char *)object_sissi->name);
  else
    gtk_entry_set_text((GtkEntry *)prop_dialog->name, "");
    
    document_fill_in_dialog(object_sissi);

   prop_dialog->properties_others=clear_list_property_widget(prop_dialog->properties_others);
   for (i=0;i<g_list_length(object_sissi->properties_others);i++)
   {
	SISSI_Property_Widget *property_others_widget;
	property_others_widget=g_malloc0(sizeof(SISSI_Property_Widget));
	if(i < object_sissi->nb_others_fixes)
	{
	/* if the case is empty we put NULL for no have problem in traduction */
		if ((((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->label)!=NULL)
		{
			if (strcmp((((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->label),"")==0)
			{	property_others_widget->label = gtk_label_new (NULL);}
			else
			{	property_others_widget->label = gtk_label_new (_(((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->label));}
		}
		else
		{
			property_others_widget->label = gtk_label_new (NULL);
		}
		
		if ((((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->description)!=NULL)
		{
			if (strcmp((((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->description),"")==0)
			{	property_others_widget->description = gtk_label_new (NULL);}
			else
			{	property_others_widget->description = gtk_label_new (_(((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->description));}
		}
		else
		{
			property_others_widget->description = gtk_label_new (NULL);
		}

	}
	else
	{
		property_others_widget->label = gtk_entry_new();
		gtk_entry_set_text((GtkEntry *)property_others_widget->label, (((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->label));
		property_others_widget->description= gtk_entry_new();
		gtk_entry_set_text((GtkEntry *)property_others_widget->description, (((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->description));
	}
	property_others_widget->value = gtk_entry_new();
	if ((((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->value)!=NULL)
	  gtk_entry_set_text((GtkEntry *)property_others_widget->value, (((SISSI_Property *)(g_list_nth(object_sissi->properties_others,i)->data))->value));
	
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), (GtkWidget *)property_others_widget->label, 0, 1, i+5, i+6);
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), (GtkWidget *)property_others_widget->value, 1,2,i+5,i+6);
	gtk_table_attach_defaults (GTK_TABLE (prop_dialog->table_others), (GtkWidget *)property_others_widget->description, 2,3,i+5,i+6);
	prop_dialog->properties_others=g_list_append(prop_dialog->properties_others,property_others_widget);
   }
   prop_dialog->nb_properties_others=i;
  
   gtk_widget_show (prop_dialog->table_others);
   gtk_widget_show_all (prop_dialog->vbox_others);
   gtk_widget_show (prop_dialog->page_label_others);

}
/* apply the properties of dialog box to Object */
extern ObjectChange *object_sissi_apply_properties_dialog(ObjetSISSI *object_sissi)
{
  SISSIDialog *prop_dialog;
  DiaObject *obj;
  GList *added = NULL;
  GList *deleted = NULL;
  GList *disconnected = NULL;
  SISSIState *old_state = NULL;
  prop_dialog = object_sissi->properties_dialog;

  obj = &object_sissi->element.object;
    properties_menaces_read_from_dialog(object_sissi, prop_dialog);
    properties_others_read_from_dialog(object_sissi, prop_dialog);
    document_read_from_dialog(object_sissi, prop_dialog);
    object_sissi_update_data(object_sissi, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
   return new_sissi_change(object_sissi, old_state, added, deleted, disconnected);
}

static void create_dialog_pages(GtkNotebook *notebook, ObjetSISSI *object_sissi)
{
  properties_others_create_page(notebook, object_sissi);
  properties_menaces_create_page(notebook, object_sissi);
  document_create_page(notebook, object_sissi);
}

extern GtkWidget *object_sissi_get_properties_dialog(ObjetSISSI *object_sissi, gboolean is_default)
{
  SISSIDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *notebook;
  /* GtkObject *Adjust; */

  if (object_sissi->properties_dialog == NULL) {
    prop_dialog = g_new(SISSIDialog, 1);
    object_sissi->properties_dialog = prop_dialog;
  
/**** initialisation of properties list **********/

    vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);

    gtk_object_ref(GTK_OBJECT(vbox));
    gtk_object_sink(GTK_OBJECT(vbox));

   prop_dialog->dialog = vbox;


/************ initialisation of data ****************/
    prop_dialog->properties_menaces=NULL;
    prop_dialog->properties_others = NULL;
    prop_dialog->url_docs = NULL;

    notebook = gtk_notebook_new ();

    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
/* fixing the width of dialog box */
    gtk_container_set_border_width (GTK_CONTAINER (notebook), 0);

    gtk_object_set_user_data(GTK_OBJECT(notebook), (gpointer) object_sissi);
    
    gtk_signal_connect (GTK_OBJECT (notebook),
			"switch_page",
			GTK_SIGNAL_FUNC(switch_page_callback),
			(gpointer) object_sissi);
    gtk_signal_connect (GTK_OBJECT (object_sissi->properties_dialog->dialog),
		        "destroy",
			GTK_SIGNAL_FUNC(destroy_properties_dialog),
			(gpointer) object_sissi);
    
    create_dialog_pages(GTK_NOTEBOOK( notebook ), object_sissi);

    gtk_widget_show (notebook);
  }

  fill_in_dialog(object_sissi);
  gtk_widget_show (object_sissi->properties_dialog->dialog);
  return object_sissi->properties_dialog->dialog;
}

static SISSI_Property *property_copy(SISSI_Property *prop)
{
  SISSI_Property *newprop;
  newprop = g_new0(SISSI_Property, 1);
  
  newprop->label = g_strdup(prop->label);
  newprop->value = g_strdup(prop->value);
  newprop->description = g_strdup(prop->description);
  return newprop;
};
