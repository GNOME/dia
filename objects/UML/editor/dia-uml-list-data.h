#include <glib-object.h>

#ifndef UML_LIST_DATA_H
#define UML_LIST_DATA_H

#define DIA_UML_TYPE_LIST_DATA (dia_uml_list_data_get_type ())
G_DECLARE_INTERFACE (DiaUmlListData, dia_uml_list_data, DIA_UML, LIST_DATA, GObject)

struct _DiaUmlListDataInterface {
  GTypeInterface parent_iface;
  
  const gchar * (* format) (DiaUmlListData *self);
};

void         dia_uml_list_data_changed (DiaUmlListData *self);
const gchar *dia_uml_list_data_format  (DiaUmlListData *self);

#endif
