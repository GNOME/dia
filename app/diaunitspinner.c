/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * diaunitspinner.[ch] -- a spin button widget for length measurements.
 * Copyright (C) 1999 James Henstridge
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

#include <ctype.h>
#include <string.h> /* strcmp */
#include "diaunitspinner.h"
#include "gdk/gdkkeysyms.h"

typedef struct _DiaUnitDef DiaUnitDef;
struct _DiaUnitDef {
  char* name;
  char* unit;
  float factor;
};

/* from gnome-libs/libgnome/gnome-paper.c */
static const DiaUnitDef units[] =
{
  /* XXX does anyone *really* measure paper size in feet?  meters? */

  /* human name, abreviation, points per unit */
  { "Feet",       "ft", 864 },
  { "Meter",      "m",  2834.6457 },
  { "Decimeter",  "dm", 283.46457 },
  { "Millimeter", "mm", 2.8346457 },
  { "Point",      "pt", 1. },
  { "Centimeter", "cm", 28.346457 },
  { "Inch",       "in", 72 },
  { "Pica",       "pi", 12 },
  { 0 }
};

static GtkObjectClass *parent_class;
static GtkObjectClass *entry_class;

static void dia_unit_spinner_class_init(DiaUnitSpinnerClass *class);
static void dia_unit_spinner_init(DiaUnitSpinner *self);

GtkType
dia_unit_spinner_get_type(void)
{
  static GtkType us_type = 0;

  if (!us_type) {
    GtkTypeInfo us_info = {
      "DiaUnitSpinner",
      sizeof(DiaUnitSpinner),
      sizeof(DiaUnitSpinnerClass),
      (GtkClassInitFunc) dia_unit_spinner_class_init,
      (GtkObjectInitFunc) dia_unit_spinner_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
    };
    us_type = gtk_type_unique(gtk_spin_button_get_type(), &us_info);
  }
  return us_type;
}

static void
dia_unit_spinner_value_changed(GtkAdjustment *adjustment,
			       DiaUnitSpinner *spinner)
{
  char buf[256];
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(spinner);

  g_snprintf(buf, sizeof(buf), "%0.*f%s", sbutton->digits, adjustment->value,
	     units[spinner->unit_num].unit);
  if (strcmp(buf, gtk_entry_get_text(GTK_ENTRY(spinner))))
    gtk_entry_set_text(GTK_ENTRY(spinner), buf);
}

static gint dia_unit_spinner_focus_out(GtkWidget *widget, GdkEventFocus *ev);
static gint dia_unit_spinner_button_press(GtkWidget *widget,GdkEventButton*ev);
static gint dia_unit_spinner_key_press(GtkWidget *widget, GdkEventKey *event);
static void dia_unit_spinner_activate(GtkEditable *editable);

static void
dia_unit_spinner_class_init(DiaUnitSpinnerClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkEditableClass *editable_class;

  object_class = (GtkObjectClass *)class;
  widget_class = (GtkWidgetClass *)class;
  editable_class = (GtkEditableClass *)class;

  widget_class->focus_out_event    = dia_unit_spinner_focus_out;
  widget_class->button_press_event = dia_unit_spinner_button_press;
  widget_class->key_press_event    = dia_unit_spinner_key_press;
  editable_class->activate         = dia_unit_spinner_activate;

  parent_class = gtk_type_class(GTK_TYPE_SPIN_BUTTON);
  entry_class  = gtk_type_class(GTK_TYPE_ENTRY);
}

static void
dia_unit_spinner_init(DiaUnitSpinner *self)
{
  /* change over to our own print function that appends the unit name on the
   * end */
  if (self->parent.adjustment) {
    gtk_signal_disconnect_by_data(GTK_OBJECT(self->parent.adjustment),
				  (gpointer) self);
    gtk_signal_connect(GTK_OBJECT(self->parent.adjustment), "value_changed",
		       GTK_SIGNAL_FUNC(dia_unit_spinner_value_changed),
		       (gpointer) self);
  }

  self->unit_num = DIA_UNIT_CENTIMETER;
}

GtkWidget *
dia_unit_spinner_new(GtkAdjustment *adjustment, guint digits, DiaUnit adj_unit)
{
  DiaUnitSpinner *self = gtk_type_new(dia_unit_spinner_get_type());

  gtk_spin_button_configure(GTK_SPIN_BUTTON(self), adjustment, 0.0, digits);

  if (adjustment) {
    gtk_signal_disconnect_by_data(GTK_OBJECT(adjustment),
				  (gpointer) self);
    gtk_signal_connect(GTK_OBJECT(adjustment), "value_changed",
		       GTK_SIGNAL_FUNC(dia_unit_spinner_value_changed),
		       (gpointer) self);
  }

  self->unit_num = adj_unit;

  return GTK_WIDGET(self);
}

void
dia_unit_spinner_set_value(DiaUnitSpinner *self, gfloat val)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  if (val < sbutton->adjustment->lower)
    val = sbutton->adjustment->lower;
  else if (val > sbutton->adjustment->upper)
    val = sbutton->adjustment->upper;
  if (val != sbutton->adjustment->value) {
    sbutton->adjustment->value = val;
    gtk_adjustment_value_changed(sbutton->adjustment);
  }
}

gfloat
dia_unit_spinner_get_value(DiaUnitSpinner *self)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  return sbutton->adjustment->value;
}

static void
dia_unit_spinner_update(DiaUnitSpinner *self)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);
  gfloat val, factor = 1.0;
  gchar *extra = NULL;

  val = g_strtod(gtk_entry_get_text(GTK_ENTRY(self)), &extra);

  /* get rid of extra white space after number */
  while (*extra && isspace(*extra)) extra++;
  if (*extra) {
    int i;

    for (i = 0; units[i].name != NULL; i++)
      if (!g_strcasecmp(units[i].unit, extra)) {
	factor = units[i].factor / units[self->unit_num].factor;
	break;
      }
  }
  /* convert to prefered units */
  val *= factor;
  if (val < sbutton->adjustment->lower)
    val = sbutton->adjustment->lower;
  else if (val > sbutton->adjustment->upper)
    val = sbutton->adjustment->upper;
  gtk_adjustment_set_value(sbutton->adjustment, val);
}

static gint
dia_unit_spinner_focus_out(GtkWidget *widget, GdkEventFocus *event)
{
  if (GTK_EDITABLE(widget)->editable)
    dia_unit_spinner_update(DIA_UNIT_SPINNER(widget));
  return GTK_WIDGET_CLASS(entry_class)->focus_out_event(widget, event);
}

static gint
dia_unit_spinner_button_press(GtkWidget *widget, GdkEventButton *event)
{
  dia_unit_spinner_update(DIA_UNIT_SPINNER(widget));
  return GTK_WIDGET_CLASS(parent_class)->button_press_event(widget, event);
}

static gint
dia_unit_spinner_key_press(GtkWidget *widget, GdkEventKey *event)
{
  gint key = event->keyval;

  if (GTK_EDITABLE (widget)->editable &&
      (key == GDK_Up || key == GDK_Down || 
       key == GDK_Page_Up || key == GDK_Page_Down))
    dia_unit_spinner_update (DIA_UNIT_SPINNER(widget));
  return GTK_WIDGET_CLASS(parent_class)->key_press_event(widget, event);
}

static void
dia_unit_spinner_activate(GtkEditable *editable)
{
  if (editable->editable)
    dia_unit_spinner_update(DIA_UNIT_SPINNER(editable));
}

