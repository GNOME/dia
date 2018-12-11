#ifndef __DIALIST_H__
#define __DIALIST_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

G_BEGIN_DECLS


#define DIA_TYPE_LIST (dia_list_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaList, dia_list, DIA, LIST, GtkFrame)

struct _DiaListClass {
	GtkFrameClass parent_class;
};

#define DIA_TYPE_LIST_ITEM (dia_list_item_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaListItem, dia_list_item, DIA, LIST_ITEM, GtkListBoxRow)

struct _DiaListItemClass {
	GtkListBoxRowClass parent_class;
};

void             dia_list_select_child        (DiaList          *self,
                                               DiaListItem      *widget);
void             dia_list_unselect_child      (DiaList          *self,
                                               DiaListItem      *widget);
void             dia_list_append_items        (DiaList          *self,
                                               GList            *items);
void             dia_list_insert_items        (DiaList          *self,
                                               GList            *items,
                                               gint              i);
void             dia_list_remove_items        (DiaList          *self,
                                               GList            *items);
DiaListItem     *dia_list_append              (DiaList          *self,
                                               const gchar      *item);
void             dia_list_add                 (DiaList          *self,
                                               GtkListBoxRow    *itm);
void             dia_list_set_active          (DiaList          *self,
                                               gint              index);
void             dia_list_add_seperator       (DiaList          *self);
void             dia_list_select_item         (DiaList          *self,
                                               gint              i);
void             dia_list_empty               (DiaList          *self);
gint             dia_list_child_position      (DiaList          *self,
                                               DiaListItem      *row);
void             dia_list_unselect_all        (DiaList          *self);
void             dia_list_select_all          (DiaList          *self);
GtkWidget       *dia_list_new                 (void);
GList           *dia_list_get_children        (DiaList          *self);
DiaListItem     *dia_list_get_selection       (DiaList          *self);
GtkSelectionMode dia_list_get_selection_mode  (DiaList          *self);
void             dia_list_set_selection_mode  (DiaList          *self,
                                               GtkSelectionMode  value);
GtkWidget       *dia_list_item_new_with_label (const gchar      *lbl);
GtkWidget       *dia_list_item_new            (void);
const gchar     *dia_list_item_get_value      (DiaListItem      *self);
void             dia_list_item_set_value      (DiaListItem      *self,
                                               const gchar      *value);

G_END_DECLS

#endif
