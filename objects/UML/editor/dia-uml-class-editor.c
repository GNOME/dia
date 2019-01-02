#include "dia-uml-class-editor.h"
#include "dia-uml-operation-row.h"
#include "dia-uml-operation-dialog.h"
#include "dia_dirs.h"

G_DEFINE_TYPE (DiaUmlClassEditor, dia_uml_class_editor, GTK_TYPE_SCROLLED_WINDOW)

enum {
  UML_CEDIT_PROP_CLASS = 1,
  UML_CEDIT_N_PROPS
};
static GParamSpec* uml_cedit_properties[UML_CEDIT_N_PROPS];

static void
build_list (DiaUmlClassEditor *self)
{
  GList *list;
  GtkWidget *item;

  gtk_container_foreach (GTK_CONTAINER (self->operations), (GtkCallback *) gtk_widget_destroy, NULL);

  list = dia_uml_class_get_operations (self->klass);
  self->building_ops = TRUE;
  while (list != NULL) {
    DiaUmlOperation *op = (DiaUmlOperation *)list->data;
    item = dia_uml_operation_row_new (op);
    gtk_widget_show (item);
    gtk_container_add (GTK_CONTAINER (self->operations), item);
    
    list = g_list_next(list);
  }
  self->building_ops = FALSE;
}

static void
remove_op_row (GtkWidget       *row,
               DiaUmlOperation *op)
{
  DiaUmlOperation *curr_row;

  curr_row = dia_uml_operation_row_get_operation (DIA_UML_OPERATION_ROW (row));

  if (op == curr_row)
    gtk_widget_destroy (row);

  g_object_unref (curr_row);
}

static void
remove_op (DiaUmlOperationDialog *dlg,
           DiaUmlOperation       *op,
           DiaUmlClassEditor     *self)
{
  gtk_container_foreach (GTK_CONTAINER (self->operations),
                         (GtkCallback *) remove_op_row,
                         op);
}

static void
add_operation (DiaUmlClassEditor *self)
{
  DiaUmlOperation *op;
  GtkWidget *row;
  GtkWidget *edit;
  GtkWidget *parent;

  op = dia_uml_operation_new ();
  row = dia_uml_operation_row_new (op);

  gtk_widget_show (row);
  gtk_container_add (GTK_CONTAINER (self->operations), row);

  parent = gtk_widget_get_toplevel (GTK_WIDGET (self));
  edit = dia_uml_operation_dialog_new (GTK_WINDOW (parent), op);
  g_signal_connect (edit, "operation-deleted", G_CALLBACK (remove_op), self);

  gtk_widget_show (edit);
}

static void
edit_operation (DiaUmlClassEditor *self,
                GtkListBoxRow     *row)
{
  GtkWidget *dlg;
  GtkWidget *parent;
  DiaUmlOperation *op;

  if (!DIA_UML_IS_OPERATION_ROW (row))
    return;
  
  parent = gtk_widget_get_toplevel (GTK_WIDGET (self));
  op = dia_uml_operation_row_get_operation (DIA_UML_OPERATION_ROW (row));
  dlg = dia_uml_operation_dialog_new (GTK_WINDOW (parent), op);
  g_signal_connect (dlg, "operation-deleted", G_CALLBACK (remove_op), self);
  gtk_widget_show (dlg);
  g_object_unref (op);
}

static void
operation_added (DiaUmlClassEditor *self,
                 GtkListBoxRow     *row)
{
  DiaUmlOperation *op;
  int index;

  if (self->building_ops || !DIA_UML_IS_OPERATION_ROW (row))
    return;

  op = dia_uml_operation_row_get_operation (DIA_UML_OPERATION_ROW (row));
  index = gtk_list_box_row_get_index (row);

  dia_uml_class_insert_operation (self->klass, op, index);
  g_object_unref (op);
}

