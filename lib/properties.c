/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * properties.h: property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
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

#include <glib.h>

#include "geometry.h"
#include "arrows.h"
#include "color.h"
#include "font.h"

#include "intl.h"
#include "widgets.h"

#undef G_INLINE_FUNC
#define G_INLINE_FUNC extern
#define G_CAN_INLINE 1
#include "properties.h"

typedef struct {
  gchar *name;
  PropType_Copy cfunc;
  PropType_Free ffunc;
  PropType_GetWidget wfunc;
  PropType_SetProp sfunc;
} CustomProp;

static GArray *custom_props = NULL;
static GHashTable *custom_props_hash = NULL;

PropType
prop_type_register(const gchar *name, PropType_Copy cfunc,
		   PropType_Free ffunc, PropType_GetWidget wfunc,
		   PropType_SetProp sfunc)
{
  CustomProp cprop;
  PropType ret;

  g_return_val_if_fail(name != NULL, PROP_TYPE_INVALID);
  g_return_val_if_fail((wfunc != NULL) == (sfunc != NULL), PROP_TYPE_INVALID);

  if (!custom_props) {
    custom_props = g_array_new(TRUE, TRUE, sizeof(CustomProp));
    custom_props_hash = g_hash_table_new(g_str_hash, g_str_equal);
  }

  cprop.name = g_strdup(name);
  cprop.cfunc = cfunc;
  cprop.ffunc = ffunc;
  cprop.wfunc = wfunc;
  cprop.sfunc = sfunc;

  ret = custom_props->len + PROP_LAST;
  g_array_append_val(custom_props, cprop);
  g_hash_table_insert(custom_props_hash, cprop.name,
		&g_array_index(custom_props, CustomProp, custom_props->len-1));

  return ret;
}

PropDescription *
prop_desc_lists_union(GList *plists)
{
  GArray *arr = g_array_new(TRUE, TRUE, sizeof(PropDescription));
  PropDescription *ret;
  GList *tmp;

  for (tmp = plists; tmp; tmp = tmp->next) {
    PropDescription *plist = tmp->data;
    int i;

    for (i = 0; plist[i].name != NULL; i++) {
      int j;
      for (j = 0; j < arr->len; j++)
	if (g_array_index(arr, PropDescription, j).quark == plist[i].quark)
	  break;
      if (j == arr->len)
	g_array_append_val(arr, plist[i]);
    }
  }
  ret = (PropDescription *)arr->data;
  g_array_free(arr, FALSE);
  return ret;
}

PropDescription *
prop_desc_lists_intersection(GList *plists)
{
  GArray *arr = g_array_new(TRUE, TRUE, sizeof(PropDescription));
  PropDescription *ret;
  GList *tmp;
  gint i;

  if (plists) {
    ret = plists->data;
    for (i = 0; ret[i].name != NULL; i++)
      g_array_append_val(arr, ret[i]);

    /* check each PropDescription list for intersection */
    for (tmp = plists->next; tmp; tmp = tmp->next) {
      ret = tmp->data;

      /* go through array in reverse so that removals don't stuff things up */
      for (i = arr->len - 1; i >= 0; i++) {
	gint j;

	for (j = 0; ret[j].name != NULL; j++)
	  if (g_array_index(arr, PropDescription, i).quark == ret[j].quark)
	    break;
	if (ret[j].name == NULL)
	  g_array_remove_index(arr, i);
      }
    }
  }
  ret = (PropDescription *)arr->data;
  g_array_free(arr, FALSE);
  return ret;
}

