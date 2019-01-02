#include "dia-uml-list-store.h"
#include <glib.h>
#include <gio/gio.h>

struct _DiaUmlListStore
{
  GObject parent_instance;

  GList *data;
};

enum
{
  PROP_0,
  PROP_ITEM_TYPE,
  N_PROPERTIES
};

static void dia_uml_list_store_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (DiaUmlListStore, dia_uml_list_store, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, dia_uml_list_store_iface_init));

static void
dia_uml_list_store_dispose (GObject *object)
{
  DiaUmlListStore *store = DIA_UML_LIST_STORE (object);

  g_list_free_full (store->data, g_object_unref);

  G_OBJECT_CLASS (dia_uml_list_store_parent_class)->dispose (object);
}

static void
dia_uml_list_store_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  switch (property_id) {
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, DIA_UML_TYPE_LIST_DATA);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
dia_uml_list_store_class_init (DiaUmlListStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dia_uml_list_store_dispose;
  object_class->get_property = dia_uml_list_store_get_property;

  g_object_class_install_property (object_class, PROP_ITEM_TYPE,
    g_param_spec_gtype ("item-type", "", "", DIA_UML_TYPE_LIST_DATA,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static GType
dia_uml_list_store_get_item_type (GListModel *list)
{
  return DIA_UML_TYPE_LIST_DATA;
}

static guint
dia_uml_list_store_get_n_items (GListModel *list)
{
  DiaUmlListStore *store = DIA_UML_LIST_STORE (list);

  return g_list_length (store->data);
}

static gpointer
dia_uml_list_store_get_item (GListModel *list,
                             guint       position)
{
  DiaUmlListStore *store = DIA_UML_LIST_STORE (list);
  DiaUmlListData *item;

  item = g_list_nth_data (store->data, position);

  if (item)
    return g_object_ref (item);
  else
    return NULL;
}

static void
dia_uml_list_store_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = dia_uml_list_store_get_item_type;
  iface->get_n_items = dia_uml_list_store_get_n_items;
  iface->get_item = dia_uml_list_store_get_item;
}

static void
dia_uml_list_store_init (DiaUmlListStore *store)
{
  store->data = NULL;
}

DiaUmlListStore *
dia_uml_list_store_new ()
{
  return g_object_new (DIA_UML_TYPE_LIST_STORE, NULL);
}

void
dia_uml_list_store_insert (DiaUmlListStore *store,
                           DiaUmlListData  *item,
                           int              index)
{
  g_return_if_fail (DIA_UML_IS_LIST_STORE (store));

  if (index >= g_list_length (store->data)) {
    dia_uml_list_store_add (store, item);
    return;
  }

  store->data = g_list_insert (store->data, g_object_ref (item), index);

  g_list_model_items_changed (G_LIST_MODEL (store), index, 0, 1);
}

void
dia_uml_list_store_add (DiaUmlListStore *store,
                        DiaUmlListData  *item)
{
  guint n_items;

  g_return_if_fail (DIA_UML_IS_LIST_STORE (store));

  n_items = g_list_length (store->data);
  store->data = g_list_append (store->data, g_object_ref (item));

  g_list_model_items_changed (G_LIST_MODEL (store), n_items, 0, 1);
}

void
dia_uml_list_store_remove (DiaUmlListStore *store,
                           DiaUmlListData  *item)
{
  int index;

  g_return_if_fail (DIA_UML_IS_LIST_STORE (store));

  index = g_list_index (store->data, item);
  store->data = g_list_remove (store->data, item);
  g_object_unref (item);

  g_list_model_items_changed (G_LIST_MODEL (store), index, 1, 0);
}

void
dia_uml_list_store_empty (DiaUmlListStore *store)
{
  guint n_items;

  g_return_if_fail (DIA_UML_IS_LIST_STORE (store));

  n_items = g_list_length (store->data);
  g_list_free_full (store->data, g_object_unref);
  store->data = NULL;

  g_list_model_items_changed (G_LIST_MODEL (store), 0, n_items, 0);
}
