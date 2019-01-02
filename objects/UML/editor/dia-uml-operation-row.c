#include "dia-uml-operation-row.h"
#include "dia_dirs.h"

G_DEFINE_TYPE (DiaUmlOperationRow, dia_uml_operation_row, GTK_TYPE_LIST_BOX_ROW)

enum {
  UML_OP_PROW_PROP_OPERATION = 1,
  UML_OP_PROW_N_PROPS
};
static GParamSpec* uml_op_prow_properties[UML_OP_PROW_N_PROPS];

static void
dia_uml_operation_row_finalize (GObject *object)
{
  DiaUmlOperationRow *self = DIA_UML_OPERATION_ROW (object);

  G_DEBUG_HERE();

  g_object_unref (self->operation);
}

static void
display_op (DiaUmlOperation    *op,
            DiaUmlOperationRow *row)
{
  gtk_label_set_label (GTK_LABEL (row->title),
                       dia_uml_operation_format (op));
}

static void
dia_uml_operation_row_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  DiaUmlOperationRow *self = DIA_UML_OPERATION_ROW (object);
  switch (property_id) {
    case UML_OP_PROW_PROP_OPERATION:
      self->operation = g_value_dup_object (value);
      g_signal_connect (G_OBJECT (self->operation), "changed",
                        G_CALLBACK (display_op), self);
      display_op (self->operation, self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_operation_row_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  DiaUmlOperationRow *self = DIA_UML_OPERATION_ROW (object);
  switch (property_id) {
    case UML_OP_PROW_PROP_OPERATION:
      g_value_set_object (value, self->operation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
move_up (DiaUmlOperationRow *self)
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
move_down (DiaUmlOperationRow *self)
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
dia_uml_operation_row_class_init (DiaUmlOperationRowClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = dia_uml_operation_row_finalize;
  object_class->set_property = dia_uml_operation_row_set_property;
  object_class->get_property = dia_uml_operation_row_get_property;

  uml_op_prow_properties[UML_OP_PROW_PROP_OPERATION] =
    g_param_spec_object ("operation",
                         "Operation",
                         "Operation this editor controls",
                         DIA_UML_TYPE_OPERATION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     UML_OP_PROW_N_PROPS,
                                     uml_op_prow_properties);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-uml-operation-row.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationRow, title);
  gtk_widget_class_bind_template_callback (widget_class, move_up);
  gtk_widget_class_bind_template_callback (widget_class, move_down);

  g_object_unref (template_file);
}

static void
dia_uml_operation_row_init (DiaUmlOperationRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
dia_uml_operation_row_new (DiaUmlOperation *operation)
{
  return g_object_new (DIA_UML_TYPE_OPERATION_ROW,
                       "operation", operation, 
                       NULL);
}

DiaUmlOperation *
dia_uml_operation_row_get_operation (DiaUmlOperationRow *self)
{
  return g_object_ref (self->operation);
}