void
prop_copy(Property *dest, Property *src)
{
  CustomProp *cp;

  dest->name = src->name;
  dest->type = src->type;
  dest->extra_data = src->extra_data;
  switch (src->type) {
  case PROP_TYPE_INVALID:
  case PROP_TYPE_CHAR:
  case PROP_TYPE_BOOL:
  case PROP_TYPE_INT:
  case PROP_TYPE_ENUM:
  case PROP_TYPE_POINT:
  case PROP_TYPE_RECT:
  case PROP_TYPE_ARROW:
  case PROP_TYPE_COLOUR:
  case PROP_TYPE_FONT:
    dest->d = src->d;
    break;
  case PROP_TYPE_STRING:
    g_free(PROP_VALUE_STRING(*dest));
    PROP_VALUE_STRING(*dest) = g_strdup(PROP_VALUE_STRING(*src));
    break;
  case PROP_TYPE_POINTARRAY:
    g_free(PROP_VALUE_POINTARRAY(*dest).pts);
    PROP_VALUE_POINTARRAY(*dest).npts = PROP_VALUE_POINTARRAY(*src).npts;
    PROP_VALUE_POINTARRAY(*dest).pts =g_memdup(PROP_VALUE_POINTARRAY(*src).pts,
			sizeof(Point) * PROP_VALUE_POINTARRAY(*src).npts);
    break;
  default:
    if (src->type - PROP_LAST >= custom_props->len) {
      g_warning("prop type id out of range!!!");
      return;
    }
    cp = &g_array_index(custom_props, CustomProp, src->type - PROP_LAST);
    if (cp->ffunc)
      cp->ffunc(dest);
    if (cp->cfunc)
      cp->cfunc(dest, src);
    else
      dest->d = src->d; /* straight copy */
  }
}

void
prop_free(Property *prop)
{
  CustomProp *cp;

  switch (prop->type) {
  case PROP_TYPE_INVALID:
  case PROP_TYPE_CHAR:
  case PROP_TYPE_BOOL:
  case PROP_TYPE_INT:
  case PROP_TYPE_ENUM:
  case PROP_TYPE_POINT:
  case PROP_TYPE_RECT:
  case PROP_TYPE_ARROW:
  case PROP_TYPE_COLOUR:
  case PROP_TYPE_FONT:
    break;
  case PROP_TYPE_STRING:
    g_free(PROP_VALUE_STRING(*prop));
    break;
  case PROP_TYPE_POINTARRAY:
    g_free(PROP_VALUE_POINTARRAY(*prop).pts);
    break;
  default:
    if (prop->type - PROP_LAST >= custom_props->len) {
      g_warning("prop type id out of range!!!");
      return;
    }
    cp = &g_array_index(custom_props, CustomProp, prop->type - PROP_LAST);
    if (cp->ffunc)
      cp->ffunc(prop);
  }
}

static void
bool_toggled(GtkWidget *wid)
{
  if (GTK_TOGGLE_BUTTON(wid)->active)
    gtk_label_set(GTK_LABEL(GTK_BIN(wid)->child), _("Yes"));
  else
    gtk_label_set(GTK_LABEL(GTK_BIN(wid)->child), _("No"));
}

