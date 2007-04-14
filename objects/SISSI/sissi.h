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

static struct _ImageProperties {
  gchar *file;
  gboolean draw_border;
  gboolean keep_aspect;
} default_properties = { "", FALSE, TRUE };


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

  DiaImage 		image;
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
extern void object_sissi_apply_properties(ObjetSISSI *object_sissi);

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

/************* creation of variable repeat for each object based on EBIOS method */
static SISSI_Property property_classification_data[] = {
  { N_("No Protection"), "NO_PROTECTION" ,NULL},
  { N_("Restricted Diffusion"), "RESTRICTED_DIFFUSION" ,NULL},
  { N_("Special Country Confidential"), "SPECIAL_COUNTRY_CONFIDENTIAL" ,NULL},
  { N_("NATO Confidential"), "NATO_CONFIDENTIAL" ,NULL},
  { N_("Personal Confidential"), "PERSONAL_CONFIDENTIAL" ,NULL},
  { N_("Medical Confidential"), "MEDICAL_CONFIDENTIAL" ,NULL},
  { N_("Industrie Confidential"), "INDUSTRIE_CONFIDENTIAL" ,NULL},
  { N_("Secret"), "SECRET" ,NULL},
  { N_("Secret special country"), "SECRET_SPECIAL_COUNTRY" ,NULL},
  { N_("NATO Secret"), "NATO_SECRET" ,NULL},
  { N_("Very Secret"), "VERY_SECRET" ,NULL},
  { N_("NATO Very Secret"), "NATO_VERY_SECRET" ,NULL},
  { NULL,0,NULL}
};

static SISSI_Property property_integrity_data[] = {
  { N_("No integrity"), "NULL" ,NULL},
  { N_("Low integrity"), "LOW_INTEGRITY" ,NULL},
  { N_("Average software integrity"), "AVERAGE_SOFTWARE_INTEGRITY" ,NULL},
  { N_("Hight software integrity"), "HIGHT_SOFTWARE_INTEGRITY" ,NULL},
  { N_("Average hardware integrity"), "AVERAGE_HARDWARE_INTEGRITY" ,NULL},
  { N_("Hight hardware integrity"), "HIGHT_HARDWARE_INTEGRITY" ,NULL},
  { NULL,0,NULL}
};

static SISSI_Property property_disponibility_level_data[] = {
  { N_("Millisecond"), "MILLISECOND" ,NULL},
  { N_("Second"), "SECOND" ,NULL},
  { N_("Minute"), "MINUTE" ,NULL},
  { N_("Hour"), "HOUR" ,NULL},
  { N_("Day"), "DAY" ,NULL},
  { N_("Week"), "WEEK" ,NULL},
  { NULL,0,NULL}
};

static SISSI_Property property_system_data[] = {
  { N_("SYSTEM"), "SYS",NULL},
  { N_("Internet access device"), "SYS_INT",NULL},
  { N_("Electronic messaging"), "SYS_MAIL",NULL},
  { N_("Intranet"), "SYS_ITR",NULL},
  { N_("Company directory"), "SYS_DIRECTORY",NULL},
  { N_("External portal"), "SYS_WEB",NULL},
  { NULL,0,NULL}
};

static SISSI_Property property_organisation_data[] = {
  { N_("ORGANISATION"), "ORG",NULL},
     { N_("Higher-tier organisation"), "ORG_DEP",NULL},
     { N_("Structure of the organisation"), "ORG_GEN",NULL},
     { N_("Project or system organisation"), "ORG_PRO",NULL},
     { N_("Subcontractors / Suppliers / Manufacturers"),"ORG_EXT",NULL},
  { NULL,0,NULL}
};

