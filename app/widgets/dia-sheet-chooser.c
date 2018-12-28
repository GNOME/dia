#include <gtk/gtk.h>

#include "dia-sheet-chooser.h"
#include "dia_dirs.h"

G_DEFINE_TYPE (DiaSheetChooserPopover, dia_sheet_chooser_popover, GTK_TYPE_POPOVER)

enum {
  SHEET_SELECTED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };
static guint popover_signals[LAST_SIGNAL] = { 0 };

static void
dia_sheet_chooser_popover_class_init (DiaSheetChooserPopoverClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  popover_signals[SHEET_SELECTED] = g_signal_new ("sheet-selected",
                                                  G_TYPE_FROM_CLASS (klass),
                                                  G_SIGNAL_RUN_FIRST,
                                                  0, NULL, NULL, NULL,
                                                  G_TYPE_NONE, 1,
                                                  DIA_TYPE_SHEET);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-sheet-chooser-popover.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child (widget_class, DiaSheetChooserPopover, list);
  gtk_widget_class_bind_template_child (widget_class, DiaSheetChooserPopover, filter);

  g_object_unref (template_file);
}

static GtkWidget *
render_row (gpointer item, gpointer user_data)
{
  GtkWidget *row;
  GtkWidget *box;
  GtkWidget *name;
  GtkWidget *desc;

  row = gtk_list_box_row_new ();
  /* TODO: Let's avoid the trap of set_data */
  g_object_set_data (G_OBJECT (row), "dia-sheet", item);
  g_object_set_data (G_OBJECT (row),
                     "dia-list-top",
                     g_object_get_data (G_OBJECT (item), "dia-list-top"));
  gtk_widget_set_tooltip_markup (row,
                                 g_strdup_printf ("<b>%s</b>\n%s",
                                                  DIA_SHEET (item)->name,
                                                  DIA_SHEET (item)->description));
  gtk_widget_show (row);

  box = g_object_new (GTK_TYPE_BOX,
                      "orientation", GTK_ORIENTATION_VERTICAL,
                      "spacing", 2,
                      "margin", 4,
                      NULL);
  gtk_container_add (GTK_CONTAINER (row), box);
  gtk_widget_show (box);

  name = g_object_new (GTK_TYPE_LABEL,
                       "label", DIA_SHEET (item)->name,
                       "xalign", 0.0,
                       "ellipsize", PANGO_ELLIPSIZE_END,
                       "max-width-chars", 25,
                       NULL);
  gtk_widget_show (name);
  gtk_box_pack_start (GTK_BOX (box), name, FALSE, FALSE, 0);

  desc = g_object_new (GTK_TYPE_LABEL,
                       "label", DIA_SHEET (item)->description,
                       "xalign", 0.0,
                       "ellipsize", PANGO_ELLIPSIZE_END,
                       "max-width-chars", 25,
                       NULL);
  gtk_style_context_add_class (gtk_widget_get_style_context (desc),
                               GTK_STYLE_CLASS_DIM_LABEL);
  gtk_widget_show (desc);
  gtk_box_pack_start (GTK_BOX (box), desc, FALSE, FALSE, 0);

  return row;
}

static void
header_func (GtkListBoxRow *row,
             GtkListBoxRow *before,
             gpointer user_data)
{
  gboolean last = FALSE;
  gboolean this = FALSE;

  if (before)
    last = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (before), "dia-list-top"));
  this = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (row), "dia-list-top"));

  if (last && !this) {
    GtkWidget *sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show (sep);
    gtk_list_box_row_set_header (row, sep);
  }
}

static void
sheet_selected (GtkListBox             *box,
                GtkListBoxRow          *row,
                DiaSheetChooserPopover *self)
{
  DiaSheet *sheet = g_object_get_data (G_OBJECT (row), "dia-sheet");

  g_signal_emit_by_name (G_OBJECT (self), "sheet-selected", sheet);
}

static void
sheet_activated (GtkListBox             *box,
                 GtkListBoxRow          *row,
                 DiaSheetChooserPopover *self)
{
  gtk_popover_popdown (GTK_POPOVER (self));
}

static void
dia_sheet_chooser_popover_init (DiaSheetChooserPopover *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_list_box_set_header_func (GTK_LIST_BOX (self->list),
                                header_func, NULL, NULL);

  g_signal_connect (G_OBJECT (self->list), "row-selected",
                    G_CALLBACK (sheet_selected), self);
  g_signal_connect (G_OBJECT (self->list), "row-activated",
                    G_CALLBACK (sheet_activated), self);
}

void
dia_sheet_chooser_popover_set_model (DiaSheetChooserPopover *self,
                                     GListModel             *model)
{
  gtk_list_box_bind_model (GTK_LIST_BOX (self->list),
                           G_LIST_MODEL (model),
                           render_row, NULL, NULL);
}

GtkWidget *
dia_sheet_chooser_popover_new ()
{
  return g_object_new (DIA_TYPE_SHEET_CHOOSER_POPOVER, NULL);
}

G_DEFINE_TYPE (DiaSheetChooser, dia_sheet_chooser, GTK_TYPE_MENU_BUTTON)

static void
propagate (DiaSheetChooserPopover *chooser,
           DiaSheet               *sheet,
           DiaSheetChooser        *self)
{
  gtk_label_set_label (GTK_LABEL (self->label), sheet->name);

  g_signal_emit_by_name (G_OBJECT (self), "sheet-selected", sheet);
}

static void
dia_sheet_chooser_class_init (DiaSheetChooserClass *klass)
{
  signals[SHEET_SELECTED] = g_signal_new ("sheet-selected",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_FIRST,
                                          0, NULL, NULL, NULL,
                                          G_TYPE_NONE, 1,
                                          DIA_TYPE_SHEET);
}

static void
dia_sheet_chooser_init (DiaSheetChooser *self)
{
  GtkWidget *box;
  GtkWidget *arrow;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 16);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (self), box);

  self->label = g_object_new (GTK_TYPE_LABEL,
                              "label", "[NONE]",
                              "ellipsize", PANGO_ELLIPSIZE_END,
                              "max-width-chars", 14,
                              "xalign", 0.0,
                              NULL);
  gtk_widget_show (self->label);
  gtk_box_pack_start (GTK_BOX (box), self->label, TRUE, TRUE, 0);

  arrow = gtk_image_new_from_icon_name ("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (arrow);
  gtk_box_pack_end (GTK_BOX (box), arrow, FALSE, FALSE, 0);

  self->popover = dia_sheet_chooser_popover_new ();
  g_signal_connect (G_OBJECT (self->popover), "sheet-selected",
                    G_CALLBACK (propagate), self);
  gtk_menu_button_set_popover (GTK_MENU_BUTTON (self), self->popover);
}

void
dia_sheet_chooser_set_model (DiaSheetChooser *self,
                             GListModel      *model)
{
  dia_sheet_chooser_popover_set_model (DIA_SHEET_CHOOSER_POPOVER (self->popover),
                                       model);
}

GtkWidget *
dia_sheet_chooser_new ()
{
  return g_object_new (DIA_TYPE_SHEET_CHOOSER, NULL);
}
