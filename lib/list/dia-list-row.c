#include "dia-list-data.h"
#include "dia-list-row.h"
#include "dia_dirs.h"

struct _DiaListRow {
  GtkListBoxRow parent;

  GtkWidget *title;

  DiaListData *data;
  DiaListStore *model;
};

G_DEFINE_TYPE (DiaListRow, dia_list_row, GTK_TYPE_LIST_BOX_ROW)

enum {
  PROP_DATA = 1,
  PROP_MODEL,
  N_PROPS
};
static GParamSpec* properties[N_PROPS];

static void
dia_list_row_finalize (GObject *object)
{
  DiaListRow *self = DIA_LIST_ROW (object);

  g_clear_object (&self->data);
  g_clear_object (&self->model);
}

static void
display_op (DiaListData *op,
            DiaListRow  *row)
{
  gtk_label_set_label (GTK_LABEL (row->title),
                       dia_list_data_format (op));
}

static void
dia_list_row_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  DiaListRow *self = DIA_LIST_ROW (object);
  switch (property_id) {
    case PROP_DATA:
      self->data = g_value_dup_object (value);
      g_signal_connect (G_OBJECT (self->data), "changed",
                        G_CALLBACK (display_op), self);
      display_op (self->data, self);
      break;
    case PROP_MODEL:
      self->model = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_list_row_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  DiaListRow *self = DIA_LIST_ROW (object);
  switch (property_id) {
    case PROP_DATA:
      g_value_set_object (value, self->data);
      break;
    case PROP_MODEL:
      g_value_set_object (value, self->model);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
move_up (DiaListRow *self)
{
  int index;

  index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self));

  g_object_ref (self->data);
  dia_list_store_remove (self->model, self->data);
  dia_list_store_insert (self->model, self->data, index - 1);
  g_object_unref (self->data);
}

static void
move_down (DiaListRow *self)
{
  int index;
  
  index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self));

  g_object_ref (self->data);
  dia_list_store_remove (self->model, self->data);
  dia_list_store_insert (self->model, self->data, index + 1);
  g_object_unref (self->data);
}

static void
dia_list_row_class_init (DiaListRowClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = dia_list_row_finalize;
  object_class->set_property = dia_list_row_set_property;
  object_class->get_property = dia_list_row_get_property;

  properties[PROP_DATA] =
    g_param_spec_object ("data",
                         "Data",
                         "Data this row represents",
                         DIA_TYPE_LIST_DATA,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  properties[PROP_MODEL] =
    g_param_spec_object ("model",
                         "Model",
                         "Model this row belongs to",
                         DIA_TYPE_LIST_STORE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-list-row.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child (widget_class, DiaListRow, title);
  gtk_widget_class_bind_template_callback (widget_class, move_up);
  gtk_widget_class_bind_template_callback (widget_class, move_down);

  g_object_unref (template_file);
}

static void
dia_list_row_init (DiaListRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
dia_list_row_new (DiaListData  *data,
                  DiaListStore *model)
{
  return g_object_new (DIA_TYPE_LIST_ROW,
                       "data", data, 
                       "model", model,
                       NULL);
}

DiaListData *
dia_list_row_get_data (DiaListRow *self)
{
  return g_object_ref (self->data);
}
