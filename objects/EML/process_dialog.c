#include "nlist.h"
#include "process.h"

#include "config.h"
#include "object.h"
#include "objchange.h"
#include "intl.h"

typedef enum _EMLParametersShow {
  SHOW_ALL,
  SHOW_RELATIONS,
  SHOW_VARS
} EMLParametersShow;

typedef struct _Disconnect Disconnect;
typedef struct _EMLProcessChange EMLProcessChange;

struct _Disconnect {
  ConnectionPoint *cp;
  DiaObject *other_object;
  Handle *other_handle;
};

struct _EMLProcessChange {
  ObjectChange obj_change;
  
  EMLProcess *obj;

  GList *added_cp;
  GList *deleted_cp;
  GList *disconnected;

  int applied;
  
  EMLProcessState *saved_state;
};

static ObjectChange * new_emlprocess_change(EMLProcess *, EMLProcessState *,
                                            GList *, GList *, GList *);
static void emlprocess_fill_from_state(EMLProcess *, EMLProcessState *);

static EMLProcessState * emlprocess_get_state(EMLProcess *);

static gchar** nlc_relmember_to_raw(const GNode *, const gpointer);
static void nlc_set_relmembers_data(GNode *, gpointer);
static gpointer nlc_relmember_new(GNode *);
static void nlc_relmember_destroy(GNode *, const gpointer);
static gpointer nlc_relmember_create_temp(GNode *);
static void nlc_relmember_destroy_temp(GNode *, const gpointer);
static void nlc_relmember_after_select(GNode *);
static void nlc_relmember_after_update(GNode *);

static gchar** nlc_parameter_to_raw(const GNode *, const gpointer);
static void nlc_set_parameters_data(GNode *, gpointer);
static gpointer nlc_parameter_new(GNode *);
static void nlc_parameter_destroy(GNode *, const gpointer);
static gpointer nlc_parameters_create_temp(GNode *);
static void nlc_parameters_destroy_temp(GNode *, const gpointer);
static void nlc_parameter_after_select(GNode *);
static void nlc_parameter_after_update(GNode *);

static void nlc_set_ifmessages_data(GNode *, gpointer);
static gpointer nlc_ifmessage_new(GNode *);

static gchar** nlc_iffunction_to_raw(const GNode *, const gpointer);
static void nlc_set_iffunctions_data(GNode *, gpointer);
static gpointer nlc_iffunction_new(GNode *);
static void nlc_iffunction_destroy(GNode *, const gpointer);
static gpointer nlc_iffunctions_create_temp(GNode *);
static void nlc_iffunctions_destroy_temp(GNode *, const gpointer);
static void nlc_iffunction_after_select(GNode *);
static void nlc_iffunction_after_update(GNode *);

static gchar** nlc_interface_to_raw(const GNode *, const gpointer);
static void nlc_set_interfaces_data(GNode *, gpointer);
static gpointer nlc_interface_new(GNode *);
static void nlc_interface_destroy(GNode *, const gpointer);
static gpointer nlc_interfaces_create_temp(GNode *);
static void nlc_interfaces_destroy_temp(GNode *, const gpointer);
static void nlc_interface_after_select(GNode *);
static void nlc_interface_after_update(GNode *);

NListController EMLInterfaceNLC = {
  &nlc_set_interfaces_data,
  &nlc_interface_new,
  &nlc_interface_destroy,
  &nlc_interface_to_raw,
  &nlc_interface_after_select,
  &nlc_interface_after_update,
  &nlc_interfaces_create_temp,
  &nlc_interfaces_destroy_temp

};

NListController EMLIfmessageNLC = {
  &nlc_set_ifmessages_data,
  &nlc_ifmessage_new,
  &nlc_parameter_destroy,
  &nlc_parameter_to_raw,
  &nlc_parameter_after_select,
  &nlc_parameter_after_update,
  &nlc_parameters_create_temp,
  &nlc_parameters_destroy_temp

};

NListController EMLParamNLC = {
  &nlc_set_parameters_data,
  &nlc_parameter_new,
  &nlc_parameter_destroy,
  &nlc_parameter_to_raw,
  &nlc_parameter_after_select,
  &nlc_parameter_after_update,
  &nlc_parameters_create_temp,
  &nlc_parameters_destroy_temp

};

NListController EMLIffunctionNLC = {
  &nlc_set_iffunctions_data,
  &nlc_iffunction_new,
  &nlc_iffunction_destroy,
  &nlc_iffunction_to_raw,
  &nlc_iffunction_after_select,
  &nlc_iffunction_after_update,
  &nlc_iffunctions_create_temp,
  &nlc_iffunctions_destroy_temp

};

NListController  EMLRelmemberNLC = {
  &nlc_set_relmembers_data,
  &nlc_relmember_new,
  &nlc_relmember_destroy,
  &nlc_relmember_to_raw,
  &nlc_relmember_after_select,
  &nlc_relmember_after_update,
  &nlc_relmember_create_temp,
  &nlc_relmember_destroy_temp

};

