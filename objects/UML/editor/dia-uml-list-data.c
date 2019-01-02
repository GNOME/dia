#include <glib-object.h>
#include "dia-uml-list-data.h"

G_DEFINE_INTERFACE (DiaUmlListData, dia_uml_list_data, G_TYPE_OBJECT)

enum {
  CHANGED,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static const gchar *
dia_uml_list_data_default_format (DiaUmlListData *self)
{
  return G_OBJECT_TYPE_NAME (self);
}

static void
dia_uml_list_data_default_init (DiaUmlListDataInterface *iface)
{
  iface->format = dia_uml_list_data_default_format;

  signals[CHANGED] = g_signal_new ("changed",
                                   G_TYPE_FROM_CLASS (iface),
                                   G_SIGNAL_RUN_FIRST,
                                   0, NULL, NULL, NULL,
                                   G_TYPE_NONE, 0);
}

void
dia_uml_list_data_changed (DiaUmlListData *self)
{
  g_signal_emit (G_OBJECT (self), signals[CHANGED], 0);
}

const gchar *
dia_uml_list_data_format (DiaUmlListData *self)
{
  DiaUmlListDataInterface *iface;
  
  g_return_val_if_fail (DIA_UML_IS_LIST_DATA (self), NULL);
  
  iface = DIA_UML_LIST_DATA_GET_IFACE (self);
  g_return_val_if_fail (iface->format != NULL, NULL);
  
  return iface->format (self);
}
