#include <glib-object.h>
#include "dia-list-data.h"

#ifndef LIST_STORE_H
#define LIST_STORE_H

G_BEGIN_DECLS

#define DIA_TYPE_LIST_STORE (dia_list_store_get_type ())
G_DECLARE_FINAL_TYPE (DiaListStore, dia_list_store, DIA, LIST_STORE, GObject)

DiaListStore *dia_list_store_new    (void);
void          dia_list_store_insert (DiaListStore *store,
                                     DiaListData  *item,
                                     int              index);
void          dia_list_store_add    (DiaListStore *store,
                                     DiaListData  *item);
void          dia_list_store_remove (DiaListStore *store,
                                     DiaListData  *item);
void          dia_list_store_empty  (DiaListStore *store);

G_END_DECLS

#endif