/**** Utility functions ******/
static void
emlprocess_store_disconnects(EMLProcessDialog *prop_dialog,
			   ConnectionPoint *cp)
{
  Disconnect *dis;
  DiaObject *connected_obj;
  GList *list;
  int i;
  
  list = cp->connected;
  while (list != NULL) {
    connected_obj = (DiaObject *)list->data;
    
    for (i=0;i<connected_obj->num_handles;i++) {
      if (connected_obj->handles[i]->connected_to == cp) {
	dis = g_new(Disconnect, 1);
	dis->cp = cp;
	dis->other_object = connected_obj;
	dis->other_handle = connected_obj->handles[i];

	prop_dialog->disconnected_connections =
	  g_list_prepend(prop_dialog->disconnected_connections, dis);
      }
    }
    list = g_list_next(list);
  }
}

static
GtkWidget* create_dialog_entry(gchar *entry_name, GtkWidget **entry)
{
  GtkWidget *hbox;
  GtkWidget *label;

  hbox = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(entry_name);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_widget_show(label);
  *entry = gtk_entry_new();
  gtk_box_pack_start (GTK_BOX (hbox), *entry, TRUE, TRUE, 0);
  gtk_widget_show(*entry);

  return hbox;
}

/********************************************************
 ******************** PROCESS ***************************
 ********************************************************/

/********************************************************
 ******************** RELATION MEMBERS ******************
 ********************************************************/

static
gpointer
nlc_relmember_create_temp(GNode *node)
{
  return g_strdup("");
}

static
void
nlc_relmember_destroy_temp(GNode *node, const gpointer data)
{
  g_free(data);
}

static
void
nlc_set_relmembers_data(GNode *node_list, gpointer data)
{
  EMLParameter *param;
  NNode *node;

  node = (NNode*) node_list->data;
  param =  (EMLParameter*) data;
  node->data = &param->relmembers;
}

static
gpointer
nlc_relmember_new(GNode *node_list)
{
  return g_strdup("");
}

static
void
nlc_relmember_destroy(GNode* node, const gpointer data)
{
  g_free(data);
}

static
gchar**
nlc_relmember_to_raw(const GNode *node, const gpointer data)
{
  gchar* *str;
  
  str = g_new0(gchar*, 2);
  str[0] = g_strdup((gchar*) data);

  return str;
}

static
void
nlc_relmember_after_select(GNode *node_list)
{
  gchar *relmember;
  NNode *node;

  node = (NNode*) node_list->data;
  relmember = (gchar*) node->current_el;
  
  g_free(node->edit_el);
  node->edit_el = g_strdup(relmember);
}

static void nlc_relmember_after_update(GNode *node_list)
{
  NNode *node;

  node = (NNode*) node_list->data;
  g_free(node->current_el);

  node->current_el = g_strdup(node->edit_el);
}