static SISSI_Property property_physic_data[] = {
  { N_("SITE"), "PHY",NULL},
   { N_("Places"), "PHY_LIE",NULL},
     { N_("External environment"), "PHY_LIE1",NULL},
     { N_("Premises"), "PHY_LIE2",NULL},
     { N_("Zone"), "PHY_LIE3",NULL},
   { N_("Service Essentiel"), "PHY_SRV",NULL},
     { N_("Communication"), "PHY_SRV1",NULL},
     { N_("Power"), "PHY_SRV2",NULL},
     { N_("Cooling / Pollution"), "PHY_SRV3",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_detecTHERMIC_data[] = {
  { N_("THERMIC DETECTION"), "PHY_DETECTION_THERMIC",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_detecFIRE_data[] = {
  { N_("FIRE DETECTION"), "PHY_DETECTION_FIRE",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_detecWATER_data[] = {
  { N_("WATER DETECTION"), "PHY_DETECTION_WATER",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_detecAIR_data[] = {
  { N_("AIR DETECTION"), "PHY_DETECTION_AIR",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_detecENERGY_data[] = {
  { N_("ENERGY DETECTION"), "PHY_DETECTION_ENERGY",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_detecINTRUSION_data[] = {
  { N_("INTRUSION DETECTION"), "PHY__DETECTION_INTRUSION",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_actionTHERMIC_data[] = {
  { N_("THERMIC ACTION"), "PHY_ACTION_THERMIC",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_actionFIRE_data[] = {
  { N_("FIRE ACTION"), "PHY_ACTION_FIRE",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_actionWATER_data[] = {
 { N_("WATER ACTION"), "PHY_ACTION_WATER",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_actionAIR_data[] = {
  { N_("AIR ACTION"), "PHY_ACTION_AIR",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_actionENERGY_data[] = {
  { N_("ENERGY ACTION"), "PHY_ACTION_ENERGY",NULL},
  { NULL, 0,NULL}
};
static SISSI_Property property_physic_actionINTRUSION_data[] = {
  { N_("INTRUSION ACTION"), "PHY_ACTION_INTRUSION",NULL},
  { NULL, 0,NULL}
};

static SISSI_Property property_personnel_data[] = {
  { N_("PERSONAL"), "PER",NULL},
   { N_("Decision maker"), "PER_DEC",NULL},
   { N_("SSI Responsible"), "PER_RESP_SSI",NULL},
   { N_("Users"), "PER_UTI",NULL},
   { N_("Fonctionnal administrator"), "PER_FONC_ADMIN",NULL},
   { N_("Technician aministrator"), "PER_TECH_ADMIN",NULL},
   { N_("SSI administrator"), "PER_SSI_ADMIN",NULL},
   { N_("Developer"), "PER_DEV",NULL},
   { N_("Operator / Maintenance"), "PER_EXP",NULL},
  { NULL, 0,NULL}
};

static SISSI_Property property_reseau_data[] = {
  { N_("NETWORK"), "RES",NULL},
   { N_("Medium and supports"), "RES_INF",NULL},
   { N_("Passive or active relay"), "RES_REL",NULL},
   { N_("Communication interface"), "RES_INT",NULL},
  { NULL, 0,NULL}
};

static SISSI_Property property_logiciel_data[] = {
  { N_("LOGICIEL"), "LOG",NULL},
    { N_("Operating System"), "LOG_OS",NULL},
    { N_("Service - maintenance or administration software"), "LOG_SRV",NULL},
    { N_("Package software or standard software"), "LOG_STD",NULL},
    { N_("Business application"), "LOG_APP",NULL},
      { N_("Standard business application"), "LOG_APP1",NULL},
      { N_("Specific business application"), "LOG_APP2",NULL},
  { NULL, 0,NULL}
};

static SISSI_Property property_material_data[] = {
  { N_("HARDWARE"), "MAT",NULL},
  { N_("Data processing equipement (active)"), "MAT_ACT",NULL},
  { N_("Transportable equipement"), "MAT_ACT1",NULL},
  { N_("Fixed equipement"), "MAT_ACT2",NULL},
  { N_("Peripheral processing"), "MAT_ACT3",NULL},
  { N_("Electronic medium"), "MAT_PAS1",NULL},
  { N_("Other media"), "MAT_PAS2",NULL},  
  { N_("Data medium (passive)"), "MAT_PAS",NULL},
  { NULL, 0,NULL}
};


/*///////////////d�inition of color types  ////////////////*/
static Color color_red = { 1.0f, 0.0f, 0.0f };
static Color color_gris = { 0.5f, 0.5f, 0.5f };
static Color color_gris_clear = { 0.95f, 0.95f, 0.95f };
static Color color_green = { 0.0f, 1.0f, 0.0f };
static Color color_green_clear = { 0.87, 0.98f, 0.91 };

#endif /* SISSI_H */
