#include "dia-uml-operation-dialog.h"
#include "dia-uml-operation-parameter-row.h"
#include "list/dia-list-store.h"
#include "dia_dirs.h"

G_DEFINE_TYPE (DiaUmlOperationDialog, dia_uml_operation_dialog, GTK_TYPE_DIALOG)

enum {
  UML_OP_DLG_PROP_OPERATION = 1,
  UML_OP_DLG_N_PROPS
};
static GParamSpec* uml_op_dlg_properties[UML_OP_DLG_N_PROPS];

enum {
  UML_OP_DLG_OPERATION_DELETED,
  UML_OP_DLG_LAST_SIGNAL
};
static guint uml_op_dlg_signals[UML_OP_DLG_LAST_SIGNAL] = { 0 };

static void
dia_uml_operation_dialog_finalize (GObject *object)
{
  DiaUmlOperationDialog *self = DIA_UML_OPERATION_DIALOG (object);

  g_object_unref (self->operation);
}

static gboolean
visibility_to (GBinding *binding,
               const GValue *from_value,
               GValue *to_value,
               gpointer user_data)
{
  const gchar *name = g_value_get_string (from_value);

  if (g_strcmp0 (name, "private") == 0) {
    g_value_set_int (to_value, UML_PRIVATE);
  } else if (g_strcmp0 (name, "protected") == 0) {
    g_value_set_int (to_value, UML_PROTECTED);
  } else if (g_strcmp0 (name, "implementation") == 0) {
    g_value_set_int (to_value, UML_IMPLEMENTATION);
  } else {
    g_value_set_int (to_value, UML_PUBLIC);
  }
  
  return TRUE;
}

static gboolean
visibility_from (GBinding *binding,
                 const GValue *from_value,
                 GValue *to_value,
                 gpointer user_data)
{
  int mode = g_value_get_int (from_value);

  if (mode == UML_PRIVATE) {
    g_value_set_static_string (to_value, "private");
  } else if (mode == UML_PROTECTED) {
    g_value_set_static_string (to_value, "protected");
  } else if (mode == UML_IMPLEMENTATION) {
    g_value_set_static_string (to_value, "implementation");
  } else {
    g_value_set_static_string (to_value, "public");
  }
  
  return TRUE;
}

static gboolean
inheritance_to (GBinding *binding,
                const GValue *from_value,
                GValue *to_value,
                gpointer user_data)
{
  const gchar *name = g_value_get_string (from_value);

  if (g_strcmp0 (name, "abstract") == 0) {
    g_value_set_int (to_value, UML_ABSTRACT);
  } else if (g_strcmp0 (name, "poly") == 0) {
    g_value_set_int (to_value, UML_POLYMORPHIC);
  } else {
    g_value_set_int (to_value, UML_LEAF);
  }
  
  return TRUE;
}

static gboolean
inheritance_from (GBinding *binding,
                  const GValue *from_value,
                  GValue *to_value,
                  gpointer user_data)
{
  int mode = g_value_get_int (from_value);

  if (mode == UML_ABSTRACT) {
    g_value_set_static_string (to_value, "abstract");
  } else if (mode == UML_POLYMORPHIC) {
    g_value_set_static_string (to_value, "poly");
  } else {
    g_value_set_static_string (to_value, "leaf");
  }
  
  return TRUE;}