static
void
relmember_set_value(GtkWidget *entry, GdkEventFocus *ev, GNode *node_list)
{
  NNode *node;

  node = (NNode*) node_list->data;

  g_free(node->edit_el);
  node->edit_el = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void relmember_show_details(GNode *node_list, GtkWidget *entry)
{
  NNode *node;

  node = (NNode*) node_list->data;

  gtk_entry_set_text(GTK_ENTRY(entry), node->edit_el);
}

static
GNode*
relmembers_dialog_create(EMLProcess *emlprocess,
                         gchar *list_title,
                         gchar *data_title,
                         GtkWidget **widget)
{
  GNode *node_list;
  NNode *node;
  GtkWidget *entry;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *frame;

  node_list = nlist_node_new(1,
                             list_title,
                             data_title,
                             NULL,
                             NULL,
                             &EMLRelmemberNLC,
                             emlprocess);

  node = (NNode*) node_list->data;
  vbox = nlist_create_box(node_list);

  frame = gtk_frame_new(node->data_title);
  vbox2 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  hbox2 = gtk_hbox_new(FALSE, 5);
  entry = gtk_entry_new();

  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_widget_show(entry);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
  gtk_widget_show(hbox2);

  gtk_widget_show(vbox2);

  nlist_add_entry_widget(node_list,
                         GTK_WIDGET(entry),
                         relmember_show_details,
                         relmember_set_value);

  *widget = vbox;

  return node_list;
}

/********************************************************
 ****************** PARAMETERS AND MESSAGES *************
 ********************************************************/
static
void
nlc_set_ifmessages_data(GNode *node_list, gpointer data)
{
  EMLInterface *iface;
  NNode *node;

  node = (NNode*) node_list->data;
  iface =  (EMLInterface*) data;
  node->data = &iface->messages;
}

static
gpointer
nlc_ifmessage_new(GNode *node_list)
{
  EMLParameter *param;

  param = nlc_parameter_new(node_list);
  param->type = EML_RELATION;

  return param;
}

static
gpointer
nlc_parameters_create_temp(GNode *node)
{
  return eml_parameter_new();
}

static
void
nlc_parameters_destroy_temp(GNode *node, const gpointer data)
{
  eml_parameter_destroy((EMLParameter*) data);
}

static
void
nlc_set_parameters_data(GNode *node_list, gpointer data)
{
  EMLFunction *fun;
  NNode *node;

  node = (NNode*) node_list->data;
  fun =  (EMLFunction*) data;
  node->data = &fun->parameters;
}

static
gpointer
nlc_parameter_new(GNode *node_list)
{
  EMLProcess *emlprocess;
  DiaObject *obj;
  EMLProcessDialog *dlg;
  EMLParameter *param;
  NNode *node;

  node = (NNode*) node_list->data;
  emlprocess = (EMLProcess*) node->user_data;
  obj = (DiaObject *) emlprocess;

  dlg  = emlprocess->properties_dialog;
  param = eml_parameter_new();

  emlconnected_new((EMLConnected*) param);
  param->left_connection->object = obj;
  param->right_connection->object = obj;

  dlg->added_connections = g_list_append(dlg->added_connections,
                                           param->left_connection);
  dlg->added_connections = g_list_append(dlg->added_connections,
                                           param->right_connection);
  return param;

}

static
void
nlc_parameter_destroy(GNode* node_list, const gpointer data)
{
  EMLProcessDialog *dlg;
  EMLParameter *param;
  NNode *node;

  node = (NNode*) node_list->data;
  dlg = ((EMLProcess*) node->user_data)->properties_dialog;
  param = (EMLParameter*) node->current_el;

    dlg->deleted_connections = g_list_append(dlg->deleted_connections,
                                             param->left_connection);
    dlg->deleted_connections = g_list_append(dlg->deleted_connections,
                                             param->right_connection);
  eml_parameter_destroy(param);
}

static
gchar**
nlc_parameter_to_raw(const GNode *node_list, const gpointer data)
{
  EMLParameter *param;
  gchar* *str;

  str = g_new0(gchar*, 2);
  param = (EMLParameter*) data;
  str[0] = eml_get_parameter_string(param);

  return str;
}

static
void
nlc_parameter_after_select(GNode *node_list)
{
  EMLParameter *param, *edit_param;
  NNode *node;

  node = (NNode*) node_list->data;
  param = (EMLParameter*) node->current_el;
  edit_param = (EMLParameter*) node->edit_el;
  
  edit_param->type = param->type;
  g_free(edit_param->name);
  edit_param->name = g_strdup(param->name);
}

static void nlc_parameter_after_update(GNode *node_list)
{
  EMLProcess *emlprocess;
  EMLParameter *param, *edit_param;
  EMLProcessDialog *dlg;
  NNode *node;

  node = (NNode*) node_list->data;
  emlprocess = (EMLProcess*) node->user_data;
  dlg = emlprocess->properties_dialog;
  param = (EMLParameter*) node->current_el;
  edit_param = (EMLParameter*) node->edit_el;

  g_assert(param != NULL);

  if (param->type == EML_RELATION && param->type != edit_param->type) {

    emlprocess_store_disconnects(dlg, param->left_connection);
    object_remove_connections_to(param->left_connection);
    emlprocess_store_disconnects(dlg, param->right_connection);
    object_remove_connections_to(param->right_connection);

  }

  g_free(param->name);
  param->name = g_strdup(edit_param->name);
  param->type = edit_param->type;

}

static
void
parameter_set_name(GtkWidget *entry, GdkEventFocus *ev, GNode *node_list)
{
  NNode *node;
  EMLParameter *param;

  node = (NNode*) node_list->data;
  param = (EMLParameter *) node->edit_el;
  
  if (param == NULL)
    return;

  g_free(param->name);
  param->name =  g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void parameter_show_name(GNode *node_list, GtkWidget *entry)
{
  NNode *node;
  EMLParameter *param;

  node = (NNode*) node_list->data;
  param = (EMLParameter *) node->edit_el;
  
  if (param == NULL)
    return;

  gtk_entry_set_text(GTK_ENTRY(entry), param->name);
}

static
void
parameter_set_type(GtkWidget *button, GdkEventFocus *ev, GNode *node_list)
{
  NNode *node;
  EMLParameter *param;

  node = (NNode*) node_list->data;
  param = (EMLParameter *) node->edit_el;
  
  if (param == NULL)
    return;

  param->type = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(button));

}

static void parameter_show_type(GNode *node_list, GtkWidget *button)
{
  NNode *node;
  EMLParameter *param;

  node = (NNode*) node_list->data;
  param = (EMLParameter *) node->edit_el;
  
  if (param == NULL)
    return;

  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(button), param->type);
}