GtkWidget *
prop_get_widget(Property *prop)
{
  GtkWidget *ret = NULL;
  gchar buf[256];

  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("invalid property type for `%s'",  prop->name);
    break;
  case PROP_TYPE_CHAR:
    ret = gtk_entry_new();
    buf[0] = PROP_VALUE_CHAR(*prop);
    buf[1] = '\0';
    gtk_entry_set_text(GTK_ENTRY(ret), buf);
    break;
  case PROP_TYPE_BOOL:
    ret = gtk_toggle_button_new_with_label(_("No"));
    gtk_signal_connect(GTK_OBJECT(ret), "toggled",
		       GTK_SIGNAL_FUNC(bool_toggled), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret),
				 PROP_VALUE_BOOL(*prop));
    break;
  case PROP_TYPE_INT:
  case PROP_TYPE_ENUM:
    ret = gtk_entry_new(); /* should use spin button/option menu */
    g_snprintf(buf, sizeof(buf), "%d", PROP_VALUE_INT(*prop));
    gtk_entry_set_text(GTK_ENTRY(ret), buf);
    break;
  case PROP_TYPE_REAL:
    ret = gtk_entry_new(); /* should use spin button */
    g_snprintf(buf, sizeof(buf), "%g", PROP_VALUE_REAL(*prop));
    gtk_entry_set_text(GTK_ENTRY(ret), buf);
    break;
  case PROP_TYPE_STRING:
    ret = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(ret), PROP_VALUE_STRING(*prop));
    break;
  case PROP_TYPE_POINT:
  case PROP_TYPE_POINTARRAY:
  case PROP_TYPE_RECT:
    ret = gtk_label_new(_("No edit widget"));
    break;
  case PROP_TYPE_ARROW:
    ret = dia_arrow_selector_new();
    dia_arrow_selector_set_arrow(DIAARROWSELECTOR(ret),
				 PROP_VALUE_ARROW(*prop));
    break;
  case PROP_TYPE_COLOUR:
    ret = dia_color_selector_new();
    dia_color_selector_set_color(DIACOLORSELECTOR(ret),
				 &PROP_VALUE_COLOUR(*prop));
    break;
  case PROP_TYPE_FONT:
    ret = dia_font_selector_new();
    dia_font_selector_set_font(DIAFONTSELECTOR(ret), PROP_VALUE_FONT(*prop));
    break;
  default:
    /* custom property */
    if (prop->type - PROP_LAST >= custom_props->len ||
	g_array_index(custom_props, CustomProp,
		      prop->type - PROP_LAST).wfunc == NULL)
      ret = gtk_label_new(_("No edit widget"));
    else
      ret = g_array_index(custom_props, CustomProp,
			  prop->type - PROP_LAST).wfunc(prop);
  }
  return ret;
}

void
prop_set_from_widget(Property *prop, GtkWidget *widget)
{
  switch (prop->type) {
  case PROP_TYPE_INVALID:
    g_warning("invalid property type for `%s'",  prop->name);
    break;
  case PROP_TYPE_CHAR:
    PROP_VALUE_CHAR(*prop) = gtk_entry_get_text(GTK_ENTRY(widget))[0];
    break;
  case PROP_TYPE_BOOL:
    PROP_VALUE_BOOL(*prop) = GTK_TOGGLE_BUTTON(widget)->active;
    break;
  case PROP_TYPE_INT:
  case PROP_TYPE_ENUM:
    PROP_VALUE_INT(*prop) = strtol(gtk_entry_get_text(GTK_ENTRY(widget)),
				   NULL, 0);
    break;
  case PROP_TYPE_REAL:
    PROP_VALUE_REAL(*prop) = g_strtod(gtk_entry_get_text(GTK_ENTRY(widget)),
				      NULL);
    break;
  case PROP_TYPE_STRING:
    g_free(PROP_VALUE_STRING(*prop));
    PROP_VALUE_STRING(*prop) = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
    break;
  case PROP_TYPE_POINT:
  case PROP_TYPE_POINTARRAY:
  case PROP_TYPE_RECT:
    /* nothing */
    break;
  case PROP_TYPE_ARROW:
    PROP_VALUE_ARROW(*prop) =
      dia_arrow_selector_get_arrow(DIAARROWSELECTOR(widget));
    break;
  case PROP_TYPE_COLOUR:
    dia_color_selector_get_color(DIACOLORSELECTOR(widget),
				 &PROP_VALUE_COLOUR(*prop));
    break;
  case PROP_TYPE_FONT:
     PROP_VALUE_FONT(*prop) =
       dia_font_selector_get_font(DIAFONTSELECTOR(widget));
    break;
  default:
    /* custom property */
    if (prop->type - PROP_LAST < custom_props->len &&
	g_array_index(custom_props, CustomProp,
		      prop->type - PROP_LAST).sfunc != NULL)
      g_array_index(custom_props, CustomProp,
		    prop->type - PROP_LAST).sfunc(prop, widget);
  }
}

