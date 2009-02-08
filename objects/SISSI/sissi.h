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

#ifndef SISSI_H
#define SISSI_H

#include <stdio.h>
#include <assert.h>
#include <glib.h>

#include "dia_dirs.h"

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

#include "menace.h"
#include "properties.h"

/************** Start of "define" daclaration of SISSI **************/

#define ORDINATEUR_LINEWIDTH  0.1
#define ORDINATEUR_FONTHEIGHT 0.8
#define LEFT_SPACE 0.7
#define RIGHT_SPACE 0.3
#define DEFAULT_FONT 0.7
/************** End of "define" daclaration of SISSI **************/

typedef struct _ObjetSISSI ObjetSISSI;
typedef struct _SISSIDialog SISSIDialog;
typedef struct _SISSIState SISSIState;
typedef struct _SISSIChange SISSIChange;


struct _SISSIState {
  /* others: */
	GList *properties_others;
};

struct _SISSIChange {
  ObjectChange obj_change;
  
  ObjetSISSI *obj;
  SISSIState *saved_state;
  int applied;
};


typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;


struct _ObjetSISSI {
  Element element;
  ConnectionPoint connections[9];
  /*/PolyShape poly;	*/	/* always 1st! */
  AnchorShape 		celltype;
  real 			radius;			/* pseudo-radius */
  int 			subscribers;		/* number of subscribers in this cell,*/
  gboolean 		show_background;
  Color 		fill_colour;
  Text 			*text;
  Color 		line_colour;
  LineStyle 		line_style;
  real 			dashlength;
  real 			line_width;

  DiaImage 		*image;
  gchar 		*file;	/* url of file picture */
  real 			border_width;
  Color 		border_color;
    
  gboolean 		draw_border;
  gboolean		keep_aspect;
 
  gchar 		*id_db;   /* this id is use by a external database and used only by there*/
	/*entity classification properties*/
  gchar 		*confidentiality,*integrity,*disponibility_level;
  float 		disponibility_value;
  gchar 		*name;  /* name of object */
  gchar			*entity; /* name of entity */
  gchar			*entity_type;
  gchar			*type_element;  /* type of object */
  /****** name of romm and site where is the object *******/
  gchar			*room,*site;
  
    /********* menaces *********/
  GList 		*properties_menaces;  /* lit of mencaces */
  unsigned int		nb_others_fixes;

  /*********** others properties ***************/
  GList 		*properties_others;
  int			nb_property_fixe;
  /********** documents list ****************/ 
  GList 		*url_docs; 
  /*********** dialog box **********/
  SISSIDialog 		*properties_dialog;

};


struct _SISSIDialog {
  GtkWidget 	*dialog;
  GtkWidget	*confidentiality, *integrity, *disponibility_level, *disponibility_value;
  GtkWidget	*entity;
  GtkWidget	*name;
  GtkWidget 	*table_document,*page_label_document,*vbox_document;
  GtkWidget 	*table_others,*page_label_others,*vbox_others;
  int		nb_doc;
  int		nb_properties_others;

    /*********menaces *********/
  GList 	*properties_menaces;
  /************ others properties **************/
  GList 	*properties_others;
  /**************** Documents list***************/
  GList 	*url_docs;
};

typedef struct _Url_Docs Url_Docs;
typedef struct _Url_Docs_Widget Url_Docs_Widget;

struct _Url_Docs {
  gchar *label, *url, *description;
};

struct _Url_Docs_Widget {
  GtkWidget 	*label,*url,*description;
};

typedef struct _SISSI_Property SISSI_Property;
typedef struct _SISSI_Property_Widget SISSI_Property_Widget;

struct _SISSI_Property {
  gchar *label;
  gchar *value;
  gchar *description;
};

struct _SISSI_Property_Widget {
  GtkWidget 	*label,*description;
  GtkWidget	*value;
};


extern SISSI_Property *create_new_property_other(gchar *label, gchar *description, gchar *value);
extern Url_Docs *create_new_url(gchar *label, gchar *description, gchar *value);
void property_url_write(AttributeNode attr_node, Url_Docs *url_docs);

extern real object_sissi_distance_from(ObjetSISSI *object_sissi, Point *point);
extern void object_sissi_select(ObjetSISSI *object_sissi, Point *clicked_point, DiaRenderer *interactive_renderer);
extern ObjectChange* object_sissi_move_handle(ObjetSISSI *object_sissi, Handle *handle, Point *to, ConnectionPoint *cp,
			    HandleMoveReason reason, ModifierKeys modifiers);
extern ObjectChange* object_sissi_move(ObjetSISSI *object_sissi, Point *to);
extern void object_sissi_draw(ObjetSISSI *object_sissi, DiaRenderer *renderer);
extern void object_sissi_update_data(ObjetSISSI *pc, AnchorShape horix, AnchorShape vert);

extern void object_sissi_destroy(ObjetSISSI *object_sissi);
void dialog_sissi_destroy(SISSIDialog *properties_dialog);
extern ObjetSISSI *object_sissi_load(ObjectNode obj_node, int version, const char *filename, ObjetSISSI *object_sissi,Element *elem,DiaObject *obj);

extern GtkWidget *object_sissi_get_properties(ObjetSISSI *object_sissi, gboolean is_default);
extern ObjectChange *object_sissi_apply_props_from_dialog(ObjetSISSI *object_sissi, GtkWidget *widget);

extern DiaObject *object_sissi_copy_using_properties(ObjetSISSI *object_sissi_origine);

extern void object_sissi_save(ObjetSISSI *object_sissi, ObjectNode obj_node, const char *filename);
extern void property_menace_write(AttributeNode attr_node, SISSI_Property_Menace *properties_menaces);
SISSI_Property_Menace *property_menace_read(DataNode composite);
extern void property_other_write(AttributeNode attr_node, SISSI_Property *properties_autre);
SISSI_Property *property_other_read(DataNode composite);
extern void url_doc_write(AttributeNode attr_node, Url_Docs *url_docs);
extern Url_Docs *url_doc_read(DataNode composite);
extern GList *clear_list_property (GList *list);
extern GList *clear_list_url_doc (GList *list);
extern void gestion_specificite(ObjetSISSI *object_sissi,int user_data);
extern xmlNodePtr find_node_named (xmlNodePtr p, const char *name);

gchar *sissi_get_sheets_directory(const gchar* subdir);
xmlDocPtr sissi_read_object_from_xml(int data);

ObjectChange *object_sissi_apply_properties_dialog(ObjetSISSI *object_sissi);
GtkWidget *object_sissi_get_properties_dialog(ObjetSISSI *object_sissi, gboolean is_default);


#endif /* SISSI_H */