static
GNode*
parameters_dialog_create(EMLProcess *emlprocess,
                         gchar *list_title,
                         gchar *data_title,
                         gchar *rel_title,
                         gchar *reldata_title,
                         gboolean detail_type,
                         NListController *controller,
                         GtkWidget **widget)
{
  GNode *node_list;
  NNode *node;
  GtkWidget *entry;
  GtkWidget *param_widget;
  GtkWidget *hbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *detail_frame;
  GtkWidget *detail_vbox;
  GtkWidget *cbutton;
  GNode *rel_list;
  GtkWidget *rel_widget;

  node_list = nlist_node_new(1,
                             list_title,
                             data_title,
                             NULL,
                             NULL,
                             controller,
                             emlprocess);

  node = (NNode*) node_list->data;
  param_widget = nlist_create_box(node_list);

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE);

  vbox2 = gtk_vbox_new(FALSE, 5);
  gtk_box_pack_start (GTK_BOX (vbox2), param_widget, FALSE, TRUE, 0);
  gtk_widget_show(param_widget);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
  gtk_widget_show(vbox2);

  detail_frame = gtk_frame_new(node->data_title);
  detail_vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (detail_vbox), 5);
  gtk_container_add (GTK_CONTAINER (detail_frame), detail_vbox);
  gtk_box_pack_start (GTK_BOX (hbox), detail_frame, FALSE, TRUE, 0);
  gtk_widget_show(detail_frame);

  if (detail_type == SHOW_ALL) {

    hbox2 = gtk_hbox_new(FALSE, 5);

    cbutton = gtk_check_button_new_with_label(_("relation"));
    gtk_box_pack_start (GTK_BOX (hbox2), cbutton, TRUE, TRUE, 0);
    gtk_widget_show(cbutton);
    gtk_box_pack_start (GTK_BOX (detail_vbox), hbox2, FALSE, TRUE, 0);
    gtk_widget_show(hbox2);

    nlist_add_entry_widget(node_list,
                           GTK_WIDGET(cbutton),
                           parameter_show_type,
                           parameter_set_type);
  }

  hbox2 = create_dialog_entry(_("Name:"), &entry);
  gtk_box_pack_start (GTK_BOX (detail_vbox), hbox2, FALSE, TRUE, 0);
  gtk_widget_show(hbox2);
  
  nlist_add_entry_widget(node_list,
                         GTK_WIDGET(entry),
                         parameter_show_name,
                         parameter_set_name);

  if (detail_type == SHOW_ALL || detail_type == SHOW_RELATIONS) {

    rel_list = relmembers_dialog_create(emlprocess,
                                        _(rel_title),
                                        _(reldata_title),
                                        &rel_widget);

    nlist_add_child(node_list, rel_list, rel_widget);

    vbox2 = gtk_vbox_new(FALSE, 5);
    gtk_box_pack_start (GTK_BOX (vbox2), rel_widget, FALSE, TRUE, 0);
    gtk_widget_show(rel_widget);
    gtk_box_pack_start (GTK_BOX (detail_vbox), vbox2, FALSE, TRUE, 0);
    gtk_widget_show(vbox2);
  }

  *widget = hbox;

  return node_list;
}

/********************************************************
 ********************** INTERFACES **********************
 ********************************************************/
static
void
nlc_set_iffunctions_data(GNode *node_list, gpointer data)
{
  EMLInterface *iface;
  NNode *node;

  node = (NNode*) node_list->data;
  iface =  (EMLInterface*) data;
  node->data = &iface->functions;
}

static
gpointer
nlc_iffunctions_create_temp(GNode *node)
{
  EMLFunction *fun;

  fun = eml_function_new();
  fun->name = g_strdup("");

  return fun;
}

static
void
nlc_iffunctions_destroy_temp(GNode *node, const gpointer data)
{
  if (data != NULL)
    eml_function_destroy((EMLFunction*) data);
}

static
gpointer
nlc_iffunction_new(GNode *node_list)
{
  EMLProcess *emlprocess;
  DiaObject *obj;
  EMLProcessDialog *dlg;
  EMLFunction *fun;
  NNode *node;

  node = (NNode*) node_list->data;
  emlprocess = (EMLProcess*) node->user_data;
  obj = (DiaObject *) emlprocess;

  dlg  = emlprocess->properties_dialog;
  fun = eml_function_new();

  emlconnected_new((EMLConnected*) fun);
  fun->left_connection->object = obj;
  fun->right_connection->object = obj;

  dlg->added_connections = g_list_append(dlg->added_connections,
                                           fun->left_connection);
  dlg->added_connections = g_list_append(dlg->added_connections,
                                           fun->right_connection);
  return fun;

}

static
void
nlc_iffunction_destroy(GNode* node_list, const gpointer data)
{
  EMLProcessDialog *dlg;
  EMLFunction *fun;
  NNode *node;

  node = (NNode*) node_list->data;
  dlg = ((EMLProcess*) node->user_data)->properties_dialog;
  fun = (EMLFunction*) node->current_el;

    dlg->deleted_connections = g_list_append(dlg->deleted_connections,
                                             fun->left_connection);
    dlg->deleted_connections = g_list_append(dlg->deleted_connections,
                                             fun->right_connection);
  eml_function_destroy(fun);
}

static
gchar**
nlc_iffunction_to_raw(const GNode *node_list, const gpointer data)
{
  EMLFunction *fun;
  gchar* *str;

  str = g_new0(gchar*, 2);
  fun = (EMLFunction*) data;
  str[0] = eml_get_function_string(fun);

  return str;
}

static
void
nlc_iffunction_after_select(GNode *node_list)
{
  EMLFunction *fun, *edit_fun;
  NNode *node;

  node = (NNode*) node_list->data;
  fun = (EMLFunction*) node->current_el;
  edit_fun = (EMLFunction*) node->edit_el;
  
  g_free(edit_fun->name);
  edit_fun->name = g_strdup(fun->name);

  g_free(edit_fun->module);
  edit_fun->module = g_strdup(fun->module);

}

static void nlc_iffunction_after_update(GNode *node_list)
{
  EMLProcess *emlprocess;
  EMLFunction *fun, *edit_fun;
  EMLProcessDialog *dlg;
  NNode *node;

  node = (NNode*) node_list->data;
  emlprocess = (EMLProcess*) node->user_data;
  dlg = emlprocess->properties_dialog;
  fun = (EMLFunction*) node->current_el;
  edit_fun = (EMLFunction*) node->edit_el;

  g_assert(fun != NULL);

  g_free(fun->name);
  fun->name = g_strdup(edit_fun->name);

  g_free(fun->module);
  fun->module = g_strdup(edit_fun->module);

}