static void
operation_removed (DiaUmlClassEditor *self,
                   GtkListBoxRow     *row)
{
  DiaUmlOperation *op;

  if (!DIA_UML_IS_OPERATION_ROW (row) || gtk_widget_in_destruction (GTK_WIDGET (row)))
    return;
  
  op = dia_uml_operation_row_get_operation (DIA_UML_OPERATION_ROW (row));

  dia_uml_class_remove_operation (self->klass, op);
  /* Don't unref op, we might be being moved so must give it the change to survive */
}

static void
dia_uml_class_editor_finalize (GObject *object)
{
  DiaUmlClassEditor *self = DIA_UML_CLASS_EDITOR (object);

  g_object_unref (self->klass);
}

static void
dia_uml_class_editor_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  DiaUmlClassEditor *self = DIA_UML_CLASS_EDITOR (object);

  switch (property_id) {
    case UML_CEDIT_PROP_CLASS:
      self->klass = g_value_dup_object (value);
      build_list (self);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_class_editor_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  DiaUmlClassEditor *self = DIA_UML_CLASS_EDITOR (object);
  switch (property_id) {
    case UML_CEDIT_PROP_CLASS:
      g_value_set_object (value, self->klass);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_class_editor_class_init (DiaUmlClassEditorClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = dia_uml_class_editor_set_property;
  object_class->get_property = dia_uml_class_editor_get_property;
  object_class->finalize = dia_uml_class_editor_finalize;

  uml_cedit_properties[UML_CEDIT_PROP_CLASS] =
    g_param_spec_object ("class",
                         "Class",
                         "Controlled class",
                         DIA_UML_TYPE_CLASS,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     UML_CEDIT_N_PROPS,
                                     uml_cedit_properties);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-uml-class-editor.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, attributes);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, operations);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, templates);
  gtk_widget_class_bind_template_callback (widget_class, add_operation);
  gtk_widget_class_bind_template_callback (widget_class, edit_operation);
  gtk_widget_class_bind_template_callback (widget_class, operation_added);
  gtk_widget_class_bind_template_callback (widget_class, operation_removed);

  g_object_unref (template_file);
}

static void
dia_uml_class_editor_init (DiaUmlClassEditor *self)
{
  GtkWidget *box;
  GtkWidget *label;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->building_ops = FALSE;

  box = g_object_new (GTK_TYPE_BOX,
                      "orientation", GTK_ORIENTATION_VERTICAL,
                      "spacing", 16,
                      "margin", 16,
                      NULL);
  gtk_widget_show (box);

  label = gtk_label_new ("No attributes");
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (box), label);

  gtk_list_box_set_placeholder (GTK_LIST_BOX (self->attributes), box);
  gtk_list_box_set_header_func (GTK_LIST_BOX (self->attributes), list_box_separators, NULL, NULL);

  box = g_object_new (GTK_TYPE_BOX,
                      "orientation", GTK_ORIENTATION_VERTICAL,
                      "spacing", 16,
                      "margin", 16,
                      NULL);
  gtk_widget_show (box);

  label = gtk_label_new ("No operations");
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (box), label);

  gtk_list_box_set_placeholder (GTK_LIST_BOX (self->operations), box);
  gtk_list_box_set_header_func (GTK_LIST_BOX (self->operations), list_box_separators, NULL, NULL);

  box = g_object_new (GTK_TYPE_BOX,
                      "orientation", GTK_ORIENTATION_VERTICAL,
                      "spacing", 16,
                      "margin", 16,
                      NULL);
  gtk_widget_show (box);

  label = gtk_label_new ("No templates");
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (box), label);

  gtk_list_box_set_placeholder (GTK_LIST_BOX (self->templates), box);
  gtk_list_box_set_header_func (GTK_LIST_BOX (self->templates), list_box_separators, NULL, NULL);
}

GtkWidget *
dia_uml_class_editor_new (DiaUmlClass *klass)
{
  return g_object_new (DIA_UML_TYPE_CLASS_EDITOR,
                       "class", klass,
                       NULL);
}

DiaUmlClass 
*dia_uml_class_editor_get_class (DiaUmlClassEditor *self)
{
  return self->klass;
}