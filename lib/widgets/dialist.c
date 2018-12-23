/* Minimal reimplementation of GtkList, prototyped in Vala */


#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "dialist.h"

typedef struct _DiaListPrivate DiaListPrivate;

struct _DiaListPrivate {
  GtkWidget *real;
};

G_DEFINE_TYPE_WITH_CODE (DiaList, dia_list, GTK_TYPE_FRAME,
                         G_ADD_PRIVATE (DiaList))

enum {
  DIA_LIST_0_PROPERTY,
  DIA_LIST_CHILDREN_PROPERTY,
  DIA_LIST_SELECTION_PROPERTY,
  DIA_LIST_SELECTION_MODE_PROPERTY,
  DIA_LIST_NUM_PROPERTIES
};
static GParamSpec* dia_list_properties[DIA_LIST_NUM_PROPERTIES];

enum {
  DIA_LIST_CHILD_SELECTED_SIGNAL,
  DIA_LIST_SELECTION_CHANGED_SIGNAL,
  DIA_LIST_NUM_SIGNALS
};
static guint dia_list_signals[DIA_LIST_NUM_SIGNALS] = {0};

typedef struct _DiaListItemPrivate DiaListItemPrivate;

struct _DiaListItemPrivate {
  GtkWidget *label;
};

G_DEFINE_TYPE_WITH_CODE (DiaListItem, dia_list_item, GTK_TYPE_LIST_BOX_ROW,
                         G_ADD_PRIVATE (DiaListItem))

enum {
  DIA_LIST_ITEM_0_PROPERTY,
  DIA_LIST_ITEM_VALUE_PROPERTY,
  DIA_LIST_ITEM_NUM_PROPERTIES
};
static GParamSpec* dia_list_item_properties[DIA_LIST_ITEM_NUM_PROPERTIES];

enum {
  DIA_LIST_ITEM_ACTIVATE_SIGNAL,
  DIA_LIST_ITEM_NUM_SIGNALS
};
static guint dia_list_item_signals[DIA_LIST_ITEM_NUM_SIGNALS] = {0};

typedef struct _Block1Data Block1Data;

struct _Block1Data {
  int _ref_count_;
  DiaList* self;
  gint i;
};

static void __lambda6_ (DiaList* self,
                 DiaListItem* item);
static void ___lambda6__gfunc (gpointer data,
                        gpointer self);
static void __dia_list___lambda8__gtk_list_box_row_selected (GtkListBox* _sender,
                                                      GtkListBoxRow* row,
                                                      gpointer self);
static void dia_list_get_property      (GObject * object,
                                        guint property_id,
                                        GValue * value,
                                        GParamSpec * pspec);
static void dia_list_set_property      (GObject * object,
                                        guint property_id,
                                        const GValue * value,
                                        GParamSpec * pspec);
static void dia_list_item_get_property (GObject * object,
                                        guint property_id,
                                        GValue * value,
                                        GParamSpec * pspec);
static void dia_list_item_set_property (GObject * object,
                                        guint property_id,
                                        const GValue * value,
                                        GParamSpec * pspec);
static void dia_list_finalize          (GObject * obj);
static void dia_list_item_finalize     (GObject * obj);

void
dia_list_select_child (DiaList* self,
                       DiaListItem* widget)
{
  GtkWidget *list;

  g_return_if_fail (self != NULL);
  g_return_if_fail (widget != NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_list_box_select_row (GTK_LIST_BOX (list), GTK_LIST_BOX_ROW (widget));
  g_signal_emit (self, dia_list_signals[DIA_LIST_CHILD_SELECTED_SIGNAL], 0, widget);
}

void
dia_list_unselect_child (DiaList* self,
                         DiaListItem* widget)
{
  GtkWidget *list;

  g_return_if_fail (self != NULL);
  g_return_if_fail (widget != NULL);
  
  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_list_box_unselect_row (GTK_LIST_BOX (list), GTK_LIST_BOX_ROW (widget));
}

static void
append_item (gpointer item,
             gpointer self)
{
  GtkWidget *list;

  g_return_if_fail (item != NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (DIA_LIST (self)))->real;
  gtk_container_add (GTK_CONTAINER (list), GTK_WIDGET (item));
}

void
dia_list_append_items (DiaList* self,
                       GList* items)
{
  g_return_if_fail (self != NULL);
  g_list_foreach (items, append_item, self);
}

void
dia_list_set_active (DiaList *self, gint index)
{
  GtkWidget *list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  GtkListBoxRow *row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list), index);

  gtk_list_box_select_row (GTK_LIST_BOX (list), row);
}

