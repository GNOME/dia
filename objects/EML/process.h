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
#ifndef PROCESS_H
#define PROCESS_H

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "intl.h"
/*#include "lazyprops.h"*/

#include "dbox.h"
#include "eml.h"
#include "listfun.h"

#define EMLPROCESS_STATE_COPY(from, to) \
  (to)->name       = g_strdup((from)->name); \
  (to)->refname       = g_strdup((from)->refname); \
  (to)->proclife   = g_strdup((from)->proclife); \
  (to)->startupfun = eml_function_copy((from)->startupfun); \
  (to)->interfaces = list_map((from)->interfaces, \
                              (MapFun) eml_interface_copy);

#define EMLPROCESS_FREE_STATE(state) \
  g_free((state)->name); \
  g_free((state)->refname); \
  g_free((state)->proclife); \
  if ((state)->startupfun != NULL) \
    eml_function_destroy((state)->startupfun); \
 \
  g_list_foreach((state)->interfaces, \
                 list_foreach_fun, eml_interface_destroy);

#define EMLPROCESS_STCONN 10

typedef struct _EMLProcessState EMLProcessState;
typedef struct _EMLProcess EMLProcess;
typedef struct _EMLProcessDialogState EMLProcessDialogState;
typedef struct _EMLProcessDialog EMLProcessDialog;

struct _EMLProcessState {
  ObjectState obj_state;

  gchar *name;
  gchar *refname;
  gchar *proclife;
  EMLFunction *startupfun;
  GList *interfaces;
};

struct _EMLProcessDialogState {

  gchar *name;
  gchar *refname;
  gchar *proclife;
  EMLFunction *startupfun;
  GList *interfaces;

  GtkWidget *process_name;
  GtkWidget *process_refname;
  GtkWidget *process_proclife;

  GtkWidget *startupfun_module;
  GtkWidget *startupfun_name;
  
};

struct _EMLProcess {
  Element element;
  ConnectionPoint proc_connections[EMLPROCESS_STCONN];

  gchar *name;
  gchar *refname;
  EMLFunction *startupfun;
  gchar *proclife;
  GList *interfaces;

  EMLBox *box;
  EMLBox *name_box;
  GList *box_connections;
  
  real name_font_height;
  real startupfun_font_height;
  real startupparams_height;
  real proclife_font_height;
  real interface_font_height;

  gchar *name_font;
  gchar *startupfun_font;
  gchar *startupparams_font;
  gchar *proclife_font;
  gchar *interface_font;

  /* Dialog: */
  EMLProcessDialog *properties_dialog;
  EMLProcessDialogState dialog_state;
};

typedef struct _EMLProcessPage EMLProcessPage;
typedef struct _EMLStartupfunPage EMLStartupfunPage;
typedef struct _EMLStartupfun EMLStartupfun;
typedef struct _EMLInterfacePage EMLInterfacePage;

/***************************************************
 The following was pulled out of lazyprops.h:
 */

/* Attribute editing dialog ("Properties.*" stuff) */
/* Goal : no, or almost no gtk garb^Wstuff in an object's code. Or at worst, 
   really nicely hidden. */

/* contract : 
    - object type Foo has a single object properties dialog (FooProperties), 
    which will be pointed to by the first parameter "PropertiesDialog" 
    (or "propdlg", in short) of all the following macros.
    - "propdlg" has a single AttributeDialog field, called "dialog", which
    in fact is a gtk VBox.
    - "propdlg" has a single Foo * pointer, called "parent", which traces back
    to the Foo object structure being edited, or NULL.

    - All of (Foo *)'s fields which are to be edited by the property dialog
    has a matching "propdlg" field of a related type.
    - Description strings are to be passed with _( ) around it (i18n). 

    - all type-specific macros take at least the following arguments : 
       * propdlg   the name of the propdlg variable
       * field     the name of the field being changed (in both propdlg and
                                                        propdlg->parent)
       * desc      an i18n-able description string. 

   For the text font, font height and color attributes, the fields in the 
   dialog structure must respectively be called foo_font, foo_fontheight and
   foo_color (where "foo" is the name of the text field in the parent object).

*/

#define PROPDLG_TYPE GtkWidget *
typedef struct {
  GtkWidget *d;
  GtkWidget *label;
  gboolean ready;
} AttributeDialog; /* This is intended to be opaque */

