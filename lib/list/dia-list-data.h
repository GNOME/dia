#include <glib-object.h>

#ifndef LIST_DATA_H
#define LIST_DATA_H

#define DIA_TYPE_LIST_DATA (dia_list_data_get_type ())
G_DECLARE_INTERFACE (DiaListData, dia_list_data, DIA, LIST_DATA, GObject)

struct _DiaListDataInterface {
  GTypeInterface parent_iface;
  
  const gchar * (* format) (DiaListData *self);
};

void         dia_list_data_changed (DiaListData *self);
const gchar *dia_list_data_format  (DiaListData *self);

#endif
