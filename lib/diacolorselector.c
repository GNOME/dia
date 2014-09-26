/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <config.h>

#include <gtk/gtk.h>

#include "intl.h"
#include "diadynamicmenu.h"
#include "persistence.h"
#include "widgets.h"

/************* DiaColorSelector: ***************/
struct _DiaColorSelector
{
  GtkHBox         hbox; /* just contaning the other two widgets */
  DiaDynamicMenu *ddm; /* the widget previously alone */
  GtkColorButton *color_button; /* to reflect alpha */
  gboolean        use_alpha;
};
struct _DiaColorSelectorClass
{
  GtkHBoxClass parent_class;
};
enum {
  DIA_COLORSEL_VALUE_CHANGED,
  DIA_COLORSEL_LAST_SIGNAL
};
static guint dia_colorsel_signals[DIA_COLORSEL_LAST_SIGNAL] = { 0 };

static GtkWidget *dia_color_selector_menu_new (DiaColorSelector *cs);

static void
dia_color_selector_class_init (DiaColorSelector *class)
{
  dia_colorsel_signals[DIA_COLORSEL_VALUE_CHANGED]
      = g_signal_new("value_changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}
static void
dia_color_selector_color_set (GtkColorButton *button, gpointer user_data)
{
  DiaColorSelector *cs = DIACOLORSELECTOR(user_data);
  gchar *entry;
  GdkColor gcol;

  gtk_color_button_get_color (button, &gcol);

  entry = g_strdup_printf("#%02X%02X%02X", gcol.red/256, gcol.green/256, gcol.blue/256);
  dia_dynamic_menu_select_entry(cs->ddm, entry);
  g_free(entry);

  g_signal_emit (G_OBJECT (cs), dia_colorsel_signals[DIA_COLORSEL_VALUE_CHANGED], 0);
}
static void
dia_color_selector_value_changed (DiaDynamicMenu *ddm, gpointer user_data)
{
  DiaColorSelector *cs = DIACOLORSELECTOR(user_data);
  gchar *entry = dia_dynamic_menu_get_entry(cs->ddm);
  GdkColor gcol;

  gdk_color_parse (entry, &gcol);
  g_free(entry);
  gtk_color_button_set_color (cs->color_button, &gcol);

  g_signal_emit (G_OBJECT (cs), dia_colorsel_signals[DIA_COLORSEL_VALUE_CHANGED], 0);
}
static void
dia_color_selector_init (DiaColorSelector *cs)
{
  cs->ddm = DIA_DYNAMIC_MENU(dia_color_selector_menu_new(cs));
  cs->color_button = GTK_COLOR_BUTTON (gtk_color_button_new ());
  
  gtk_widget_show (GTK_WIDGET (cs->ddm));

  /* default off */
  gtk_color_button_set_use_alpha (cs->color_button, cs->use_alpha);
  /* delegate color changes to compound object */
  g_signal_connect (G_OBJECT (cs->color_button), "color-set", 
		    G_CALLBACK (dia_color_selector_color_set), cs);
  /* listen to menu selction to update the button */
  g_signal_connect (G_OBJECT (cs->ddm), "value-changed",
		    G_CALLBACK (dia_color_selector_value_changed), cs);

  if (cs->use_alpha)
    gtk_widget_show (GTK_WIDGET (cs->color_button));

  gtk_box_pack_start(GTK_BOX(cs), GTK_WIDGET(cs->ddm), TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(cs), GTK_WIDGET(cs->color_button), TRUE, TRUE, 0);
}
void
dia_color_selector_set_use_alpha (GtkWidget *widget, gboolean use_alpha)
{
  DiaColorSelector *cs = DIACOLORSELECTOR(widget);

  if (use_alpha)
    gtk_widget_show (GTK_WIDGET (cs->color_button));
  else
    gtk_widget_hide (GTK_WIDGET (cs->color_button));
  cs->use_alpha = use_alpha;
  gtk_color_button_set_use_alpha (cs->color_button, cs->use_alpha);
}
GType
dia_color_selector_get_type (void)
{
  static GType dcs_type = 0;

  if (!dcs_type) {
    static const GTypeInfo dcs_info = {
      sizeof (DiaColorSelectorClass),
      (GBaseInitFunc)NULL,
      (GBaseFinalizeFunc)NULL,
      (GClassInitFunc) dia_color_selector_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (DiaColorSelector),
      0, /* n_preallocs */
      (GInstanceInitFunc) dia_color_selector_init
    };
    
    dcs_type = g_type_register_static (gtk_hbox_get_type (),
				       "DiaColorSelector", 
				       &dcs_info, 0);
  }
  
  return dcs_type;
}

static GtkWidget *
dia_color_selector_create_string_item(DiaDynamicMenu *ddm, gchar *string)
{
  GtkWidget *item = gtk_menu_item_new_with_label(string);
  gint r, g, b, a;
  gchar *markup;
  
  sscanf(string, "#%2x%2x%2x%2x", &r, &g, &b, &a);

  markup = g_strdup_printf("#%02X%02X%02X", r, g, b);

  /* See http://web.umr.edu/~rhall/commentary/color_readability.htm for
   * explanation of this formula */
  if (r*299+g*587+b*114 > 500 * 256) {
    gchar *label = g_strdup_printf("<span foreground=\"black\" background=\"%s\">%s</span>", markup, string);
    gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), label);
    g_free(label);
  } else {
    gchar *label = g_strdup_printf("<span foreground=\"white\" background=\"%s\">%s</span>", markup, string);
    gtk_label_set_markup(GTK_LABEL(gtk_bin_get_child(GTK_BIN(item))), label);
    g_free(label);
  }
  
  g_free(markup);
  return item;
}

