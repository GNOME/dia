/* 
 * exit_dialog.h: Dialog to allow the user to choose which data to
 * save on exit or to cancel exit.
 *
 * Copyright (C) 2007 Patrick Hallinan
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

/* TODO: Non-modal api */

#include <config.h>

#include "exit_dialog.h"

#include "intl.h"

#include <gtk/gtk.h>

#include <glib.h>

#define EXIT_DIALOG_TREEVIEW "EXIT_DIALOG_TREEVIEW"

enum {
  CHECK_COL,
  NAME_COL,
  PATH_COL,
  DATA_COL, /* To store optional data (not shown in listview) */
  NUM_COL
};

static void selected_state_set_all   (GtkTreeView * treeview,
                                      gboolean      state);

static gint get_selected_items (GtkWidget * dialog,
                                exit_dialog_item_array_t ** items);

/* Event Handlers */
static void exit_dialog_destroy  (GtkWidget * exit_dialog,
                                  gpointer    data);

static void select_all_clicked   (GtkButton * button,
                                  gpointer    data);

static void select_none_clicked  (GtkButton * button,
                                  gpointer    data);

static void toggle_check_button  (GtkCellRendererToggle * renderer,
                                  gchar * path,
                                  GtkTreeView * treeview);

/* A dialog to allow a user to select which unsaved files to save
 * (if any) or to abort exiting
 *
 * @param parent_window This is needed for modal behavior.
 * @param title Text display on the dialog's title bar.
 * @return The dialog.
 */
GtkWidget *
exit_dialog_make (GtkWindow * parent_window,
                  gchar *     title)
{
    GtkWidget * dialog = gtk_dialog_new_with_buttons (title, parent_window,
                                                      GTK_DIALOG_MODAL,
                                                      _("_Do Not Exit"),
                                                      EXIT_DIALOG_EXIT_CANCEL,
                                                      _("_Exit Without Save"),
                                                      EXIT_DIALOG_EXIT_NO_SAVE,
                                                      _("_Save Selected"),
                                                      EXIT_DIALOG_EXIT_SAVE_SELECTED,
                                                      NULL);

    GtkBox * vbox = GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(dialog)));

    GtkWidget * label = gtk_label_new (_("The following are not saved:"));

    GtkWidget * scrolled;
    GtkWidget * button;
    GtkWidget * hbox;

    GtkWidget *         treeview;
    GtkListStore *      model;
    GtkCellRenderer *   renderer;
    GtkTreeViewColumn * column;

    GdkGeometry geometry = { 0 };

    gtk_box_pack_start (vbox, label, FALSE, FALSE, 0);
    
    gtk_widget_show (label);

    /* Scrolled window for displaying things which need saving */
    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_box_pack_start (vbox, scrolled, TRUE, TRUE, 0);
    gtk_widget_show (scrolled);

    model = gtk_list_store_new (NUM_COL, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

    treeview = gtk_tree_view_new_with_model ( GTK_TREE_MODEL(model));

    renderer = gtk_cell_renderer_toggle_new ();

    column   = gtk_tree_view_column_new_with_attributes (_("Save"), renderer,
                                                         "active", CHECK_COL,
                                                          NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    g_signal_connect (G_OBJECT (renderer), "toggled",
                      G_CALLBACK (toggle_check_button), 
                      treeview);

    renderer = gtk_cell_renderer_text_new ();
    column   = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
                                                         "text", NAME_COL,
                                                          NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

    renderer = gtk_cell_renderer_text_new ();
    column   = gtk_tree_view_column_new_with_attributes (_("Path"), renderer,
                                                         "text", PATH_COL,
                                                          NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);


    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled), GTK_WIDGET (treeview));

 
    hbox = gtk_hbox_new (FALSE, 3);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    button = gtk_button_new_with_label (_("Select All"));
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (select_all_clicked), 
                      treeview);

    button = gtk_button_new_with_label (_("Select None"));
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 3);
    gtk_widget_show (button);

    g_signal_connect (G_OBJECT (button), "clicked",
                      G_CALLBACK (select_none_clicked), 
                      treeview);

    g_object_unref (model);
    gtk_widget_show (GTK_WIDGET (treeview));

    gtk_widget_show_all (GTK_WIDGET(vbox));

    g_object_set_data (G_OBJECT (dialog), EXIT_DIALOG_TREEVIEW,  treeview);
    
    g_signal_connect (G_OBJECT (dialog), "destroy",
                      G_CALLBACK (exit_dialog_destroy), 
                      treeview);

    /* golden ratio */
    geometry.min_aspect = 0.618;
    geometry.max_aspect = 1.618;
    geometry.win_gravity = GDK_GRAVITY_CENTER;
    gtk_window_set_geometry_hints (GTK_WINDOW (dialog), GTK_WIDGET (vbox), &geometry,
                                   GDK_HINT_ASPECT|GDK_HINT_WIN_GRAVITY);
    return dialog;
}

