#ifndef NLIST_H
#define NLIST_H

#include <gtk/gtk.h>

typedef struct _NNode NNode;

typedef struct _NListController NListController;

typedef gchar** (* const NLCtoRowData) (const GNode*, const gpointer);

typedef void (* const RowDataToNLC) (const GNode*, const gpointer,
                                     const gchar**);

typedef void (* NListEntryUpdate) ( GNode *, GtkWidget *widget);

struct _NNode {

  int num_cols;
  gchar *head_title;
  gchar *data_title;
  gchar** col_titles;
  GtkCList *gtkclist;
  GList *entry_widgets;
  GList *entry_callbacks;
  GList *sensitive_widgets;
  GList **data;
  gpointer current_el;
  gpointer edit_el;
  NListController* data_controller;
  gpointer user_data;
};

struct _NListController {
  
  void (* const set_node_data) (GNode *, gpointer);

  gpointer (* const new_element) (GNode*);

  void (* const destroy_element) (GNode*, const gpointer);

  NLCtoRowData to_row;

  void (* const after_select) (GNode*);

  void (* const after_update) (GNode*);

  gpointer (* const create_temp_data) (GNode*);

  void (* const destroy_temp_data) (GNode*, const gpointer);

};

void nlist_set_node_data(GNode *, gpointer);
void nlist_show_data(GNode*);
GtkWidget* nlist_create_box(GNode*);
void nlist_destroy(GNode*);
void nlist_add_child(GNode *, GNode *, GtkWidget *);
void nlist_select_row(GNode *, gint);
GNode* nlist_node_new(int, gchar *, gchar*, gchar **, GList**,
                     NListController *, gpointer user_data);
void nlist_add_entry_widget(GNode *, GtkWidget *, NListEntryUpdate,
                            GtkSignalFunc);

#endif
