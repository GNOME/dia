#include <string.h> /* strcmp */

#include "nlist.h"
#include "intl.h"

typedef struct _ListButton ListButton;

struct _ListButton {
    gchar *name;
    GtkSignalFunc callback;
};

static void list_buttons_free(GSList* buttons) {
    
    GSList *list = buttons;
    ListButton *button;
    
    while (list != NULL) {
        button = list->data;
        g_free(button->name);
        g_free(button);
        list = g_slist_next(list);
    }

    g_slist_free(buttons);
}

static GSList* create_move_buttons(GtkSignalFunc new_callback,
                                   GtkSignalFunc delete_callback,
                                   GtkSignalFunc up_callback,
                                   GtkSignalFunc down_callback) {

  ListButton *button;
  GSList *list = NULL;
    
    button = g_new(ListButton, 1);
    button->name = g_strdup(_("New"));
    button->callback = new_callback;
    list = g_slist_append(list, button);

    button = g_new(ListButton, 1);
    button->name = g_strdup(_("Delete"));
    button->callback = delete_callback;
    list = g_slist_append(list, button);

    button = g_new(ListButton, 1);
    button->name = g_strdup(_("Move up"));
    button->callback = up_callback;
    list = g_slist_append(list, button);

    button = g_new(ListButton, 1);
    button->name = g_strdup(_("Move Down"));
    button->callback = down_callback;
    list = g_slist_append(list, button);

    return list;
    
}

static GSList* list_buttons_pack(GtkWidget *vbox,
                                 GSList *buttons,
                                 gpointer func_data) {

  GSList *ret_list = NULL;
  GSList *list = buttons;
  ListButton *move_button;
  GtkWidget *button;

  while (list != NULL) {
        move_button = (ListButton*) list->data;
        
        g_assert(move_button->name != NULL);
        
        button = gtk_button_new_with_label (move_button->name);
        gtk_signal_connect (GTK_OBJECT (button), "clicked",
                            GTK_SIGNAL_FUNC(move_button->callback),
                            func_data);
        gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, 0);
        gtk_widget_show (button);
        list = g_slist_next(list);
        ret_list = g_slist_append(ret_list, button);
    }
    return ret_list;
}

static void
nlist_selection_made( GtkWidget*, gint, gint, GdkEventButton*, GNode*);

static void
nlist_move_up_callback(GtkWidget*, GNode*);

static void
nlist_move_down_callback(GtkWidget*, GNode*);

static void
nlist_new_callback(GtkWidget*, GNode*);

static void
nlist_delete_callback(GtkWidget*, GNode*);

static void
nlist_set_sensitive(GNode*, gint);

static void
nlist_show_childs(GNode*);


void
nlist_add_child(GNode *parent, GNode *child, GtkWidget *child_w)
{
  NNode *parent_node;

  parent_node = (NNode*) parent->data;

  parent_node->sensitive_widgets =
    g_list_append(parent_node->sensitive_widgets, child_w);
  g_node_append(parent, child);
}

GtkWidget*
nlist_create_box(GNode* node_list)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *scrolled_win;
  GtkWidget *label;
  GtkWidget *list;
  GtkWidget *vbox2;
  GSList *move_buttons;
  NNode *node;

  node = (NNode*) node_list->data;

  vbox = gtk_vbox_new(FALSE, 5);
  hbox = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(node->head_title);

  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);
  
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  hbox = gtk_hbox_new(FALSE, 5);

  scrolled_win = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, 
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_clist_new_with_titles( node->num_cols, node->col_titles);
  gtk_clist_set_shadow_type (GTK_CLIST(list), GTK_SHADOW_IN);
  node->gtkclist = GTK_CLIST(list);
  gtk_clist_set_selection_mode (GTK_CLIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT (list), "select_row",
                      GTK_SIGNAL_FUNC(nlist_selection_made), node_list);

  vbox2 = gtk_vbox_new(FALSE, 5);

  move_buttons = create_move_buttons(nlist_new_callback,
                                     nlist_delete_callback,
                                     nlist_move_up_callback,
                                     nlist_move_down_callback);

  g_slist_free(list_buttons_pack(vbox2, move_buttons, node_list));
  list_buttons_free(move_buttons);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);
  gtk_widget_show(vbox2);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  gtk_widget_show_all (vbox);

  return vbox;

}

static
void
nlist_update_row(GNode* node_list, gint row, gpointer el)
{
  NNode *node;
  GtkCList *gtkclist;
  NListController* controller;
  gchar *cell_text;
  gchar* *param_data;
  gint idx;
  
  node = (NNode*) node_list->data;
  controller = node->data_controller;
  gtkclist = node->gtkclist;

  if (node->current_el == NULL)
      return;

  param_data = controller->to_row(node_list, el);

  for (idx = 0; idx < node->num_cols; idx++) {

      gtk_clist_get_text(gtkclist, row, idx, &cell_text);

      if (strcmp(param_data[idx], cell_text) != 0) {
          gtk_clist_set_text(gtkclist, row, idx, param_data[idx]);
      }
  }

  g_strfreev(param_data);

}