static
void
iffunction_set_name(GtkWidget *entry, GdkEventFocus *ev, GNode *node_list)
{
  NNode *node;
  EMLFunction *fun;

  node = (NNode*) node_list->data;
  fun = (EMLFunction *) node->edit_el;
  
  if (fun == NULL)
    return;

  g_free(fun->name);
  fun->name =  g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void iffunction_show_name(GNode *node_list, GtkWidget *entry)
{
  NNode *node;
  EMLFunction *fun;

  node = (NNode*) node_list->data;
  fun = (EMLFunction *) node->edit_el;
  
  if (fun == NULL)
    return;

  gtk_entry_set_text(GTK_ENTRY(entry), fun->name);
}

static
void
iffunction_set_module(GtkWidget *entry, GdkEventFocus *ev, GNode *node_list)
{
  NNode *node;
  EMLFunction *fun;

  node = (NNode*) node_list->data;
  fun = (EMLFunction *) node->edit_el;
  
  if (fun == NULL)
    return;

  g_free(fun->module);
  fun->module =  g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void iffunction_show_module(GNode *node_list, GtkWidget *entry)
{
  NNode *node;
  EMLFunction *fun;

  node = (NNode*) node_list->data;
  fun = (EMLFunction *) node->edit_el;
  
  if (fun == NULL)
    return;

  gtk_entry_set_text(GTK_ENTRY(entry), fun->module);
}

static
GNode*
iffunctions_dialog_create(EMLProcess *emlprocess,
                         GtkWidget **widget)
{
  GNode *node_list;
  NNode *node;
  GtkWidget *entry;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *frame;

  node_list = nlist_node_new(1,
                             _("Interface functions"),
                             _("Function"),
                             NULL,
                             NULL,
                             &EMLIffunctionNLC,
                             emlprocess);

  node = (NNode*) node_list->data;
  vbox = nlist_create_box(node_list);

  frame = gtk_frame_new(node->data_title);
  vbox2 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  hbox2 = create_dialog_entry(_("Function module:"), &entry);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
  gtk_widget_show(hbox2);

  nlist_add_entry_widget(node_list,
                         GTK_WIDGET(entry),
                         iffunction_show_module,
                         iffunction_set_module);

  hbox2 = create_dialog_entry(_("Function name:"), &entry);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
  gtk_widget_show(hbox2);

  nlist_add_entry_widget(node_list,
                         GTK_WIDGET(entry),
                         iffunction_show_name,
                         iffunction_set_name);

  gtk_widget_show(vbox2);

  *widget = vbox;

  return node_list;
}

/********************************************************
 *********************** INTERFACES *********************
 ********************************************************/

static
gpointer
nlc_interfaces_create_temp(GNode *node)
{
  return eml_interface_new();
}

static
void
nlc_interfaces_destroy_temp(GNode *node, const gpointer data)
{
  if (data != NULL)
    eml_interface_destroy((EMLInterface*) data);
}

static
void
nlc_set_interfaces_data(GNode *node_list, gpointer data)
{
  NNode *node;

  node = (NNode*)node_list->data;
  node->data = data;
}

static
gpointer
nlc_interface_new(GNode *node_list)
{
  EMLInterface *iface;

  iface = eml_interface_new();
  iface->name = g_strdup("");

  return iface;
}

static
void
nlc_interface_destroy(GNode* node_list, const gpointer data)
{
  EMLProcessDialog *dlg;
  EMLInterface *iface;
  EMLConnected *conn;
  NNode *node;
  GList *list;

  node = (NNode*) node_list->data;
  dlg = ((EMLProcess*) node->user_data)->properties_dialog;
  iface = (EMLInterface*) node->current_el;

  list = iface->functions;
  while (list != NULL) {
    conn = (EMLConnected*) list->data;
    dlg->deleted_connections = g_list_append(dlg->deleted_connections,
                                             conn->left_connection);
    dlg->deleted_connections = g_list_append(dlg->deleted_connections,
                                             conn->right_connection);
  list = g_list_next(list);
  }

  list = iface->messages;
  while (list != NULL) {
    conn = (EMLConnected*) list->data;
    dlg->deleted_connections = g_list_append(dlg->deleted_connections,
                                             conn->left_connection);
    dlg->deleted_connections = g_list_append(dlg->deleted_connections,
                                             conn->right_connection);
  list = g_list_next(list);
  }

  eml_interface_destroy(iface);
}

static
gchar**
nlc_interface_to_raw(const GNode *node_list, const gpointer data)
{
  EMLInterface *iface;
  gchar* *str;

  str = g_new0(gchar*, 2);
  iface = (EMLInterface*) data;
  str[0] = g_strdup(iface->name);

  return str;
}

static
void
nlc_interface_after_select(GNode *node_list)
{
  EMLInterface *iface, *edit_iface;
  NNode *node;

  node = (NNode*) node_list->data;
  iface = (EMLInterface*) node->current_el;
  edit_iface = (EMLInterface*) node->edit_el;
  
  g_free(edit_iface->name);
  edit_iface->name = g_strdup(iface->name);
}

static void nlc_interface_after_update(GNode *node_list)
{
  EMLInterface *iface, *edit_iface;
  NNode *node;

  node = (NNode*) node_list->data;
  iface = (EMLInterface*) node->current_el;
  edit_iface = (EMLInterface*) node->edit_el;

  g_assert(iface != NULL);
 
  g_free(iface->name);
  iface->name = g_strdup(edit_iface->name);

}

static
void
interface_set_name(GtkWidget *entry, GdkEventFocus *ev, GNode *node_list)
{
  NNode *node;
  EMLInterface *iface;

  node = (NNode*) node_list->data;
  iface = (EMLInterface *) node->edit_el;
  
  if (iface == NULL)
    return;

  g_free(iface->name);
  iface->name =  g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
}

static void interface_show_name(GNode *node_list, GtkWidget *entry)
{
  NNode *node;
  EMLInterface *iface;

  node = (NNode*) node_list->data;
  iface = (EMLInterface *) node->edit_el;
  
  if (iface == NULL)
    return;

  gtk_entry_set_text(GTK_ENTRY(entry), iface->name);
}

static
GNode*
interfaces_dialog_create(EMLProcess *emlprocess,
                         GtkWidget **widget)
{
  GNode *node_list;
  GNode *msg_list;
  GtkWidget *msg_widget;
  GNode *fun_list;
  GtkWidget *fun_widget;
  NNode *node;
  GtkWidget *entry;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *frame;
  GtkWidget *notebook;

  node_list = nlist_node_new(1,
                             _("Interfaces"),
                             _("Interface"),
                             NULL,
                             NULL,
                             &EMLInterfaceNLC,
                             emlprocess);

  node = (NNode*) node_list->data;
  vbox = nlist_create_box(node_list);

  frame = gtk_frame_new(node->data_title);
  vbox2 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show(frame);

  hbox2 = create_dialog_entry(_("Interface name:"), &entry);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
  gtk_widget_show(hbox2);

  nlist_add_entry_widget(node_list,
                         GTK_WIDGET(entry),
                         interface_show_name,
                         interface_set_name);

  gtk_widget_show(vbox2);

/* functions */
  fun_list = iffunctions_dialog_create(emlprocess, &fun_widget);

  nlist_add_child(node_list, fun_list, fun_widget);

/* messages */
  msg_list = parameters_dialog_create(emlprocess,
                                      _("Interface messages"),
                                      _("Message"),
                                      _("Message parameters"),
                                      _("Message parameter"),
                                      SHOW_RELATIONS,
                                      &EMLIfmessageNLC,
                                      &msg_widget);

  nlist_add_child(node_list, msg_list, msg_widget);

  notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);

  gtk_box_pack_start (GTK_BOX (vbox2), notebook, TRUE, TRUE, 0);

  gtk_notebook_append_page( GTK_NOTEBOOK(notebook), fun_widget,
                            gtk_label_new(_("Functions")));

  gtk_notebook_append_page( GTK_NOTEBOOK(notebook), msg_widget,
                            gtk_label_new(_("Messages")));

  gtk_widget_show(notebook);

  *widget = vbox;

  return node_list;
}

static
void
emlprocess_update_connections(EMLProcess *emlprocess)
{
  DiaObject *obj;
  GList *list;
  int i;

  obj = (DiaObject *) emlprocess;
  obj->num_connections = g_list_length(emlprocess->box_connections);
  obj->connections =
    g_realloc(obj->connections,
	      obj->num_connections*sizeof(ConnectionPoint *));

  list = g_list_nth(emlprocess->box_connections, EMLPROCESS_STCONN);
  for (i = EMLPROCESS_STCONN; list != NULL; i++) {
    obj->connections[i] = (ConnectionPoint *) list->data;
    list = g_list_next(list);
  }

}

static
void 
emlprocess_set_state(EMLProcess *emlprocess, EMLProcessState *state)
{

  emlprocess_fill_from_state(emlprocess, state);
  g_free(state);

  emlprocess_destroy_box(emlprocess);
  emlprocess_create_box(emlprocess);
  emlprocess_calculate_data(emlprocess);
  emlprocess_calculate_connections(emlprocess);
  emlprocess_update_connections(emlprocess);
  emlprocess_update_data(emlprocess);
}

void
emlprocess_dialog_destroy(EMLProcess *emlprocess)
{
  EMLProcessDialog *dlg;

  g_assert(emlprocess != NULL);
  g_assert(emlprocess->properties_dialog != NULL);

  dlg = emlprocess->properties_dialog;

  if (dlg->dialog.d != NULL)
      gtk_widget_destroy(dlg->dialog.d);

  EMLPROCESS_FREE_STATE(&emlprocess->dialog_state);
  /* entries state vars */

  list_free_foreach(dlg->added_connections, NULL);
  g_list_free(dlg->added_connections);
  g_list_free(dlg->deleted_connections);
  g_list_free(dlg->disconnected_connections);

  nlist_destroy(dlg->startupfun_pg->parameters);
  g_free(dlg->startupfun_pg);

  nlist_destroy(dlg->interface_pg->interfaces);
  g_free(dlg->interface_pg);

  g_free(dlg->process_pg);
  g_free(dlg);

}

static
void
create_process_page(EMLProcess *emlprocess)
{
  GtkWidget *entry;
  EMLProcessDialog *dlg = emlprocess->properties_dialog;
  EMLProcessDialogState *dialog_state = &emlprocess->dialog_state;

  PROPDLG_PAGE_CREATE(dlg, dlg->nb, process_pg, _("Process"));

  if (!dlg->dialog.ready) {

    entry = create_dialog_entry( _("Process name:"), &dialog_state->process_name);
    gtk_box_pack_start (GTK_BOX (dlg->process_pg->dialog.d),
                        entry, FALSE, TRUE, 0);

    entry = create_dialog_entry( _("Process reference name:"),
                                 &dialog_state->process_refname);
    gtk_box_pack_start (GTK_BOX (dlg->process_pg->dialog.d),
                        entry, FALSE, TRUE, 0);

    entry = create_dialog_entry( _("Process lifetime:"),
                                 &dialog_state->process_proclife);
    gtk_box_pack_start (GTK_BOX (dlg->process_pg->dialog.d),
                        entry, FALSE, TRUE, 0);

  }

  gtk_entry_set_text(GTK_ENTRY(dialog_state->process_name),
                     dialog_state->name);
  gtk_entry_set_text(GTK_ENTRY(dialog_state->process_refname),
                     dialog_state->refname);
  gtk_entry_set_text(GTK_ENTRY(dialog_state->process_proclife),
                     dialog_state->proclife);

  PROPDLG_PAGE_READY(dlg, nb, process_pg);
}

static
void
apply_process_page(EMLProcess *emlprocess)
{
  EMLProcessDialogState *dialog_state = &emlprocess->dialog_state;

  dialog_state->name =
    g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog_state->process_name)));
  dialog_state->refname =
    g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog_state->process_refname)));
  dialog_state->proclife =
    g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog_state->process_proclife)));

}