/**
 * Add name and path of a file that needs to be saved
 * @param dialog Exit dialog created with exit_dialog_make()
 * @param name User identifiable name of the thing which needs saving.
 * @param path File system path of the thing which needs saving.
 * @param optional_data Optional data to be returned with selected file info.
 */
void
exit_dialog_add_item (GtkWidget *    dialog,
                      const gchar *  name, 
                      const gchar *  path, 
                      const gpointer optional_data)
{
    GtkTreeView *  treeview;
    GtkTreeIter    iter;
    GtkListStore * model;
    const gchar *  name_copy = g_strdup (name);
    const gchar *  path_copy = g_strdup (path);

    treeview = g_object_get_data (G_OBJECT (dialog), EXIT_DIALOG_TREEVIEW);
  
    model = GTK_LIST_STORE (gtk_tree_view_get_model (treeview)); 

    gtk_list_store_append (model, &iter);

    gtk_list_store_set (model, &iter,
                        CHECK_COL, 1,
                        NAME_COL, name_copy,
                        PATH_COL, path_copy,
                        DATA_COL, optional_data,
                        -1);
}

gint
exit_dialog_run (GtkWidget * dialog,
                 exit_dialog_item_array_t ** items)
{
    gint                       result;
    gint                       count;
    
    while (TRUE)
    {
        result = gtk_dialog_run ( GTK_DIALOG(dialog));

        switch (result) 
        {
            case EXIT_DIALOG_EXIT_CANCEL:
            case EXIT_DIALOG_EXIT_NO_SAVE:
                *items = NULL;
                return result;
	    case EXIT_DIALOG_EXIT_SAVE_SELECTED :
		break;
	    default : /* e.g. if closed by window manager button */
	        return EXIT_DIALOG_EXIT_CANCEL;
        }
    
        count = get_selected_items (dialog, items);
    
        if (count == 0)
        {
           GtkWidget * msg_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                                            GTK_DIALOG_MODAL,
                                                            GTK_MESSAGE_WARNING,
                                                            GTK_BUTTONS_YES_NO,
                                                            _("Nothing selected for saving.  Would you like to try again?"));

           gint yes_or_no = gtk_dialog_run (GTK_DIALOG (msg_dialog));

           gtk_widget_destroy (msg_dialog);

           if (yes_or_no == GTK_RESPONSE_NO)
           {
               return EXIT_DIALOG_EXIT_NO_SAVE;
           }
        }
        else
        {
            return EXIT_DIALOG_EXIT_SAVE_SELECTED;
        }
    }
}


/**
 * Gets the list of items selected for saving by the user.
 * @param dialog Exit dialog.
 * @param items Structure to hold the selected items.  Set to NULL if not items selected.
 * @return The number of selected items.
 */
gint
get_selected_items (GtkWidget * dialog,
                    exit_dialog_item_array_t ** items)
{
    GtkTreeView *              treeview;
    GtkTreeIter                iter;
    GtkListStore *             model;
    gboolean                   valid;
    GSList *                   list = NULL;
    GSList *                   list_iter;
    gint                       selected_count;
    gint                       i;

    treeview = g_object_get_data (G_OBJECT (dialog), EXIT_DIALOG_TREEVIEW);
  
    model = GTK_LIST_STORE (gtk_tree_view_get_model (treeview)); 
  
    /* Get the first iter in the list */
    valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter);

    /* Get the selected items */
    while (valid)
    {
        const char * name;
        const char * path;
        gpointer     data;
        gboolean     is_selected;

        gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
                            CHECK_COL, &is_selected,
                            NAME_COL,  &name,
                            PATH_COL,  &path,
                            DATA_COL,  &data,
                            -1);

        if (is_selected)
        {
            exit_dialog_item_t * item = g_new (exit_dialog_item_t,1);
            item->name = name;
            item->path = path;
            item->data = data;
            list = g_slist_prepend (list, item);
        }
        
        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter);
    }

    selected_count = g_slist_length (list);

    if (selected_count > 0)
    {
        *items              = g_new (exit_dialog_item_array_t,1);
       (*items)->array_size = selected_count;
       (*items)->array      = g_new (exit_dialog_item_t, (*items)->array_size);
    
        list_iter = list;
        for(i = 0 ; i < selected_count ; i++)
        {
	    exit_dialog_item_t * item;
            g_assert(list_iter!=NULL); /* can't be if g_slist_length works */
            item = list_iter->data;
            (*items)->array[i].name = item->name;
            (*items)->array[i].path = item->path;
            (*items)->array[i].data = item->data;
            list_iter = g_slist_next(list_iter);
        }
    
        g_slist_free (list);
    }
    else
    {
      *items = NULL;
    }

    return selected_count;
}

