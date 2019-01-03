#include "dia-uml-class-editor.h"
#include "dia-uml-list-row.h"
#include "dia-uml-list-store.h"
#include "dia-uml-operation-dialog.h"
#include "dia-uml-attribute-dialog.h"
#include "dia-uml-formal-parameter-dialog.h"
#include "dia_dirs.h"

struct _DiaUmlClassEditor {
  GtkScrolledWindow parent;

  GtkWidget *name;
  GtkWidget *stereotype;
  GtkTextBuffer *comment;
  GtkWidget *abstract;
  GtkWidget *comments;
  GtkWidget *comments_wrap_point;
  GtkWidget *doc_tag;

  GtkWidget *attr_visible;
  GtkWidget *attr_suppress;
  GtkWidget *op_visible;
  GtkWidget *op_suppress;
  GtkWidget *op_wrap;
  GtkWidget *op_wrap_point;
  GtkWidget *is_template;

  GtkWidget *attributes;
  GtkWidget *operations;
  GtkWidget *templates;

  DiaUmlClass *klass;
};

G_DEFINE_TYPE (DiaUmlClassEditor, dia_uml_class_editor, GTK_TYPE_SCROLLED_WINDOW)

enum {
  UML_CEDIT_PROP_CLASS = 1,
  UML_CEDIT_N_PROPS
};
static GParamSpec* uml_cedit_properties[UML_CEDIT_N_PROPS];

static void
build_lists (DiaUmlClassEditor *self)
{
  GListModel *store;

  store = dia_uml_class_get_attributes (self->klass);
  gtk_list_box_bind_model (GTK_LIST_BOX (self->attributes), store,
                           (GtkListBoxCreateWidgetFunc) dia_uml_list_row_new,
                           store, NULL);

  store = dia_uml_class_get_operations (self->klass);
  gtk_list_box_bind_model (GTK_LIST_BOX (self->operations), store,
                           (GtkListBoxCreateWidgetFunc) dia_uml_list_row_new,
                           store, NULL);

  store = dia_uml_class_get_formal_parameters (self->klass);
  gtk_list_box_bind_model (GTK_LIST_BOX (self->templates), store,
                           (GtkListBoxCreateWidgetFunc) dia_uml_list_row_new,
                           store, NULL);
}

static void
remove_op (DiaUmlOperationDialog *dlg,
           DiaUmlOperation       *op,
           DiaUmlClassEditor     *self)
{
  dia_uml_class_remove_operation (self->klass, op);
}

static void
add_operation (DiaUmlClassEditor *self)
{
  DiaUmlOperation *op;
  GtkWidget *edit;
  GtkWidget *parent;

  op = dia_uml_operation_new ();
  dia_uml_class_insert_operation (self->klass, op, -1);

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

  if (!DIA_UML_IS_LIST_ROW (row))
    return;
  
  parent = gtk_widget_get_toplevel (GTK_WIDGET (self));
  op = DIA_UML_OPERATION (dia_uml_list_row_get_data (DIA_UML_LIST_ROW (row)));
  dlg = dia_uml_operation_dialog_new (GTK_WINDOW (parent), op);
  g_signal_connect (dlg, "operation-deleted", G_CALLBACK (remove_op), self);
  gtk_widget_show (dlg);
  g_object_unref (op);
}

static void
remove_attr (DiaUmlAttributeDialog *dlg,
             DiaUmlAttribute       *attr,
             DiaUmlClassEditor     *self)
{
  dia_uml_class_remove_attribute (self->klass, attr);
}

static void
add_attribute (DiaUmlClassEditor *self)
{
  DiaUmlAttribute *attr;
  GtkWidget *edit;
  GtkWidget *parent;

  attr = dia_uml_attribute_new ();
  dia_uml_class_insert_attribute (self->klass, attr, -1);

  parent = gtk_widget_get_toplevel (GTK_WIDGET (self));
  edit = dia_uml_attribute_dialog_new (GTK_WINDOW (parent), attr);
  g_signal_connect (edit, "attribute-deleted", G_CALLBACK (remove_attr), self);

  gtk_widget_show (edit);
}