static void
insert_items (gpointer item,
              gpointer data)
{  
  DiaList* self;
  GtkWidget *list;
  gint pos;

  g_return_if_fail (item != NULL);
  g_return_if_fail (data != NULL);

  self = ((Block1Data *) data)->self;
  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  pos = ((Block1Data *) data)->i;
  ((Block1Data *) data)->i = pos + 1;
  gtk_list_box_insert (GTK_LIST_BOX (list), GTK_WIDGET (item), pos);
}

void
dia_list_insert_items (DiaList* self,
                       GList* items,
                       gint i)
{
  Block1Data* _data1_;
  g_return_if_fail (self != NULL);
  _data1_ = g_slice_new0 (Block1Data);
  _data1_->_ref_count_ = 1;
  _data1_->self = g_object_ref (self);
  _data1_->i = i;
  g_list_foreach (items, insert_items, _data1_);
  if (g_atomic_int_dec_and_test (&_data1_->_ref_count_)) {
    DiaList* self;
    self = _data1_->self;
    g_object_unref (self);
    g_slice_free (Block1Data, _data1_);
  }
  _data1_ = NULL;
}


static void
__lambda6_ (DiaList* self,
            DiaListItem* item)
{
  GtkWidget *list;

  g_return_if_fail (item != NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_container_remove (GTK_CONTAINER (list), (GtkWidget*) item);
}

static void
___lambda6__gfunc (gpointer data,
                   gpointer self)
{
  __lambda6_ ((DiaList*) self, (DiaListItem*) data);
}

void
dia_list_remove_items (DiaList* self,
                       GList* items)
{
  g_return_if_fail (self != NULL);
  g_list_foreach (items, ___lambda6__gfunc, self);
}

DiaListItem*
dia_list_append (DiaList     *self,
                 const gchar *item)
{
  GtkWidget* row;
  GtkWidget* list;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (item != NULL, NULL);

  row = dia_list_item_new_with_label (item);
  gtk_widget_show_all (row);
  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_container_add (GTK_CONTAINER (list), row);

  return DIA_LIST_ITEM (row);
}

void
dia_list_add (DiaList       *self,
              GtkListBoxRow *item)
{
  GtkWidget* list;

  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (item != NULL, NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_container_add (GTK_CONTAINER (list), item);
}

void
dia_list_add_seperator (DiaList *self)
{
  GtkWidget *sep;
  GtkWidget *list;

  g_return_if_fail (self != NULL);

  sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_show_all (sep);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_container_add (GTK_CONTAINER (list), sep);
}

void
dia_list_select_item (DiaList *self,
                      gint     i)
{
  GtkWidget *list;
  GtkListBoxRow *tmp;

  g_return_if_fail (self != NULL);

  list = ((DiaListPrivate *) ((DiaListPrivate *) dia_list_get_instance_private (self)))->real;
  tmp = gtk_list_box_get_row_at_index (GTK_LIST_BOX (list), i);
  dia_list_select_child (self, G_TYPE_CHECK_INSTANCE_TYPE (tmp, DIA_TYPE_LIST_ITEM) ? (DIA_LIST_ITEM (tmp)) : NULL);
}

static void
empty_widget (GtkWidget *elm,
              gpointer   self)
{
  GtkWidget *list;

  g_return_if_fail (elm != NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;

  gtk_container_remove (GTK_CONTAINER (list), elm);
}

void
dia_list_empty (DiaList *self)
{
  GtkWidget *list;

  g_return_if_fail (self != NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_container_foreach (GTK_CONTAINER (list), empty_widget, self);
}

gint
dia_list_child_position (DiaList     *self,
                         DiaListItem *row)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (row != NULL, 0);

  return gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (row));
}

void
dia_list_unselect_all (DiaList *self)
{
  GtkWidget *list;

  g_return_if_fail (self != NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_list_box_unselect_all (GTK_LIST_BOX (list));
}

void
dia_list_select_all (DiaList* self)
{
  GtkWidget *list;

  g_return_if_fail (self != NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_list_box_select_all (GTK_LIST_BOX (list));
}

GtkWidget *
dia_list_new (void)
{
  return g_object_new (DIA_TYPE_LIST, NULL);
}

GList*
dia_list_get_children (DiaList* self)
{
  GList* result;

  g_return_val_if_fail (self != NULL, NULL);

  result = gtk_container_get_children (GTK_CONTAINER (self));

  return result;
}

DiaListItem*
dia_list_get_selection (DiaList* self)
{
  GtkWidget *list;
  GtkListBoxRow *tmp;

  g_return_val_if_fail (self != NULL, NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  tmp = gtk_list_box_get_selected_row (GTK_LIST_BOX (list));
  return G_TYPE_CHECK_INSTANCE_TYPE (tmp, DIA_TYPE_LIST_ITEM) ? (DIA_LIST_ITEM (tmp)) : NULL;
}

GtkSelectionMode
dia_list_get_selection_mode (DiaList* self)
{
  GtkWidget *list;

  g_return_val_if_fail (self != NULL, 0);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;

  return gtk_list_box_get_selection_mode (GTK_LIST_BOX (list));
}

void
dia_list_set_selection_mode (DiaList* self,
                             GtkSelectionMode value)
{
  GtkWidget *list;

  g_return_if_fail (self != NULL);

  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), value);
  g_object_notify_by_pspec (G_OBJECT (self), dia_list_properties[DIA_LIST_SELECTION_MODE_PROPERTY]);
}

static void
__dia_list___lambda8__gtk_list_box_row_selected (GtkListBox* _sender,
                                                 GtkListBoxRow* row,
                                                 gpointer self)
{
  g_signal_emit (G_OBJECT (self), dia_list_signals[DIA_LIST_SELECTION_CHANGED_SIGNAL], 0);
  if (DIA_IS_LIST_ITEM (row)) {
    g_signal_emit (G_OBJECT (row), dia_list_item_signals[DIA_LIST_ITEM_ACTIVATE_SIGNAL], 0);
  }
}

static GObject *
dia_list_constructor (GType type,
                      guint n_construct_properties,
                      GObjectConstructParam * construct_properties)
{
  GObject * obj;
  GObjectClass * parent_class;
  DiaList * self;
  GtkWidget *list;

  parent_class = G_OBJECT_CLASS (dia_list_parent_class);
  obj = parent_class->constructor (type, n_construct_properties, construct_properties);
  self = G_TYPE_CHECK_INSTANCE_CAST (obj, DIA_TYPE_LIST, DiaList);
  list = ((DiaListPrivate *) dia_list_get_instance_private (self))->real;

  gtk_container_add (GTK_CONTAINER (self), list);
  g_signal_connect_object (list, "row-selected", (GCallback) __dia_list___lambda8__gtk_list_box_row_selected, self, 0);

  return obj;
}

static void
dia_list_class_init (DiaListClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = dia_list_get_property;
  object_class->set_property = dia_list_set_property;
  object_class->constructor = dia_list_constructor;
  object_class->finalize = dia_list_finalize;
  
  g_object_class_install_property (object_class, DIA_LIST_CHILDREN_PROPERTY, dia_list_properties[DIA_LIST_CHILDREN_PROPERTY] = g_param_spec_pointer ("children", "children", "children", G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));
  g_object_class_install_property (object_class, DIA_LIST_SELECTION_PROPERTY, dia_list_properties[DIA_LIST_SELECTION_PROPERTY] = g_param_spec_object ("selection", "selection", "selection", DIA_TYPE_LIST_ITEM, G_PARAM_STATIC_STRINGS | G_PARAM_READABLE));
  g_object_class_install_property (object_class, DIA_LIST_SELECTION_MODE_PROPERTY, dia_list_properties[DIA_LIST_SELECTION_MODE_PROPERTY] = g_param_spec_enum ("selection-mode", "selection-mode", "selection-mode", gtk_selection_mode_get_type (), 0, G_PARAM_STATIC_STRINGS | G_PARAM_READABLE | G_PARAM_WRITABLE));
  dia_list_signals[DIA_LIST_CHILD_SELECTED_SIGNAL] = g_signal_new ("child-selected", DIA_TYPE_LIST, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, DIA_TYPE_LIST_ITEM);
  dia_list_signals[DIA_LIST_SELECTION_CHANGED_SIGNAL] = g_signal_new ("selection-changed", DIA_TYPE_LIST, G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
dia_list_init (DiaList * self)
{
  ((DiaListPrivate *) dia_list_get_instance_private (self))->real = gtk_list_box_new ();
}

static void
dia_list_finalize (GObject * obj)
{
  DiaList * self;
  self = G_TYPE_CHECK_INSTANCE_CAST (obj, DIA_TYPE_LIST, DiaList);
  g_object_unref (((DiaListPrivate *) dia_list_get_instance_private (self))->real);
  G_OBJECT_CLASS (dia_list_parent_class)->finalize (obj);
}

static void
dia_list_get_property (GObject * object,
                       guint property_id,
                       GValue * value,
                       GParamSpec * pspec)
{
  DiaList * self;
  self = G_TYPE_CHECK_INSTANCE_CAST (object, DIA_TYPE_LIST, DiaList);
  switch (property_id) {
    case DIA_LIST_CHILDREN_PROPERTY:
      g_value_set_pointer (value, dia_list_get_children (self));
      break;
    case DIA_LIST_SELECTION_PROPERTY:
      g_value_set_object (value, dia_list_get_selection (self));
      break;
    case DIA_LIST_SELECTION_MODE_PROPERTY:
      g_value_set_enum (value, dia_list_get_selection_mode (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_list_set_property (GObject * object,
                       guint property_id,
                       const GValue * value,
                       GParamSpec * pspec)
{
  DiaList * self;
  self = G_TYPE_CHECK_INSTANCE_CAST (object, DIA_TYPE_LIST, DiaList);
  switch (property_id) {
    case DIA_LIST_SELECTION_MODE_PROPERTY:
      dia_list_set_selection_mode (self, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

GtkWidget *
dia_list_item_new_with_label (const gchar* lbl)
{
  return g_object_new (DIA_TYPE_LIST_ITEM,
                       "value", lbl,
                       NULL);
}

GtkWidget *
dia_list_item_new (void)
{
  return g_object_new (DIA_TYPE_LIST_ITEM, NULL);
}

const gchar*
dia_list_item_get_value (DiaListItem* self)
{
  const gchar *result;
  GtkWidget *label;

  g_return_val_if_fail (self != NULL, NULL);

  label = ((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label;
  result = gtk_label_get_label (GTK_LABEL (label));

  return result;
}

void
dia_list_item_set_value (DiaListItem* self,
                         const gchar* value)
{
  GtkWidget *label;

  g_return_if_fail (self != NULL);

  label = ((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label;
  gtk_label_set_label (GTK_LABEL (label), value);

  g_object_notify_by_pspec ((GObject *) self, dia_list_item_properties[DIA_LIST_ITEM_VALUE_PROPERTY]);
}


static GObject *
dia_list_item_constructor (GType type,
                           guint n_construct_properties,
                           GObjectConstructParam * construct_properties)
{
  GObject * obj;
  GObjectClass * parent_class;
  DiaListItem * self;
  GtkWidget *label;

  parent_class = G_OBJECT_CLASS (dia_list_item_parent_class);
  obj = parent_class->constructor (type, n_construct_properties, construct_properties);
  self = G_TYPE_CHECK_INSTANCE_CAST (obj, DIA_TYPE_LIST_ITEM, DiaListItem);
  label = ((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label;
  gtk_container_add (GTK_CONTAINER (self), label);
  return obj;
}

static void
dia_list_item_class_init (DiaListItemClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  object_class->get_property = dia_list_item_get_property;
  object_class->set_property = dia_list_item_set_property;
  object_class->constructor = dia_list_item_constructor;
  object_class->finalize = dia_list_item_finalize;
  g_object_class_install_property (object_class, DIA_LIST_ITEM_VALUE_PROPERTY, dia_list_item_properties[DIA_LIST_ITEM_VALUE_PROPERTY] = g_param_spec_string ("value", "value", "value", NULL, G_PARAM_STATIC_STRINGS | G_PARAM_READABLE | G_PARAM_WRITABLE));
  dia_list_item_signals[DIA_LIST_ITEM_ACTIVATE_SIGNAL] =
    g_signal_new ("activate",
                  DIA_TYPE_LIST, G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

static void
dia_list_item_init (DiaListItem *self)
{
  ((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label = g_object_new (GTK_TYPE_LABEL,
                                                                                            "xalign", 0.0,
                                                                                            NULL);
}

static void
dia_list_item_finalize (GObject *obj)
{
  DiaListItem * self;

  self = G_TYPE_CHECK_INSTANCE_CAST (obj, DIA_TYPE_LIST_ITEM, DiaListItem);

  g_object_unref (((DiaListItemPrivate *) dia_list_item_get_instance_private (self))->label);

  G_OBJECT_CLASS (dia_list_item_parent_class)->finalize (obj);
}

static void
dia_list_item_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  DiaListItem * self;
  self = G_TYPE_CHECK_INSTANCE_CAST (object, DIA_TYPE_LIST_ITEM, DiaListItem);
  switch (property_id) {
    case DIA_LIST_ITEM_VALUE_PROPERTY:
      g_value_set_string (value, dia_list_item_get_value (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_list_item_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  DiaListItem * self;
  self = G_TYPE_CHECK_INSTANCE_CAST (object, DIA_TYPE_LIST_ITEM, DiaListItem);
  switch (property_id) {
    case DIA_LIST_ITEM_VALUE_PROPERTY:
      dia_list_item_set_value (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}
