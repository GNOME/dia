#include <glib-object.h>
#include "dia-list-data.h"

G_DEFINE_INTERFACE (DiaListData, dia_list_data, G_TYPE_OBJECT)

enum {
  CHANGED,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static const gchar *
dia_list_data_default_format (DiaListData *self)
{
  return G_OBJECT_TYPE_NAME (self);
}

static void
dia_list_data_default_init (DiaListDataInterface *iface)
{
  iface->format = dia_list_data_default_format;

  signals[CHANGED] = g_signal_new ("changed",
                                   G_TYPE_FROM_CLASS (iface),
                                   G_SIGNAL_RUN_FIRST,
                                   0, NULL, NULL, NULL,
                                   G_TYPE_NONE, 0);
}

void
dia_list_data_changed (DiaListData *self)
{
  g_signal_emit (G_OBJECT (self), signals[CHANGED], 0);
}

const gchar *
dia_list_data_format (DiaListData *self)
{
  DiaListDataInterface *iface;
  
  g_return_val_if_fail (DIA_IS_LIST_DATA (self), NULL);
  
  iface = DIA_LIST_DATA_GET_IFACE (self);
  g_return_val_if_fail (iface->format != NULL, NULL);
  
  return iface->format (self);
}