static void
nlist_update_row_from_entries(GNode* node_list)
{

  NNode *node;
  NListController *controller;
  gint current_row;
  GList *list;

  node = (NNode*) node_list->data;
  controller = node->data_controller;

  if (node->current_el == NULL)
    return;

  current_row = gtk_clist_find_row_from_data(node->gtkclist, node->current_el);
  g_assert(current_row != -1);

  if (controller->after_update != NULL)
      controller->after_update(node_list);

  list = g_list_nth(*node->data, current_row);
  list->data = node->current_el;

  nlist_update_row(node_list, current_row, node->current_el);
  gtk_clist_set_row_data_full(node->gtkclist, current_row,
                              node->current_el, NULL);
  
}

static void
nlist_data_update_entry(GtkWidget *entry,
                        GdkEventFocus *ev,
                        GNode     *node)
{
  nlist_update_row_from_entries(node);
  if (node->parent != NULL)
    nlist_update_row_from_entries(node->parent);
  
}

void
nlist_add_entry_widget(GNode *node_list, GtkWidget *widget,
                       NListEntryUpdate entry_update,
                       GtkSignalFunc focus_out_callback)
{
  NNode *node;

  node = (NNode*) node_list->data;
  node->entry_widgets = g_list_append(node->entry_widgets, widget);
  node->sensitive_widgets = g_list_append(node->sensitive_widgets, widget);
  node->entry_callbacks = g_list_append(node->entry_callbacks, entry_update);

  gtk_signal_connect (GTK_OBJECT (widget), "focus_out_event",
                      GTK_SIGNAL_FUNC(focus_out_callback), node_list);

  gtk_signal_connect (GTK_OBJECT (widget), "focus_out_event",
                      GTK_SIGNAL_FUNC(nlist_data_update_entry), node_list);

}

void
nlist_select_row(GNode *node_list, gint row)
{
  GtkCList *gtkclist;
  gpointer el;
  NNode *node;
  NListController *controller;
  GList *wlist;
  GList *flist;

  node = (NNode*) node_list->data;
  controller = node->data_controller;
  gtkclist = node->gtkclist;

  el = gtk_clist_get_row_data(gtkclist, row);
  node->current_el = el;

  if (controller->after_select != NULL)
      controller->after_select(node_list);

  if (el == NULL)
    return;

  wlist = node->entry_widgets;
  flist = node->entry_callbacks;
  while (wlist != NULL) {

    if (flist->data != NULL)
      ((NListEntryUpdate)flist->data)(node_list, GTK_WIDGET(wlist->data));

    wlist = g_list_next(wlist);
    flist = g_list_next(flist);

  }

  nlist_set_sensitive(node_list, TRUE);
  nlist_show_childs(node_list);
}

static void
nlist_selection_made( GtkWidget      *clist,
                      gint            row,
                      gint            column,
                      GdkEventButton *event,
                      GNode*       node_list)
{
  nlist_select_row(node_list, row);
}

static void
nlist_move_up_callback(GtkWidget *button,
                       GNode     *node_list)
{

  NNode *node;
  GtkCList *gtkclist;
  gint current_row;
  
  node  = (NNode*) node_list->data;
  gtkclist = node->gtkclist;

  if (node->current_el == NULL)
      return;

  current_row = gtk_clist_find_row_from_data(gtkclist, node->current_el);

  gtk_clist_select_row(gtkclist, --current_row, 0);

}

static void
nlist_move_down_callback(GtkWidget *button,
                         GNode     *node_list)
{

  NNode *node;
  GtkCList *gtkclist;
  gint current_row;
  
  node = (NNode*) node_list->data;
  gtkclist = node->gtkclist;

  if (node->current_el == NULL)
      return;

  current_row = gtk_clist_find_row_from_data(gtkclist, node->current_el);

  gtk_clist_select_row(gtkclist, ++current_row, 0);

}

static void
nlist_new_callback(GtkWidget *button,
                   GNode     *node_list)
{
  NNode *node;
  GtkCList *gtkclist;
  gint current_row;
  gpointer el;
  NListController* controller;
  gchar **param_data;

  node = (NNode*) node_list->data;
  gtkclist = node->gtkclist;
  controller = node->data_controller;
  el = controller->new_element(node_list);

  current_row = gtk_clist_find_row_from_data(gtkclist, node->current_el);
  current_row = (current_row == -1) ? 0 : current_row + 1;

  *node->data = g_list_insert(*node->data, el, current_row);

  param_data = controller->to_row(node_list, el);

  gtk_clist_set_row_data_full(
    gtkclist,
    gtk_clist_insert(gtkclist, current_row, param_data),
    el,
    NULL);
  gtk_clist_select_row(gtkclist, current_row, 0);

  g_strfreev(param_data);
}

