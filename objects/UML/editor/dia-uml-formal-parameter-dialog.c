#include <gtk/gtk.h>

#include "dia-uml-formal-parameter-dialog.h"
#include "dia_dirs.h"
#include "uml.h"

G_DEFINE_TYPE (DiaUmlFormalParameterDialog, dia_uml_formal_parameter_dialog, GTK_TYPE_DIALOG)

enum {
  PROP_FORMAL_PARAMETER = 1,
  N_PROPS
};
static GParamSpec* properties[N_PROPS];

enum {
  TEMPLATE_DELETED,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static void
dia_uml_formal_parameter_dialog_finalize (GObject *object)
{
  DiaUmlFormalParameterDialog *self = DIA_UML_FORMAL_PARAMETER_DIALOG (object);

  g_object_unref (self->parameter);
}

static void
dia_uml_formal_parameter_dialog_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  DiaUmlFormalParameterDialog *self = DIA_UML_FORMAL_PARAMETER_DIALOG (object);

  switch (property_id) {
    case PROP_FORMAL_PARAMETER:
      self->parameter = g_value_dup_object (value);
      g_object_bind_property (G_OBJECT (self->parameter), "name",
                              G_OBJECT (self->name), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      g_object_bind_property (G_OBJECT (self->parameter), "type",
                              G_OBJECT (self->type), "text",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_formal_parameter_dialog_get_property (GObject    *object,
                                              guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  DiaUmlFormalParameterDialog *self = DIA_UML_FORMAL_PARAMETER_DIALOG (object);
  switch (property_id) {
    case PROP_FORMAL_PARAMETER:
      g_value_set_object (value, self->parameter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
remove_template (DiaUmlFormalParameterDialog *self)
{
  g_signal_emit (G_OBJECT (self),
                 signals[TEMPLATE_DELETED], 0,
                 self->parameter);
  gtk_widget_destroy (GTK_WIDGET (self));
}

static void
dia_uml_formal_parameter_dialog_class_init (DiaUmlFormalParameterDialogClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = dia_uml_formal_parameter_dialog_finalize;
  object_class->set_property = dia_uml_formal_parameter_dialog_set_property;
  object_class->get_property = dia_uml_formal_parameter_dialog_get_property;

  properties[PROP_FORMAL_PARAMETER] =
    g_param_spec_object ("formal-parameter",
                         "Formal parameter",
                         "Formal parameter this editor controls",
                         DIA_UML_TYPE_FORMAL_PARAMETER,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);

  signals[TEMPLATE_DELETED] = g_signal_new ("template-deleted",
                                            G_TYPE_FROM_CLASS (klass),
                                            G_SIGNAL_RUN_FIRST,
                                            0, NULL, NULL, NULL,
                                            G_TYPE_NONE, 1,
                                            DIA_UML_TYPE_FORMAL_PARAMETER);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-uml-formal-parameter-dialog.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlFormalParameterDialog, name);
  gtk_widget_class_bind_template_child (widget_class, DiaUmlFormalParameterDialog, type);
  gtk_widget_class_bind_template_callback (widget_class, remove_template);

  g_object_unref (template_file);
}

static void
dia_uml_formal_parameter_dialog_init (DiaUmlFormalParameterDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
dia_uml_formal_parameter_dialog_new (GtkWindow             *parent,
                                     DiaUmlFormalParameter *param)
{
  return g_object_new (DIA_UML_TYPE_FORMAL_PARAMETER_DIALOG,
                       "formal-parameter", param,
                       "transient-for", parent,
                       "modal", TRUE,
                       NULL);
}
