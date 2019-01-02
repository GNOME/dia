#include <gtk/gtk.h>

#include "dia-uml-attribute-dialog.h"
#include "dia_dirs.h"
#include "uml.h"

G_DEFINE_TYPE (DiaUmlAttributeDialog, dia_uml_attribute_dialog, GTK_TYPE_DIALOG)

enum {
  PROP_ATTRIBUTE = 1,
  N_PROPS
};
static GParamSpec* properties[N_PROPS];

enum {
  ATTRIBUTE_DELETED,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static void
dia_uml_attribute_dialog_finalize (GObject *object)
{
  DiaUmlAttributeDialog *self = DIA_UML_ATTRIBUTE_DIALOG (object);

  g_object_unref (self->attribute);
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

static void
dia_uml_attribute_dialog_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  DiaUmlAttributeDialog *self = DIA_UML_ATTRIBUTE_DIALOG (object);

  switch (property_id) {
    case PROP_ATTRIBUTE:
      self->attribute = g_value_dup_object (value);
      g_object_bind_property (G_OBJECT (self->attribute), "name",
                              G_OBJECT (self->name), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->attribute), "type",
                              G_OBJECT (self->type), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->attribute), "value",
                              G_OBJECT (self->value), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->attribute), "comment",
                              G_OBJECT (self->comment), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

      g_object_bind_property_full (G_OBJECT (self->attribute), "visibility",
                                   G_OBJECT (self->visibility), "active-id",
                                   G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                                   visibility_from,
                                   visibility_to,
                                   NULL, NULL);

      g_object_bind_property (G_OBJECT (self->attribute), "class-scope",
                              G_OBJECT (self->scope), "active",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_attribute_dialog_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  DiaUmlAttributeDialog *self = DIA_UML_ATTRIBUTE_DIALOG (object);
  switch (property_id) {
    case PROP_ATTRIBUTE:
      g_value_set_object (value, self->attribute);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
remove_attribute (DiaUmlAttributeDialog *self)
{
  g_signal_emit (G_OBJECT (self),
                 signals[ATTRIBUTE_DELETED], 0,
                 self->attribute);
  gtk_widget_destroy (GTK_WIDGET (self));
}

static void
dia_uml_attribute_dialog_class_init (DiaUmlAttributeDialogClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = dia_uml_attribute_dialog_finalize;
  object_class->set_property = dia_uml_attribute_dialog_set_property;
  object_class->get_property = dia_uml_attribute_dialog_get_property;

  properties[PROP_ATTRIBUTE] =
    g_param_spec_object ("attribute",
                         "Attribute",
                         "Attribute this editor controls",
                         DIA_UML_TYPE_ATTRIBUTE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);

  signals[ATTRIBUTE_DELETED] = g_signal_new ("attribute-deleted",
                                             G_TYPE_FROM_CLASS (klass),
                                             G_SIGNAL_RUN_FIRST,
                                             0, NULL, NULL, NULL,
                                             G_TYPE_NONE, 1,
                                             DIA_UML_TYPE_ATTRIBUTE);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-uml-attribute-dialog.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlAttributeDialog, name);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlAttributeDialog, type);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlAttributeDialog, value);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlAttributeDialog, comment);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlAttributeDialog, visibility);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlAttributeDialog, scope);
  gtk_widget_class_bind_template_callback (widget_class, remove_attribute);

  g_object_unref (template_file);
}

static void
dia_uml_attribute_dialog_init (DiaUmlAttributeDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
dia_uml_attribute_dialog_new (GtkWindow       *parent,
                              DiaUmlAttribute *attr)
{
  return g_object_new (DIA_UML_TYPE_ATTRIBUTE_DIALOG,
                       "attribute", attr,
                       "transient-for", parent,
                       "modal", TRUE,
                       NULL);
}