static void
edit_attribute (DiaUmlClassEditor *self,
                GtkListBoxRow     *row)
{
  GtkWidget *dlg;
  GtkWidget *parent;
  DiaUmlAttribute *attr;

  if (!DIA_UML_IS_LIST_ROW (row))
    return;
  
  parent = gtk_widget_get_toplevel (GTK_WIDGET (self));
  attr = DIA_UML_ATTRIBUTE (dia_uml_list_row_get_data (DIA_UML_LIST_ROW (row)));
  dlg = dia_uml_attribute_dialog_new (GTK_WINDOW (parent), attr);
  g_signal_connect (dlg, "attribute-deleted", G_CALLBACK (remove_attr), self);
  gtk_widget_show (dlg);
  g_object_unref (attr);
}

static void
remove_template (DiaUmlFormalParameterDialog *dlg,
                 DiaUmlFormalParameter       *param,
                 DiaUmlClassEditor           *self)
{
  dia_uml_class_remove_formal_parameter (self->klass, param);
}

static void
add_template (DiaUmlClassEditor *self)
{
  DiaUmlFormalParameter *param;
  GtkWidget *edit;
  GtkWidget *parent;

  param = dia_uml_formal_parameter_new ();
  dia_uml_class_insert_formal_parameter (self->klass, param, -1);

  parent = gtk_widget_get_toplevel (GTK_WIDGET (self));
  edit = dia_uml_formal_parameter_dialog_new (GTK_WINDOW (parent), param);
  g_signal_connect (edit, "template-deleted", G_CALLBACK (remove_template), self);

  gtk_widget_show (edit);
}

static void
edit_template (DiaUmlClassEditor *self,
               GtkListBoxRow     *row)
{
  GtkWidget *dlg;
  GtkWidget *parent;
  DiaUmlFormalParameter *param;

  if (!DIA_UML_IS_LIST_ROW (row))
    return;
  
  parent = gtk_widget_get_toplevel (GTK_WIDGET (self));
  param = DIA_UML_FORMAL_PARAMETER (dia_uml_list_row_get_data (DIA_UML_LIST_ROW (row)));
  dlg = dia_uml_formal_parameter_dialog_new (GTK_WINDOW (parent), param);
  g_signal_connect (dlg, "template-deleted", G_CALLBACK (remove_template), self);
  gtk_widget_show (dlg);
  g_object_unref (param);
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
      g_object_bind_property (G_OBJECT (self->klass), "name",
                              G_OBJECT (self->name), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "stereotype",
                              G_OBJECT (self->stereotype), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "comment",
                              G_OBJECT (self->comment), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "abstract",
                              G_OBJECT (self->abstract), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "comments",
                              G_OBJECT (self->comments), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "comment-wrap-point",
                              G_OBJECT (self->comments_wrap_point), "value",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "documentation-tag",
                              G_OBJECT (self->doc_tag), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "attributes-visible",
                              G_OBJECT (self->attr_visible), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "attributes-suppress",
                              G_OBJECT (self->attr_suppress), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "operations-visible",
                              G_OBJECT (self->op_visible), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "operations-suppress",
                              G_OBJECT (self->op_suppress), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "operations-wrap",
                              G_OBJECT (self->op_wrap), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "operations-wrap-point",
                              G_OBJECT (self->op_wrap_point), "value",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->klass), "is-template",
                              G_OBJECT (self->is_template), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      build_lists (self);
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

  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, name);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, stereotype);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, comment);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, abstract);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, comments);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, comments_wrap_point);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, doc_tag);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, attr_visible);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, attr_suppress);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, op_visible);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, op_suppress);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, op_wrap);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, op_wrap_point);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, is_template);

  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, attributes);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, operations);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlClassEditor, templates);

  gtk_widget_class_bind_template_callback (widget_class, add_attribute);
  gtk_widget_class_bind_template_callback (widget_class, edit_attribute);
  gtk_widget_class_bind_template_callback (widget_class, add_operation);
  gtk_widget_class_bind_template_callback (widget_class, edit_operation);
  gtk_widget_class_bind_template_callback (widget_class, add_template);
  gtk_widget_class_bind_template_callback (widget_class, edit_template);

  g_object_unref (template_file);
}

static void
dia_uml_class_editor_init (DiaUmlClassEditor *self)
{
  GtkWidget *box;
  GtkWidget *label;

  gtk_widget_init_template (GTK_WIDGET (self));

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

DiaUmlClass *
dia_uml_class_editor_get_class (DiaUmlClassEditor *self)
{
  return self->klass;
}