static void
dia_color_selector_more_ok(GtkWidget *ok, gpointer userdata)
{
  DiaColorSelector *cs = g_object_get_data(G_OBJECT(userdata), "dia-cs");
  GtkWidget *colorsel = GTK_WIDGET(userdata);
  GdkColor gcol;
  guint galpha;
  gchar *entry;
  GtkWidget *cs2 = gtk_color_selection_dialog_get_color_selection (GTK_COLOR_SELECTION_DIALOG(colorsel));

  gtk_color_selection_get_current_color(
	GTK_COLOR_SELECTION(cs2),
	&gcol);

  galpha = gtk_color_selection_get_current_alpha(
        GTK_COLOR_SELECTION(cs2));

  entry = g_strdup_printf("#%02X%02X%02X", gcol.red/256, gcol.green/256, gcol.blue/256);
  dia_dynamic_menu_select_entry(cs->ddm, entry);
  g_free(entry);
  /* update color button */
  gtk_color_button_set_color (cs->color_button, &gcol);
  gtk_color_button_set_alpha (cs->color_button, galpha);
  /* not destroying colorsel */
}

static void
dia_color_selector_more_callback(GtkWidget *widget, gpointer userdata)
{
  GtkColorSelectionDialog *dialog = GTK_COLOR_SELECTION_DIALOG (gtk_color_selection_dialog_new(_("Select color")));
  DiaColorSelector *cs = DIACOLORSELECTOR(userdata);
  GtkColorSelection *colorsel = GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection (dialog));
  GString *palette = g_string_new ("");
  GtkWidget *button;

  gchar *old_color = dia_dynamic_menu_get_entry(cs->ddm);
  GtkWidget *parent;

  gtk_color_selection_set_has_opacity_control(colorsel, cs->use_alpha);

  if (cs->use_alpha) {
    gtk_color_selection_set_previous_alpha (colorsel, 65535);
    gtk_color_selection_set_current_alpha (colorsel, gtk_color_button_get_alpha (cs->color_button));
  }
  /* Force history to the old place */
  dia_dynamic_menu_select_entry(cs->ddm, old_color);

  /* avoid crashing if the property dialog is closed before the color dialog */
  parent = widget;
  while (parent && !GTK_IS_WINDOW (parent))
    parent = gtk_widget_get_parent (parent);
  if (parent) {
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW(parent));
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  }

  if (dia_dynamic_menu_get_default_entries(cs->ddm) != NULL) {
    GList *tmplist;
    int index = 0;
    gboolean advance = TRUE;

    for (tmplist = dia_dynamic_menu_get_default_entries(cs->ddm); 
         tmplist != NULL || advance; 
         tmplist = g_list_next(tmplist)) {
      const gchar* spec;
      GdkColor color;

      /* handle both lists */
      if (!tmplist && advance) {
        advance = FALSE;
        tmplist = persistent_list_get_glist(dia_dynamic_menu_get_persistent_name(cs->ddm));
        if (!tmplist)
          break;
      }

      spec = tmplist->data;

      gdk_color_parse (spec, &color);
#if 0
      /* the easy way if the Gtk Team would decide to make it public */
      gtk_color_selection_set_palette_color (colorsel, index, &color);
#else
      g_string_append (palette, spec);
      g_string_append (palette, ":");
#endif
      if (0 == strcmp (spec, old_color)) {
        gtk_color_selection_set_previous_color (colorsel, &color);
        gtk_color_selection_set_current_color (colorsel, &color);
      }
      index++;
    }
  }

  g_object_set (gtk_widget_get_settings (GTK_WIDGET (colorsel)), "gtk-color-palette", palette->str, NULL);
  gtk_color_selection_set_has_palette (colorsel, TRUE);
  g_string_free (palette, TRUE);
  g_free(old_color);

  g_object_get (G_OBJECT (dialog), "help-button", &button, NULL);
  gtk_widget_hide(button);

  g_object_get (G_OBJECT (dialog), "ok-button", &button, NULL);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (dia_color_selector_more_ok), dialog);

  g_object_get (G_OBJECT (dialog), "ok-button", &button, NULL);
  g_signal_connect_swapped (G_OBJECT (button), "clicked",
			    G_CALLBACK(gtk_widget_destroy), G_OBJECT(dialog));

  g_object_set_data(G_OBJECT(dialog), "dia-cs", cs);

  gtk_widget_show(GTK_WIDGET(dialog));
}