/**
 * Free memory allocated for the exit_dialog_item_array_t.  This
 * will not free any memory not allocated by the dialog itself
 * @param items Item array struct returned by exit_dialog_run()
 */
void exit_dialog_free_items (exit_dialog_item_array_t * items)
{
  if (items)
  {
    int i;
    for (i = 0 ; i < items->array_size ; i++)
    {
      /* Cast is needed to remove warning because of const decl */
      g_free ( (char*)items->array[i].name);
      g_free ( (char*)items->array[i].path);
    }

    g_free (items);
  }
}

/** 
 * Handler for the check box (button) in the exit dialogs treeview.
 * This is needed to cause the check box to change state when the
 * user clicks it
 */
static void toggle_check_button (GtkCellRendererToggle * renderer,
                                 gchar * path,
                                 GtkTreeView * treeview)
{ 
    GtkTreeModel * model;
    GtkTreeIter    iter;
    gboolean       value;

    model = gtk_tree_view_get_model (treeview);
    if (gtk_tree_model_get_iter_from_string (model, &iter, path))
    {
        gtk_tree_model_get (model, &iter, CHECK_COL, &value, -1);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, CHECK_COL, !value, -1);
    }
}

/*
 * Signal handler for "destroy" event to free memory allocated for the exit_dialog.
 *
 * @param exit_dialog 
 * @param data Exit dialog's treeview
 */
static void exit_dialog_destroy  (GtkWidget * exit_dialog,
                                  gpointer    data)
{
    GtkTreeView *  treeview;
    GtkTreeIter    iter;
    GtkTreeModel * model;
    gboolean       valid;
  
    treeview = g_object_get_data (G_OBJECT (exit_dialog), EXIT_DIALOG_TREEVIEW);

    model = GTK_TREE_MODEL (gtk_tree_view_get_model (treeview)); 
    
    /* Get the first iter in the list */
    valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter);

    while (valid)
    {
        gchar * name = NULL;
        gchar * path = NULL;

        gtk_tree_model_get (model, &iter,
                            NAME_COL, &name,
                            PATH_COL, &path,
                            -1);

        g_free (name);
        g_free (path);

        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter);
    }
}

/**
 * Sets the state of the save checkbox in the treeview to the
 * state specified by the caller.
 * @param treeview The treeview in the exit dialog box.
 * @param state Set to TRUE to select all, FALSE to de-select all.
 */
static void selected_state_set_all (GtkTreeView * treeview,
                                    gboolean      state)
{
    GtkTreeIter    iter;
    GtkListStore * model;
    gboolean       valid;

    model = GTK_LIST_STORE (gtk_tree_view_get_model (treeview)); 
  
    /* Get the first iter in the list */
    valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter);

    while (valid)
    {
        gtk_list_store_set (model, &iter,
                            CHECK_COL, state,
                            -1);

        valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter);
    }
}

/**
 * Handler for Select All button.  Causes all checkboxes in the
 * exit dialog box treeview to be checked.
 * @param button The Select all button.
 * @param data The treeview
 */
static void select_all_clicked   (GtkButton * button,
                                  gpointer    data)
{
    selected_state_set_all (data, TRUE);
}

/**
 * Handler for Select All button.  Causes all checkboxes in the
 * exit dialog box treeview to be un-checked.
 * @param button The Select all button.
 * @param data The treeview
 */
static void select_none_clicked  (GtkButton * button,
                                  gpointer    data)
{
    selected_state_set_all (data, FALSE);
}