/* to be called to create a new, empty dialog. */
#define PROPDLG_CREATE(propdlg,parentvalue) \
  if (!propdlg) { \
     (propdlg) = g_malloc0(sizeof(*(propdlg)) ); \
     (propdlg)->dialog.d = gtk_vbox_new(FALSE,5);  \
     gtk_object_ref(GTK_OBJECT((propdlg)->dialog.d)); \
     gtk_object_sink(GTK_OBJECT((propdlg)->dialog.d)); \
     gtk_container_set_border_width(GTK_CONTAINER((propdlg)->dialog.d), 10); \
     /* (propdlg)->dialog.ready = FALSE; */ \
  } \
  (propdlg)->parent = (parentvalue);

/* to be called when the dialog is almost ready to show (bracket the rest with
   PROPDLG_CREATE and PROPDLG_READY) */
#define PROPDLG_READY(propdlg) \
  if (!(propdlg)->dialog.ready) { \
     gtk_widget_show((propdlg)->dialog.d); \
     (propdlg)->dialog.ready = TRUE; \
  }

/* The return type of get_foo_properties() */
#define PROPDLG_RETURN(propdlg) \
    return propdlg->dialog.d;
/* This defines a notebook inside a dialog. */
typedef GtkWidget *AttributeNotebook;
#define PROPDLG_NOTEBOOK_CREATE(propdlg,field) \
   if (!(propdlg)->dialog.ready) {\
      (propdlg)->field = gtk_notebook_new (); \
      gtk_notebook_set_tab_pos (GTK_NOTEBOOK((propdlg)->field), GTK_POS_TOP); \
      gtk_box_pack_start (GTK_BOX ((propdlg)->dialog.d), \
                         (propdlg)->field, TRUE, TRUE, 0); \
      gtk_container_set_border_width (GTK_CONTAINER((propdlg)->field), 10); \
   }
#define PROPDLG_NOTEBOOK_READY(propdlg,field) \
   if (!(propdlg)->dialog.ready) {\
      gtk_widget_show((propdlg)->field); \
   }

/* This defines a notebook page.
   The "field" must be a pointer to an ad-hoc struct similar to the normal
   property dialog structs. */
typedef AttributeDialog AttributePage;
#define PROPDLG_PAGE_CREATE(propdlg,notebook,field,desc) \
   if (!(propdlg)->dialog.ready) { \
     (propdlg)->field = g_malloc0(sizeof(*((propdlg)->field))); \
     (propdlg)->field->dialog.d = gtk_vbox_new(FALSE,5);  \
     /*(propdlg)->field->dialog.ready = FALSE; */ \
     gtk_container_set_border_width(GTK_CONTAINER((propdlg)->field->dialog.d),\
                                   10); \
     (propdlg)->field->dialog.label = gtk_label_new((desc));  \
  } \
  (propdlg)->field->parent = (propdlg)->parent;

#define PROPDLG_PAGE_READY(propdlg,notebook,field) \
  if (!(propdlg)->field->dialog.ready) { \
     gtk_widget_show_all((propdlg)->field->dialog.d); \
     gtk_widget_show_all((propdlg)->field->dialog.label); \
     gtk_widget_show((propdlg)->field->dialog.d); \
     gtk_notebook_append_page(GTK_NOTEBOOK((propdlg)->notebook), \
                    (propdlg)->field->dialog.d, \
                    (propdlg)->field->dialog.label); \
     (propdlg)->field->dialog.ready = TRUE; \
  }
/*
 end of stuff pulled out of lazyprops.h
 ********************************** */

struct _EMLProcessPage {
  AttributePage dialog;
  EMLProcessDialogState *parent;
};

struct _EMLStartupfunPage {
  AttributePage dialog;
  EMLProcessDialogState *parent;

  GNode *parameters;
};

struct _EMLInterfacePage {
  AttributePage dialog;
  EMLProcessDialogState *parent;

  GNode *interfaces;
};

struct _EMLProcessDialog {
  AttributeDialog dialog;
  EMLProcessDialogState *parent;

  AttributeNotebook nb;
  EMLProcessPage *process_pg;
  EMLStartupfunPage *startupfun_pg;
  EMLInterfacePage *interface_pg;

  GList *disconnected_connections;
  GList *added_connections; 
  GList *deleted_connections;
};

extern void emlprocess_create_box(EMLProcess *);
extern void emlprocess_destroy_box(EMLProcess *);
extern void emlprocess_calculate_connections(EMLProcess *);
extern void emlprocess_calculate_data(EMLProcess *);
extern void emlprocess_update_data(EMLProcess *);
extern PROPDLG_TYPE emlprocess_get_properties(EMLProcess *);
extern ObjectChange *emlprocess_apply_properties(EMLProcess *);
extern void emlprocess_dialog_destroy(EMLProcess *);



#endif /* PROCESS_H */