static GtkWidget *
dia_color_selector_menu_new (DiaColorSelector *cs)
{
  GtkWidget *otheritem = gtk_menu_item_new_with_label(_("More colors\342\200\246"));
  GtkWidget *ddm = dia_dynamic_menu_new(dia_color_selector_create_string_item,
					NULL,
					GTK_MENU_ITEM(otheritem),
					"color-menu");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#000000");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#FFFFFF");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#FF0000");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#00FF00");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(ddm),
				     "#0000FF");
  g_signal_connect(G_OBJECT(otheritem), "activate",
		   G_CALLBACK(dia_color_selector_more_callback), cs);
  gtk_widget_show(otheritem);

  return ddm;
}

GtkWidget *
dia_color_selector_new (void)
{
  return GTK_WIDGET ( g_object_new (dia_color_selector_get_type (), NULL));
}
void
dia_color_selector_get_color(GtkWidget *widget, Color *color)
{
  DiaColorSelector *cs = DIACOLORSELECTOR(widget);
  gchar *entry = dia_dynamic_menu_get_entry(cs->ddm);
  gint r, g, b;

  sscanf(entry, "#%2x%2x%2x", &r, &g, &b);
  g_free(entry);
  color->red = r / 255.0;
  color->green = g / 255.0;
  color->blue = b / 255.0;

  if (cs->use_alpha) {
    color->alpha = gtk_color_button_get_alpha (cs->color_button) / 65535.0;
  } else {
    color->alpha = 1.0;
  }
}

void
dia_color_selector_set_color (GtkWidget *widget,
			      const Color *color)
{
  DiaColorSelector *cs = DIACOLORSELECTOR(widget);
  gint red, green, blue;
  gchar *entry;
  red = color->red * 255;
  green = color->green * 255;
  blue = color->blue * 255;
  if (color->red > 1.0 || color->green > 1.0 || color->blue > 1.0 || color->alpha > 1.0) {
    printf("Color out of range: r %f, g %f, b %f, a %f\n",
	   color->red, color->green, color->blue, color->alpha);
    red = MIN(red, 255);
    green = MIN(green, 255);
    blue = MIN(blue, 255);
  }
  entry = g_strdup_printf("#%02X%02X%02X", red, green, blue);
  dia_dynamic_menu_select_entry(DIA_DYNAMIC_MENU(cs->ddm), entry);
  g_free (entry);

  if (cs->use_alpha) {
    GdkColor gcol;

    color_convert (color, &gcol);
    gtk_color_button_set_color (cs->color_button, &gcol);
    gtk_color_button_set_alpha (cs->color_button, MIN(color->alpha * 65535, 65535));
  }
}