static void nlist_refresh_row(GNode *node_list)
{
  NNode *node;
  gchar **row_data;
  gint idx;
  gint current_row;

  node = (NNode*) node_list->data;

  if (!G_NODE_IS_ROOT(node_list))
    nlist_refresh_row(node_list->parent);

  if (node->current_el == NULL)
    return;

  
  current_row = gtk_clist_find_row_from_data(node->gtkclist, node->current_el);
  if (current_row == -1)
    return;

  row_data = node->data_controller->to_row(node_list, node->current_el);
  for (idx = 0; idx < node->num_cols; idx++)
    gtk_clist_set_text(node->gtkclist, current_row, idx, row_data[idx] );

  g_strfreev(row_data);
}

static void
nlist_delete_callback(GtkWidget *button,
                      GNode     *node_list)
{

  NNode *node;
  GtkCList *gtkclist;
  gint current_row;
  NListController* controller;

  node = (NNode*) node_list->data;
  controller = node->data_controller;
  gtkclist = node->gtkclist;

  if (node->current_el == NULL)
      return;
  
  current_row = gtk_clist_find_row_from_data(gtkclist, node->current_el);
  
  gtk_clist_remove(gtkclist, current_row);
  *node->data = g_list_remove(*node->data, node->current_el);
  controller->destroy_element(node_list, node->current_el);
  node->current_el = NULL;
  nlist_set_sensitive(node_list, FALSE);
  gtk_clist_select_row(gtkclist,
                       (current_row == 0) ? 0 : --current_row, 0);

  nlist_refresh_row(node_list);
}

static void
nlist_set_sensitive(GNode *node_list, gint val)
{
  NNode *node;
  GList* list;

  node = (NNode*) node_list->data;
  list = node->sensitive_widgets;
  
  while (list != NULL) {
      gtk_widget_set_sensitive(GTK_WIDGET(list->data), val);
      list = g_list_next(list);
  }

}

static void
nlist_show_childs(GNode* node_list)
{

  NNode* node;
  GNode* child;
  gpointer current_el;

  current_el = ((NNode*)node_list->data)->current_el;
  child = g_node_first_child(node_list);
  while (child != NULL) {
    node = (NNode*) child->data;
    node->data_controller->set_node_data(child, current_el);
    nlist_show_data(child);
    child = g_node_next_sibling(child);
  }

}

void
nlist_show_data(GNode* node_list)
{
  NNode* node;
  NListController *controller;
  gpointer el;
  gchar* *row_data;
  gint row_num;
  GList *list;

  node = (NNode*) node_list->data;
  controller = node->data_controller;

  gtk_clist_clear(node->gtkclist);

  list = *node->data;
  while (list != NULL) {
    el = list->data;
    row_data = controller->to_row(node_list, el);
    row_num = gtk_clist_append(node->gtkclist, row_data);
    gtk_clist_set_row_data_full(node->gtkclist, row_num, el, NULL);
    g_strfreev(row_data);
    list = g_list_next(list);
  }

  nlist_set_sensitive(node_list, FALSE);

  node->current_el = NULL;
}

GNode*
nlist_node_new(int num_cols, gchar *head_title, gchar *data_title,
               gchar **col_titles, GList **data, NListController *controller,
               gpointer user_data)
{
  GNode *node_list;
  NNode *node;

  node = g_new(NNode, 1);
  node->gtkclist = NULL;
  node->sensitive_widgets = NULL;
  node->entry_widgets = NULL;
  node->entry_callbacks = NULL;
  node->current_el = NULL;
  node->data_controller = controller;
  node->head_title = g_strdup(head_title);
  node->data_title = g_strdup(data_title);
  node->col_titles = col_titles;
  node->num_cols = num_cols;
  node->data = data;
  node->user_data = user_data;

  node_list = g_node_new(node);

  node->edit_el =  (controller == NULL) ?
    NULL : controller->create_temp_data(node_list);

  return node_list;
}

void
nlist_set_node_data(GNode *node_list, gpointer data)
{
  NNode *node;

  node = (NNode*) node_list->data;
  node->data_controller->set_node_data(node_list, data);
  
}

static void  nlist_node_destroy(NNode *node)
{
  g_list_free(node->sensitive_widgets);
  g_list_free(node->entry_widgets);
  g_list_free(node->entry_callbacks);
  g_free(node->head_title);
  g_free(node->data_title);
  g_strfreev(node->col_titles);

  g_free(node);
}

void
nlist_destroy(GNode *node_list)
{

  NNode *node;
  GNode *child;

  node = (NNode*) node_list->data;

  child = g_node_first_child(node_list);
  while (child != NULL) {
    nlist_destroy(child);
    child = g_node_next_sibling(child);
  }

  if (node->data_controller != NULL)
    node->data_controller->destroy_temp_data(node_list, node->edit_el);

  nlist_node_destroy(node);
  if (G_NODE_IS_ROOT(node_list))
    g_node_destroy(node_list);

}