static
void
create_startupfun_page(EMLProcess *emlprocess)
{
  GtkWidget *page;
  GtkWidget *entry;
  GtkWidget *param_w;
  EMLProcessDialog *dlg = emlprocess->properties_dialog;
  EMLProcessDialogState *dialog_state;

  dialog_state = &emlprocess->dialog_state;

  PROPDLG_PAGE_CREATE(dlg, dlg->nb, startupfun_pg, "Startup");

  if (!dlg->dialog.ready) {

    entry = create_dialog_entry( _("Module name:"),
                                 &dialog_state->startupfun_module);
    gtk_box_pack_start (GTK_BOX (dlg->startupfun_pg->dialog.d),
                        entry, FALSE, TRUE, 0);

    entry = create_dialog_entry( _("Function name:"),
                                 &dialog_state->startupfun_name);
    gtk_box_pack_start(GTK_BOX(dlg->startupfun_pg->dialog.d),
                        entry, FALSE, TRUE, 0);

    dlg->startupfun_pg->parameters =
      parameters_dialog_create(emlprocess,
                               _("Function parameters"),
                               _("Parameter"),
                               _("Relation members"),
                               _("Relation member"),
                               SHOW_ALL,
                               &EMLParamNLC,
                               &param_w);

    page = dlg->startupfun_pg->dialog.d;
    gtk_box_pack_start (GTK_BOX (page), param_w, TRUE, TRUE, 0);
    gtk_widget_show(param_w);

  }

  gtk_entry_set_text(GTK_ENTRY(dialog_state->startupfun_module),
                     dialog_state->startupfun->module);
  gtk_entry_set_text(GTK_ENTRY(dialog_state->startupfun_name),
                     dialog_state->startupfun->name);

  nlist_set_node_data(dlg->startupfun_pg->parameters,
                        dialog_state->startupfun);
  nlist_show_data(dlg->startupfun_pg->parameters);

  PROPDLG_PAGE_READY(dlg, nb, startupfun_pg);

}

