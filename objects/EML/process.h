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
#include "lazyprops.h"

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
