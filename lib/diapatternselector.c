/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diapatternselector.c - some pattern presets for property editing
 *
 * Copyright (C) 2014 Hans Breuer
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "diapatternselector.h"
#include "attributes.h"


typedef struct _DiaPatternSelector DiaPatternSelector;
typedef struct _DiaPatternSelectorClass DiaPatternSelectorClass;
struct _DiaPatternSelector
{
  GtkBox            hbox; /*!< just containing the other two widgets */
  GtkWidget       *state; /*!< button reflecting the state */
  GtkWidget *menu_button; /*!< pop-up menu button to select presets */

  DiaPattern    *pattern; /*!< the active pattern, maybe NULL */
};
struct _DiaPatternSelectorClass
{
  GtkBoxClass parent_class;
};

enum {
  DIA_PATTERNSEL_VALUE_CHANGED,
  DIA_PATTERNSEL_LAST_SIGNAL
};
static guint dia_patternsel_signals[DIA_PATTERNSEL_LAST_SIGNAL] = { 0 };

static DiaPattern *_create_preset_pattern (guint n);


static void
dia_pattern_selector_finalize(GObject *object)
{
  DiaPatternSelector * ps = (DiaPatternSelector *)object;

  g_clear_object (&ps->pattern);
}


static void
dia_pattern_selector_class_init (DiaPatternSelectorClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);

  dia_patternsel_signals[DIA_PATTERNSEL_VALUE_CHANGED]
      = g_signal_new("value_changed",
		     G_TYPE_FROM_CLASS(klass),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
  object_class->finalize = dia_pattern_selector_finalize;
}

static GType dia_pattern_selector_get_type (void);

G_DEFINE_TYPE (DiaPatternSelector, dia_pattern_selector, GTK_TYPE_BOX);


/* GUI stuff - not completely done yet
   - add/remove color stops
   - toggle between radial/linear
   - have some visual representation of the gradient
   - provide a list of predefined like background-to-foreground,
     horizontal/vertical/radial, ...
 */
static void
_pattern_toggled (GtkWidget *wid, DiaPatternSelector *ps)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wid))) {
    gtk_label_set_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (wid))), _("Yes"));
    if (!ps->pattern)
      ps->pattern = _create_preset_pattern (0);
  } else {
    gtk_label_set_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (wid))), _("No"));
    g_clear_object (&ps->pattern);
  }

  g_signal_emit (G_OBJECT (ps), dia_patternsel_signals[DIA_PATTERNSEL_VALUE_CHANGED], 0);
}


/*! Create the pattern preset pop-up menu */
typedef enum {
  LEFT = 0x1,
  DOWN = 0x2
} PatternPresetFlags;
struct _PatternPreset {
  gchar *title;
  guint  type;
  guint  flags;
} _pattern_presets[] = {
  { N_("Horizontal"), DIA_LINEAR_GRADIENT, LEFT },
  { N_("Diagonal"),   DIA_LINEAR_GRADIENT, LEFT|DOWN },
  { N_("Vertical"),   DIA_LINEAR_GRADIENT, DOWN },
  { N_("Radial"),     DIA_RADIAL_GRADIENT }
};

static DiaPattern *
_create_preset_pattern (guint n)
{
  DiaPattern *pat;
  Color       color;

  g_return_val_if_fail (n < G_N_ELEMENTS (_pattern_presets), NULL);
  switch (_pattern_presets[n].type) {
  case DIA_LINEAR_GRADIENT:
    pat = dia_pattern_new (_pattern_presets[n].type, 0, 0.0, 0.0);
    if (_pattern_presets[n].flags != 0)
      dia_pattern_set_point (pat,
			     _pattern_presets[n].flags & LEFT ? 1.0 : 0.0,
			     _pattern_presets[n].flags & DOWN ? 1.0 : 0.0);
    break;
  case DIA_RADIAL_GRADIENT:
    pat = dia_pattern_new (_pattern_presets[n].type, 0, 0.5, 0.5);
    dia_pattern_set_radius (pat, 0.5);
    /* set the focal point to the center */
    dia_pattern_set_point (pat, 0.5, 0.5);
    break;
  default :
    g_assert_not_reached ();
  }

  color = attributes_get_background ();
  dia_pattern_add_color (pat, 0.0, &color);
  color = attributes_get_foreground ();
  dia_pattern_add_color (pat, 1.0, &color);

  return pat;
}

#define PRESET_KEY "preset-pattern-key"

static void
_pattern_activate_preset(GtkWidget *widget, gpointer data)
{
  DiaPatternSelector *ps = (DiaPatternSelector *)data;
  guint n = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget), PRESET_KEY));

  g_set_object (&ps->pattern, _create_preset_pattern (n));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ps->state), ps->pattern != NULL);
  g_signal_emit (G_OBJECT (ps), dia_patternsel_signals[DIA_PATTERNSEL_VALUE_CHANGED], 0);
}


static gint
_popup_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  GtkWidget *menu = gtk_menu_new();
  guint i;

  for (i = 0; i < G_N_ELEMENTS (_pattern_presets); ++i) {
    GtkWidget *menu_item = gtk_menu_item_new_with_label(gettext(_pattern_presets[i].title));
    g_signal_connect (G_OBJECT (menu_item), "activate",
		      G_CALLBACK (_pattern_activate_preset), data);
    g_object_set_data (G_OBJECT (menu_item), PRESET_KEY, GINT_TO_POINTER(i));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    gtk_widget_show(menu_item);
  }
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		 event->button, event->time);
  return FALSE;
}
static void
dia_pattern_selector_init (DiaPatternSelector *ps)
{
  ps->state = gtk_toggle_button_new_with_label(_("No"));
  g_signal_connect(G_OBJECT(ps->state), "toggled",
                   G_CALLBACK (_pattern_toggled), ps);
  gtk_widget_show (ps->state);

  ps->menu_button = gtk_button_new();
  gtk_container_add(GTK_CONTAINER(ps->menu_button),
		    gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT));
  g_signal_connect(G_OBJECT(ps->menu_button), "button_press_event",
		   G_CALLBACK(_popup_button_press), ps);
  gtk_widget_show_all (ps->menu_button);

  gtk_box_pack_start(GTK_BOX(ps), GTK_WIDGET(ps->state), TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(ps), GTK_WIDGET(ps->menu_button), FALSE, TRUE, 0);
}

/*!
 * \brief Create the widget to work with pattern property
 */
GtkWidget *
dia_pattern_selector_new (void)
{
  return g_object_new(dia_pattern_selector_get_type (), NULL);
}

/*!
 * \brief Initialize the widget with pattern
 */
void
dia_pattern_selector_set_pattern (GtkWidget *sel, DiaPattern *pat)
{
  DiaPatternSelector *ps = (DiaPatternSelector *)sel;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ps->state), pat != NULL);
}

/*!
 * \brief Get the pattern from the widget
 */
DiaPattern *
dia_pattern_selector_get_pattern (GtkWidget *sel)
{
  DiaPatternSelector *ps = (DiaPatternSelector *)sel;

  g_return_val_if_fail (ps != NULL, NULL);
  if (ps->pattern)
    return g_object_ref (ps->pattern);
  return NULL;
}