static
void
apply_startupfun_page(EMLProcess *emlprocess)
{
  EMLProcessDialogState *dialog_state = &emlprocess->dialog_state;

  dialog_state->startupfun->module =
    g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog_state->startupfun_module)));
  dialog_state->startupfun->name =
    g_strdup(gtk_entry_get_text(GTK_ENTRY(dialog_state->startupfun_name)));

}

static
void
create_interface_page(EMLProcess *emlprocess)
{
  GtkWidget *page;
  GtkWidget *iface_w;
  EMLProcessDialog *dlg = emlprocess->properties_dialog;
  EMLProcessDialogState *dialog_state;

  dialog_state = &emlprocess->dialog_state;

  PROPDLG_PAGE_CREATE(dlg, dlg->nb, interface_pg, "Interfaces");

  if (!dlg->dialog.ready) {

    dlg->interface_pg->interfaces =
      interfaces_dialog_create(emlprocess, &iface_w);

    page = dlg->interface_pg->dialog.d;
    gtk_box_pack_start (GTK_BOX (page), iface_w, TRUE, TRUE, 0);
    gtk_widget_show(iface_w);
  }
  nlist_set_node_data(dlg->interface_pg->interfaces,
                        &dialog_state->interfaces);
  nlist_show_data(dlg->interface_pg->interfaces);

  PROPDLG_PAGE_READY(dlg, nb, interface_pg);

}

PROPDLG_TYPE
emlprocess_get_properties(EMLProcess *emlprocess)
{
  EMLProcessDialog *dlg;
  EMLProcessDialogState *dialog_state;

  dialog_state = &emlprocess->dialog_state;

  EMLPROCESS_FREE_STATE(dialog_state);
  EMLPROCESS_STATE_COPY(emlprocess, dialog_state);

  PROPDLG_CREATE(emlprocess->properties_dialog, dialog_state);

  dlg = emlprocess->properties_dialog;

  list_free_foreach(dlg->added_connections, NULL);
  g_list_free(dlg->added_connections);
  g_list_free(dlg->deleted_connections);

  dlg->added_connections = NULL;
  dlg->deleted_connections = NULL;
  dlg->disconnected_connections = NULL;

  PROPDLG_NOTEBOOK_CREATE(dlg, nb);

  create_process_page(emlprocess);
  create_startupfun_page(emlprocess);
  create_interface_page(emlprocess);

  PROPDLG_NOTEBOOK_READY(dlg, nb);
  PROPDLG_READY(dlg);
  PROPDLG_RETURN(dlg);

}

