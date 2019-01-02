#include <glib-object.h>
#include "dia-uml-list-data.h"

#ifndef UML_LIST_STORE_H
#define UML_LIST_STORE_H

G_BEGIN_DECLS

#define DIA_UML_TYPE_LIST_STORE (dia_uml_list_store_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlListStore, dia_uml_list_store, DIA_UML, LIST_STORE, GObject)

DiaUmlListStore *dia_uml_list_store_new    ();
void             dia_uml_list_store_insert (DiaUmlListStore *store,
                                            DiaUmlListData  *item,
                                            int              index);
void             dia_uml_list_store_add    (DiaUmlListStore *store,
                                            DiaUmlListData  *item);
void             dia_uml_list_store_remove (DiaUmlListStore *store,
                                            DiaUmlListData  *item);
void             dia_uml_list_store_empty  (DiaUmlListStore *store);

G_END_DECLS

#endif