static void
dia_uml_operation_dialog_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  DiaUmlOperationDialog *self = DIA_UML_OPERATION_DIALOG (object);
  GListModel *paras;

  switch (property_id) {
    case UML_OP_DLG_PROP_OPERATION:
      self->operation = g_value_dup_object (value);
      g_object_bind_property (G_OBJECT (self->operation), "name",
                              G_OBJECT (self->name), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->operation), "type",
                              G_OBJECT (self->type), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->operation), "stereotype",
                              G_OBJECT (self->stereotype), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->operation), "comment",
                              G_OBJECT (self->comment), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

      g_object_bind_property_full (G_OBJECT (self->operation), "visibility",
                                   G_OBJECT (self->visibility), "active-id",
                                   G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                                   visibility_from,
                                   visibility_to,
                                   NULL, NULL);

      g_object_bind_property_full (G_OBJECT (self->operation), "inheritance-type",
                                   G_OBJECT (self->inheritance), "active-id",
                                   G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                                   inheritance_from,
                                   inheritance_to,
                                   NULL, NULL);

      g_object_bind_property (G_OBJECT (self->operation), "query",
                              G_OBJECT (self->query), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->operation), "class-scope",
                              G_OBJECT (self->scope), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

      paras = dia_uml_operation_get_parameters (self->operation);
      g_message ("Got %i", g_list_model_get_n_items (paras));
      gtk_list_box_bind_model (GTK_LIST_BOX (self->list), paras,
                              (GtkListBoxCreateWidgetFunc) dia_uml_operation_parameter_row_new,
                              paras, NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_operation_dialog_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  DiaUmlOperationDialog *self = DIA_UML_OPERATION_DIALOG (object);
  switch (property_id) {
    case UML_OP_DLG_PROP_OPERATION:
      g_value_set_object (value, self->operation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dlg_add_parameter (DiaUmlOperationDialog *self)
{
  DiaUmlParameter *para = dia_uml_parameter_new ();

  dia_uml_operation_insert_parameter (self->operation, para, -1);
}

static void
remove_operation (DiaUmlOperationDialog *self)
{
  g_signal_emit (G_OBJECT (self),
                 uml_op_dlg_signals[UML_OP_DLG_OPERATION_DELETED], 0,
                 self->operation);
  gtk_widget_destroy (GTK_WIDGET (self));
}

static void
dia_uml_operation_dialog_class_init (DiaUmlOperationDialogClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = dia_uml_operation_dialog_finalize;
  object_class->set_property = dia_uml_operation_dialog_set_property;
  object_class->get_property = dia_uml_operation_dialog_get_property;

  uml_op_dlg_properties[UML_OP_DLG_PROP_OPERATION] =
    g_param_spec_object ("operation",
                         "Operation",
                         "Operation this editor controls",
                         DIA_UML_TYPE_OPERATION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     UML_OP_DLG_N_PROPS,
                                     uml_op_dlg_properties);

  uml_op_dlg_signals[UML_OP_DLG_OPERATION_DELETED] = g_signal_new ("operation-deleted",
                                                                   G_TYPE_FROM_CLASS (klass),
                                                                   G_SIGNAL_RUN_FIRST,
                                                                   0, NULL, NULL, NULL,
                                                                   G_TYPE_NONE, 1,
                                                                   DIA_UML_TYPE_OPERATION);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-uml-operation-dialog.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationDialog, name);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationDialog, type);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationDialog, stereotype);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationDialog, visibility);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationDialog, inheritance);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationDialog, scope);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationDialog, query);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationDialog, comment);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlOperationDialog, list);
  gtk_widget_class_bind_template_callback (widget_class, dlg_add_parameter);
  gtk_widget_class_bind_template_callback (widget_class, remove_operation);

  g_object_unref (template_file);
}

static void
dia_uml_operation_dialog_init (DiaUmlOperationDialog *self)
{
  GtkWidget *box;
  GtkWidget *widget;

  gtk_widget_init_template (GTK_WIDGET (self));

  box = g_object_new (GTK_TYPE_BOX,
                      "orientation", GTK_ORIENTATION_VERTICAL,
                      "spacing", 16,
                      "margin", 16,
                      NULL);
  gtk_widget_show (box);

  widget = gtk_label_new ("No parameters");
  gtk_widget_show (widget);
  gtk_container_add (GTK_CONTAINER (box), widget);

  gtk_list_box_set_placeholder (GTK_LIST_BOX (self->list), box);
  gtk_list_box_set_header_func (GTK_LIST_BOX (self->list), list_box_separators, NULL, NULL);
}

GtkWidget *
dia_uml_operation_dialog_new (GtkWindow       *parent,
                              DiaUmlOperation *op)
{
  return g_object_new (DIA_UML_TYPE_OPERATION_DIALOG,
                       "operation", op,
                       "transient-for", parent,
                       "modal", TRUE,
                       NULL);
}