ObjectChange
*emlprocess_apply_properties(EMLProcess *emlprocess)
{
  GList *list;
  GList *added, *deleted, *disconnected;
  EMLProcessDialog *dlg;
  EMLProcessState *old_state = NULL;
  EMLProcessDialogState *dialog_state = &emlprocess->dialog_state;

  dlg = emlprocess->properties_dialog;
  
  PROPDLG_SANITY_CHECK(dlg, dialog_state);

  old_state = emlprocess_get_state(emlprocess);

  apply_process_page(emlprocess);
  apply_startupfun_page(emlprocess);

  /* destroy current */
  g_free(emlprocess->name);
  g_free(emlprocess->refname);
  g_free(emlprocess->proclife);
  eml_function_destroy(emlprocess->startupfun);
  g_list_foreach(emlprocess->interfaces, list_foreach_fun,
                 eml_interface_destroy);
  g_list_free(emlprocess->interfaces);

  /* copy */
  EMLPROCESS_STATE_COPY(dialog_state, emlprocess);

  /* unconnect from all deleted connectionpoints. */
  list = dlg->deleted_connections;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;

    emlprocess_store_disconnects(dlg, connection);
    object_remove_connections_to(connection);
    
    list = g_list_next(list);
  }
  
  deleted = dlg->deleted_connections;
  dlg->deleted_connections = NULL;
  
  added = dlg->added_connections;
  dlg->added_connections = NULL;
    
  disconnected = dlg->disconnected_connections;
  dlg->disconnected_connections = NULL;

  emlprocess_destroy_box(emlprocess);
  emlprocess_create_box(emlprocess);
  emlprocess_calculate_data(emlprocess);
  emlprocess_calculate_connections(emlprocess);
  emlprocess_update_connections(emlprocess);
  emlprocess_update_data(emlprocess);

  return  new_emlprocess_change(emlprocess, old_state, added, deleted,
                                disconnected);
}

/****************** UNDO stuff: ******************/

static
void
emlprocess_fill_from_state(EMLProcess *emlprocess, EMLProcessState *state)
{
  emlprocess->name       = state->name;
  emlprocess->refname       = state->refname;
  emlprocess->proclife   = state->proclife;
  emlprocess->startupfun = state->startupfun;
  emlprocess->interfaces = state->interfaces;
}

static
EMLProcessState *
emlprocess_get_state(EMLProcess *emlprocess)
{
  EMLProcessState *state = g_new( EMLProcessState, 1);
  state->obj_state.free = NULL;

  EMLPROCESS_STATE_COPY(emlprocess, state);

  return state;
}

static void
emlprocess_change_apply(EMLProcessChange *change, DiaObject *obj)
{
  EMLProcessState *old_state;
  GList *list;
  
  old_state = emlprocess_get_state(change->obj);

  emlprocess_set_state(change->obj, change->saved_state);

  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *)list->data;

    object_unconnect(dis->other_object, dis->other_handle);

    list = g_list_next(list);
  }

  change->saved_state = old_state;
  change->applied = 1;
}

static void
emlprocess_change_revert(EMLProcessChange *change, DiaObject *obj)
{
  EMLProcessState *old_state;
  GList *list;
  
  old_state = emlprocess_get_state(change->obj);

  emlprocess_set_state(change->obj, change->saved_state);
  
  list = change->disconnected;
  while (list) {
    Disconnect *dis = (Disconnect *)list->data;

    object_connect(dis->other_object, dis->other_handle, dis->cp);

    list = g_list_next(list);
  }
  
  change->saved_state = old_state;
  change->applied = 0;
}

static void
emlprocess_change_free(EMLProcessChange *change)
{
  GList *list, *free_list;

  EMLPROCESS_FREE_STATE(change->saved_state);
  g_free(change->saved_state);

  if (change->applied) 
    free_list = change->deleted_cp;
  else
    free_list = change->added_cp;

  list = free_list;
  while (list != NULL) {
    ConnectionPoint *connection = (ConnectionPoint *) list->data;
    
    g_assert(connection->connected == NULL); /* Paranoid */
    object_remove_connections_to(connection); /* Shouldn't be needed */
    g_free(connection);
      
    list = g_list_next(list);
  }

  g_list_free(free_list);
  
}

static ObjectChange *
new_emlprocess_change(EMLProcess *obj, EMLProcessState *saved_state,
		    GList *added, GList *deleted, GList *disconnected)
{
  EMLProcessChange *change;

  change = g_new(EMLProcessChange, 1);
  
  change->obj_change.apply =
    (ObjectChangeApplyFunc) emlprocess_change_apply;
  change->obj_change.revert =
    (ObjectChangeRevertFunc) emlprocess_change_revert;
  change->obj_change.free =
    (ObjectChangeFreeFunc) emlprocess_change_free;

  change->obj = obj;
  change->saved_state = saved_state;
  change->applied = 1;

  change->added_cp = added;
  change->deleted_cp = deleted;
  change->disconnected = disconnected;

  return (ObjectChange *)change;
}
