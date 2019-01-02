#include "dia-uml-list-data.h"
#include "dia-uml-list-row.h"
#include "dia_dirs.h"

G_DEFINE_TYPE (DiaUmlListRow, dia_uml_list_row, GTK_TYPE_LIST_BOX_ROW)

enum {
  PROP_DATA = 1,
  N_PROPS
};
static GParamSpec* properties[N_PROPS];

static void
dia_uml_list_row_finalize (GObject *object)
{
  DiaUmlListRow *self = DIA_UML_LIST_ROW (object);

  g_clear_object (&self->data);
}

static void
display_op (DiaUmlListData *op,
            DiaUmlListRow  *row)
{
  gtk_label_set_label (GTK_LABEL (row->title),
                       dia_uml_list_data_format (op));
}

static void
dia_uml_list_row_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  DiaUmlListRow *self = DIA_UML_LIST_ROW (object);
  switch (property_id) {
    case PROP_DATA:
      self->data = g_value_dup_object (value);
      g_signal_connect (G_OBJECT (self->data), "changed",
                        G_CALLBACK (display_op), self);
      display_op (self->data, self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_list_row_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  DiaUmlListRow *self = DIA_UML_LIST_ROW (object);
  switch (property_id) {
    case PROP_DATA:
      g_value_set_object (value, self->data);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
move_up (DiaUmlListRow *self)
{
  GtkWidget *list;
  int index;

  /*
   * If we ever find ourselves in something other than GtkListBox we are
   * in trouble but as a GtkListBoxRow that shouldn't happen, storing the
   * new state is left to the GtkListBox or it's owner
   */
  list = gtk_widget_get_parent (GTK_WIDGET (self));
  index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self));

  g_object_ref (self);
  gtk_container_remove (GTK_CONTAINER (list), GTK_WIDGET (self));
  gtk_list_box_insert (GTK_LIST_BOX (list), GTK_WIDGET (self), index - 1);
  g_object_unref (self);
}

static void
move_down (DiaUmlListRow *self)
{
  GtkWidget *list;
  int index;
  
  list = gtk_widget_get_parent (GTK_WIDGET (self));
  index = gtk_list_box_row_get_index (GTK_LIST_BOX_ROW (self));

  g_object_ref (self);
  gtk_container_remove (GTK_CONTAINER (list), GTK_WIDGET (self));
  gtk_list_box_insert (GTK_LIST_BOX (list), GTK_WIDGET (self), index + 1);
  g_object_unref (self);
}

static void
dia_uml_list_row_class_init (DiaUmlListRowClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = dia_uml_list_row_finalize;
  object_class->set_property = dia_uml_list_row_set_property;
  object_class->get_property = dia_uml_list_row_get_property;

  properties[PROP_DATA] =
    g_param_spec_object ("data",
                         "Data",
                         "Data this row represents",
                         DIA_UML_TYPE_LIST_DATA,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-uml-list-row.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlListRow, title);
  gtk_widget_class_bind_template_callback (widget_class, move_up);
  gtk_widget_class_bind_template_callback (widget_class, move_down);

  g_object_unref (template_file);
}

static void
dia_uml_list_row_init (DiaUmlListRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
dia_uml_list_row_new (DiaUmlListData *data)
{
  return g_object_new (DIA_UML_TYPE_LIST_ROW,
                       "data", data, 
                       NULL);
}

DiaUmlListData *
dia_uml_list_row_get_data (DiaUmlListRow *self)
{
  return g_object_ref (self->data);
}
