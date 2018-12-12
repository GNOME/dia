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

#include <stdlib.h>
#include <string.h>
#include "intl.h"

#include <gtk/gtk.h>
#include "diafontselector.h"
#include "diadynamicmenu.h"
#include "font.h"

/************* DiaFontSelector: ***************/

/* Should these structs be in widgets.h instead? : 
 * _no_ they should not, noone is supposed to mess with the internals, ever heard of information hiding? 
 */

G_DEFINE_TYPE (DiaFontSelector, dia_font_selector, GTK_TYPE_FONT_BUTTON)

enum {
    DFONTSEL_VALUE_CHANGED,
    DFONTSEL_LAST_SIGNAL
};

static guint dfontsel_signals[DFONTSEL_LAST_SIGNAL] = { 0 };


/* New and improved font selector:  Contains the three standard fonts
 * and an 'Other fonts...' entry that opens the font dialog.  The fonts
 * selected in the font dialog are persistently added to the menu.
 *
 * +----------------+
 * | Sans           |
 * | Serif          |
 * | Monospace      |
 * | -------------- |
 * | Bodini         |
 * | CurlyGothic    |
 * | OldWestern     |
 * | -------------- |
 * | Other fonts... |
 * +----------------+
 */

/* GTK3 PORT NOTES:
 *
 * This was implmented as two drop downs, Font & Variant in a hbox. For
 * simplicity this is now a wrapper around GtkFontButton, possibly
 * invesigate returning to old style ?
 */

static void
dia_font_selector_class_init (DiaFontSelectorClass *class)
{
  dfontsel_signals[DFONTSEL_VALUE_CHANGED] =
    g_signal_new ("value_changed",
                  DIA_TYPE_FONT_SELECTOR,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
dia_font_selector_init (DiaFontSelector *fs)
{
  gtk_font_button_set_show_size (GTK_FONT_BUTTON (fs), FALSE);
}

GtkWidget *
dia_font_selector_new ()
{
  return g_object_new (DIA_TYPE_FONT_SELECTOR, NULL);
}

/* API functions */

/** Set the current font to be shown in the font selector.
 */
void
dia_font_selector_set_font(DiaFontSelector *fs, DiaFont *font)
{
  const gchar *fontname = dia_font_get_family(font);
  gtk_font_chooser_set_font_desc (GTK_FONT_CHOOSER (fs),
                                  dia_font_get_description (font));
  /* side effect: adds fontname to presistence list */
  g_signal_emit (G_OBJECT (fs),
                 dfontsel_signals[DFONTSEL_VALUE_CHANGED], 0);
}

DiaFont *
dia_font_selector_get_font(DiaFontSelector *fs)
{
  GtkWidget *menuitem;
  char *fontname;
  DiaFontStyle style;
  DiaFont *font;

  fontname = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (fs));
  
  // TODO: [GTK3] What is DiaFontStyle?
  /*menuitem = gtk_menu_get_active(fs->style_menu);
  if (!menuitem) // FIXME: should not happen ??? (but does if we don't have added a style)
    style = 0;
  else
    style = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuitem), "user_data"));*/
  style = DIA_FONT_NORMAL;
  font = dia_font_new(fontname, style, 1.0);
  g_free(fontname);
  return font;
}
 
