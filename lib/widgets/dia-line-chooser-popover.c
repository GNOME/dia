#include <glib.h>
#include <gtk/gtk.h>

#include "dia-line-chooser.h"
#include "dia-line-chooser-popover.h"
#include "dia_dirs.h"

typedef struct _DiaLineChooserPopoverPrivate DiaLineChooserPopoverPrivate;

struct _DiaLineChooserPopoverPrivate {
 GtkWidget *list;
 GtkWidget *length;
 GtkWidget *length_box;
};

G_DEFINE_TYPE_WITH_CODE (DiaLineChooserPopover, dia_line_chooser_popover, GTK_TYPE_POPOVER,
                         G_ADD_PRIVATE (DiaLineChooserPopover))

enum {
  VALUE_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GtkWidget *
dia_line_chooser_popover_new ()
{
  return g_object_new (DIA_TYPE_LINE_CHOOSER_POPOVER, NULL);
}

LineStyle
dia_line_chooser_popover_get_line_style (DiaLineChooserPopover *self,
                                         gdouble               *length)
{
  DiaLineChooserPopoverPrivate *priv = dia_line_chooser_popover_get_instance_private (self);
  GtkListBoxRow *row;
  GtkWidget *preview;
  LineStyle style;

  row = gtk_list_box_get_selected_row (GTK_LIST_BOX (priv->list));
  preview = gtk_bin_get_child (GTK_BIN (row));
  style = dia_line_preview_get_line_style (DIA_LINE_PREVIEW (preview));

  /* Don't both getting this if we don't have somewhere to return it */
  if (length) {
    *length = gtk_spin_button_get_value (GTK_SPIN_BUTTON (priv->length));
  }

  return style;
}

void
dia_line_chooser_popover_set_line_style (DiaLineChooserPopover *self,
                                         LineStyle              line_style)
{
  DiaLineChooserPopoverPrivate *priv = dia_line_chooser_popover_get_instance_private (self);
  GtkListBoxRow *row;

  row = gtk_list_box_get_row_at_index (GTK_LIST_BOX (priv->list), line_style);
  gtk_list_box_select_row (GTK_LIST_BOX (priv->list), row);
}

void
dia_line_chooser_popover_set_length (DiaLineChooserPopover *self,
                                     gdouble                length)
{
  DiaLineChooserPopoverPrivate *priv = dia_line_chooser_popover_get_instance_private (self);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->length), length);
}

static void
dia_line_chooser_popover_class_init (DiaLineChooserPopoverClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  signals[VALUE_CHANGED] = g_signal_new ("value-changed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_FIRST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE, 0);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-line-chooser-popover.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child_private (widget_class, DiaLineChooserPopover, list);
  gtk_widget_class_bind_template_child_private (widget_class, DiaLineChooserPopover, length);
  gtk_widget_class_bind_template_child_private (widget_class, DiaLineChooserPopover, length_box);

  g_object_unref (template_file);
}

static void
spin_change (GtkSpinButton *sb, gpointer data)
{
  g_signal_emit (G_OBJECT (data),
                 signals[VALUE_CHANGED], 0);
}

static void
row_selected (GtkListBox            *box,
              GtkListBoxRow         *row,
              DiaLineChooserPopover *self)
{
  DiaLineChooserPopoverPrivate *priv = dia_line_chooser_popover_get_instance_private (self);

  int state;
  state = dia_line_chooser_popover_get_line_style (self, NULL)
     != LINESTYLE_SOLID;

  gtk_widget_set_sensitive (priv->length_box, state);
  g_signal_emit (G_OBJECT (self),
                 signals[VALUE_CHANGED], 0);
}

static void
dia_line_chooser_popover_init (DiaLineChooserPopover *self)
{
  DiaLineChooserPopoverPrivate *priv = dia_line_chooser_popover_get_instance_private (self);

  gtk_widget_init_template (GTK_WIDGET (self));

  g_object_set (G_OBJECT (priv->length),
                "climb-rate", DEFAULT_LINESTYLE_DASHLEN,
                NULL);
  g_signal_connect (G_OBJECT (priv->length), "changed",
                    G_CALLBACK (spin_change), self);

  for (int i = 0; i <= LINESTYLE_DOTTED; i++) {
    GtkWidget *style = dia_line_preview_new (i);
    gtk_widget_show (style);
    gtk_list_box_insert (GTK_LIST_BOX (priv->list), style, i);
  }

  g_signal_connect (G_OBJECT (priv->list), "row-selected",
                    G_CALLBACK (row_selected), self);
}
