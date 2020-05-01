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
#include <string.h>
#include "intl.h"
#include "widgets.h"
#include "units.h"
#include "message.h"
#include "dia_dirs.h"
#include "diaoptionmenu.h"

#include <stdlib.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <stdio.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>

/************* DiaSizeSelector: ***************/
/* A widget that selects two sizes, width and height, optionally keeping
 * aspect ratio.  When created, aspect ratio is locked, but the user can
 * unlock it.  The current users do not store aspect ratio, so we have
 * to give a good default.
 */
struct _DiaSizeSelector
{
  GtkHBox hbox;
  GtkSpinButton *width, *height;
  GtkToggleButton *aspect_locked;
  real ratio;
  GtkAdjustment *last_adjusted;
};

struct _DiaSizeSelectorClass
{
  GtkHBoxClass parent_class;
};

enum {
    DSS_VALUE_CHANGED,
    DSS_LAST_SIGNAL
};

static guint dss_signals[DSS_LAST_SIGNAL] = { 0 };

static void
dia_size_selector_class_init (DiaSizeSelectorClass *class)
{
  dss_signals[DSS_VALUE_CHANGED]
      = g_signal_new("value-changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}

static void
dia_size_selector_adjust_width(DiaSizeSelector *ss)
{
  real height =
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->height));
  if (fabs(ss->ratio) > 1e-6)
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->width), height*ss->ratio);
}

static void
dia_size_selector_adjust_height(DiaSizeSelector *ss)
{
  real width =
    gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->width));
  if (fabs(ss->ratio) > 1e-6)
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->height), width/ss->ratio);
}

static void
dia_size_selector_ratio_callback(GtkAdjustment *limits, gpointer userdata)
{
  static gboolean in_progress = FALSE;
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR(userdata);

  ss->last_adjusted = limits;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ss->aspect_locked))
      && ss->ratio != 0.0) {

    if (in_progress)
      return;
    in_progress = TRUE;

    if (limits == gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(ss->width))) {
      dia_size_selector_adjust_height(ss);
    } else {
      dia_size_selector_adjust_width(ss);
    }

    in_progress = FALSE;
  }

  g_signal_emit(ss, dss_signals[DSS_VALUE_CHANGED], 0);

}

/*
 * Update the ratio of this DSS to be the ratio of width to height.
 * If height is 0, ratio becomes 0.0.
 */
static void
dia_size_selector_set_ratio(DiaSizeSelector *ss, real width, real height)
{
  if (height > 0.0)
    ss->ratio = width/height;
  else
    ss->ratio = 0.0;
}

static void
dia_size_selector_lock_pressed(GtkWidget *widget, gpointer data)
{
  DiaSizeSelector *ss = DIA_SIZE_SELECTOR(data);

  dia_size_selector_set_ratio(ss,
			      gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->width)),
			      gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->height)));
}

/* Possible args:  Init width, init height, digits */

static void
dia_size_selector_init (DiaSizeSelector *ss)
{
  GtkAdjustment *adj;

  ss->ratio = 0.0;
  /* Here's where we set up the real thing */
  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.01, 10,
					  0.1, 1.0, 0));
  ss->width = GTK_SPIN_BUTTON(gtk_spin_button_new(adj, 1.0, 2));
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ss->width), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ss->width), TRUE);
  gtk_box_pack_start(GTK_BOX(ss), GTK_WIDGET(ss->width), FALSE, TRUE, 0);
  gtk_widget_show(GTK_WIDGET(ss->width));

  adj = GTK_ADJUSTMENT(gtk_adjustment_new(1.0, 0.01, 10,
					  0.1, 1.0, 0));
  ss->height = GTK_SPIN_BUTTON(gtk_spin_button_new(adj, 1.0, 2));
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ss->height), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ss->height), TRUE);
  gtk_box_pack_start(GTK_BOX(ss), GTK_WIDGET(ss->height), FALSE, TRUE, 0);
  gtk_widget_show(GTK_WIDGET(ss->height));

  /* Replace label with images */
  /* should make sure they're both unallocated when the widget dies.
  * That should happen in the "destroy" handler, where both should
  * be unref'd */
  ss->aspect_locked = GTK_TOGGLE_BUTTON (
    dia_toggle_button_new_with_icon_names ("dia-chain-unbroken",
                                           "dia-chain-broken"));

  gtk_container_set_border_width(GTK_CONTAINER(ss->aspect_locked), 0);

  gtk_box_pack_start(GTK_BOX(ss), GTK_WIDGET(ss->aspect_locked), FALSE, TRUE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ss->aspect_locked), TRUE);
  gtk_widget_show(GTK_WIDGET(ss->aspect_locked));

  g_signal_connect (G_OBJECT (ss->aspect_locked), "clicked",
                    G_CALLBACK (dia_size_selector_lock_pressed), ss);
  /* Make sure that the aspect ratio stays the same */
  g_signal_connect(G_OBJECT(gtk_spin_button_get_adjustment(ss->width)),
		   "value_changed",
		   G_CALLBACK(dia_size_selector_ratio_callback), (gpointer)ss);
  g_signal_connect(G_OBJECT(gtk_spin_button_get_adjustment(ss->height)),
		   "value_changed",
		   G_CALLBACK(dia_size_selector_ratio_callback), (gpointer)ss);
}

GType
dia_size_selector_get_type (void)
{
  static GType dss_type = 0;

  if (!dss_type) {
    static const GTypeInfo dss_info = {
      sizeof (DiaSizeSelectorClass),
      (GBaseInitFunc)NULL,
      (GBaseFinalizeFunc)NULL,
      (GClassInitFunc)dia_size_selector_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (DiaSizeSelector),
      0, /* n_preallocs */
      (GInstanceInitFunc) dia_size_selector_init
    };

    dss_type = g_type_register_static (gtk_hbox_get_type (),
				       "DiaSizeSelector",
				       &dss_info, 0);
  }
  return dss_type;
}

GtkWidget *
dia_size_selector_new (real width, real height)
{
  GtkWidget *wid;

  wid = GTK_WIDGET ( g_object_new (dia_size_selector_get_type (), NULL));
  dia_size_selector_set_size(DIA_SIZE_SELECTOR(wid), width, height);
  return wid;
}

void
dia_size_selector_set_size(DiaSizeSelector *ss, real width, real height)
{
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->width), width);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(ss->height), height);
  /*
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ss->aspect_locked),
			       fabs(width - height) < 0.000001);
  */
  dia_size_selector_set_ratio(ss, width, height);
}

void
dia_size_selector_set_locked(DiaSizeSelector *ss, gboolean locked)
{
  if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ss->aspect_locked))
      && locked) {
    dia_size_selector_set_ratio(ss,
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->width)),
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->height)));
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ss->aspect_locked), locked);
}

gboolean
dia_size_selector_get_size(DiaSizeSelector *ss, real *width, real *height)
{
  *width = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->width));
  *height = gtk_spin_button_get_value(GTK_SPIN_BUTTON(ss->height));
  return gtk_toggle_button_get_active(ss->aspect_locked);
}

/************* DiaAlignmentSelector: ***************/


GtkWidget *
dia_alignment_selector_new (void)
{
  GtkWidget *omenu = dia_option_menu_new ();

  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Left"), ALIGN_LEFT);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Center"), ALIGN_CENTER);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Right"), ALIGN_RIGHT);

  return omenu;
}


Alignment
dia_alignment_selector_get_alignment(GtkWidget *as)
{
  return (Alignment) dia_option_menu_get_active (DIA_OPTION_MENU (as));
}


void
dia_alignment_selector_set_alignment (GtkWidget *as,
                                      Alignment  align)
{
  dia_option_menu_set_active (DIA_OPTION_MENU (as), align);
}


/************* DiaFileSelector: ***************/
struct _DiaFileSelector
{
  GtkHBox hbox;
  GtkEntry *entry;
  GtkButton *browse;
  GtkWidget *dialog;
  char *sys_filename;
  char *pattern; /* for supported formats */
};

struct _DiaFileSelectorClass
{
  GtkHBoxClass parent_class;
};

enum {
    DFILE_VALUE_CHANGED,
    DFILE_LAST_SIGNAL
};

static guint dfile_signals[DFILE_LAST_SIGNAL] = { 0 };

static void
dia_file_selector_unrealize(GtkWidget *widget)
{
  DiaFileSelector *fs = DIAFILESELECTOR(widget);

  if (fs->dialog != NULL) {
    gtk_widget_destroy(GTK_WIDGET(fs->dialog));
    fs->dialog = NULL;
  }
  g_clear_pointer (&fs->sys_filename, g_free);
  g_clear_pointer (&fs->pattern, g_free);

  (* GTK_WIDGET_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (fs)))->unrealize) (widget);
}

static void
dia_file_selector_class_init (DiaFileSelectorClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass*) class;
  widget_class->unrealize = dia_file_selector_unrealize;

  dfile_signals[DFILE_VALUE_CHANGED]
      = g_signal_new("value-changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}

static void
dia_file_selector_entry_changed(GtkEditable *editable
				, gpointer data)
{
  DiaFileSelector *fs = DIAFILESELECTOR(data);
  g_signal_emit(fs, dfile_signals[DFILE_VALUE_CHANGED], 0);
}


static void
file_open_response_callback(GtkWidget *dialog,
                            gint       response,
                            gpointer   user_data)
{
  DiaFileSelector *fs =
    DIAFILESELECTOR(g_object_get_data(G_OBJECT(dialog), "user_data"));

  if (response == GTK_RESPONSE_ACCEPT || response == GTK_RESPONSE_OK) {
    char *utf8 = g_filename_to_utf8 (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog)),
                            -1, NULL, NULL, NULL);
    gtk_entry_set_text (GTK_ENTRY (fs->entry), utf8);
    g_clear_pointer (&utf8, g_free);
  }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}


static void
dia_file_selector_browse_pressed (GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  DiaFileSelector *fs = DIAFILESELECTOR(data);
  char *filename;
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);

  if (toplevel && !GTK_WINDOW(toplevel))
    toplevel = NULL;

  if (fs->dialog == NULL) {
    GtkFileFilter *filter;

    dialog = fs->dialog =
      gtk_file_chooser_dialog_new (_("Select image file"),
                                   toplevel ? GTK_WINDOW (toplevel) : NULL,
                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                   _("_Cancel"), GTK_RESPONSE_CANCEL,
                                   _("_Open"), GTK_RESPONSE_ACCEPT,
                                   NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    g_signal_connect (G_OBJECT (dialog), "response",
                      G_CALLBACK (file_open_response_callback), NULL);
    g_signal_connect (G_OBJECT (fs->dialog), "destroy",
                      G_CALLBACK (gtk_widget_destroyed), &fs->dialog);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Supported Formats"));
    if (fs->pattern)
      gtk_file_filter_add_pattern (filter, fs->pattern);
    else /* fallback */
      gtk_file_filter_add_pixbuf_formats (filter);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All Files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

    g_object_set_data(G_OBJECT(dialog), "user_data", fs);
  }

  filename = g_filename_from_utf8(gtk_entry_get_text(fs->entry), -1, NULL, NULL, NULL);
  /* selecting something in the filechooser officially sucks. See e.g. http://bugzilla.gnome.org/show_bug.cgi?id=307378 */
  if (g_path_is_absolute(filename))
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(fs->dialog), filename);

  g_clear_pointer (&filename, g_free);

  gtk_widget_show(GTK_WIDGET(fs->dialog));
}

static void
dia_file_selector_init (DiaFileSelector *fs)
{
  /* Here's where we set up the real thing */
  fs->dialog = NULL;
  fs->sys_filename = NULL;
  fs->pattern = NULL;

  fs->entry = GTK_ENTRY(gtk_entry_new());
  gtk_box_pack_start(GTK_BOX(fs), GTK_WIDGET(fs->entry), FALSE, TRUE, 0);
  g_signal_connect(G_OBJECT(fs->entry), "changed",
		   G_CALLBACK(dia_file_selector_entry_changed), fs);
  gtk_widget_show(GTK_WIDGET(fs->entry));

  fs->browse = GTK_BUTTON(gtk_button_new_with_label(_("Browse")));
  gtk_box_pack_start(GTK_BOX(fs), GTK_WIDGET(fs->browse), FALSE, TRUE, 0);
  g_signal_connect (G_OBJECT (fs->browse), "clicked",
                    G_CALLBACK(dia_file_selector_browse_pressed), fs);
  gtk_widget_show(GTK_WIDGET(fs->browse));
}


GType
dia_file_selector_get_type (void)
{
  static GType dfs_type = 0;

  if (!dfs_type) {
    static const GTypeInfo dfs_info = {
      sizeof (DiaFileSelectorClass),
      (GBaseInitFunc)NULL,
      (GBaseFinalizeFunc)NULL,
      (GClassInitFunc)dia_file_selector_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (DiaFileSelector),
      0, /* n_preallocs */
      (GInstanceInitFunc)dia_file_selector_init,
    };

    dfs_type = g_type_register_static (gtk_hbox_get_type (),
				       "DiaFileSelector",
				       &dfs_info, 0);
  }
  return dfs_type;
}

GtkWidget *
dia_file_selector_new (void)
{
  return GTK_WIDGET ( g_object_new (dia_file_selector_get_type (), NULL));
}

void
dia_file_selector_set_extensions  (DiaFileSelector *fs, const char **exts)
{
  GString *pattern = g_string_new ("*.");
  int i = 0;

  g_clear_pointer (&fs->pattern, g_free);

  while (exts[i] != NULL) {
    if (i != 0)
      g_string_append (pattern, "|*.");
    g_string_append (pattern, exts[i]);
    ++i;
  }
  fs->pattern = pattern->str;
  g_string_free (pattern, FALSE);
}


void
dia_file_selector_set_file (DiaFileSelector *fs, char *file)
{
  /* filename is in system encoding */
  char *utf8 = g_filename_to_utf8 (file, -1, NULL, NULL, NULL);
  gtk_entry_set_text (GTK_ENTRY (fs->entry), utf8);
  g_clear_pointer (&utf8, g_free);
}


const char *
dia_file_selector_get_file(DiaFileSelector *fs)
{
  /* let it behave like gtk_file_selector_get_file */
  g_clear_pointer (&fs->sys_filename, g_free);
  fs->sys_filename = g_filename_from_utf8(gtk_entry_get_text(GTK_ENTRY(fs->entry)),
                                          -1, NULL, NULL, NULL);
  return fs->sys_filename;
}

/************* DiaUnitSpinner: ***************/

/**
 * DiaUnitSpinner:
 *
 * A Spinner that allows a 'favoured' unit to display in. External access
 * to the value still happens in cm, but display is in the favored unit.
 * Internally, the value is kept in the favored unit to a) allow proper
 * limits, and b) avoid rounding problems while editing.
 *
 * Since: dawn-of-time
 */

static void dia_unit_spinner_init(DiaUnitSpinner *self);

GType
dia_unit_spinner_get_type(void)
{
  static GType us_type = 0;

  if (!us_type) {
    static const GTypeInfo us_info = {
      sizeof(DiaUnitSpinnerClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      NULL, /* class_init*/
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof(DiaUnitSpinner),
      0,    /* n_preallocs */
      (GInstanceInitFunc) dia_unit_spinner_init,
    };
    us_type = g_type_register_static(GTK_TYPE_SPIN_BUTTON,
                                     "DiaUnitSpinner",
                                     &us_info, 0);
  }
  return us_type;
}


static void
dia_unit_spinner_init(DiaUnitSpinner *self)
{
  self->unit_num = DIA_UNIT_CENTIMETER;
}

/*
  Callback functions for the "input" and "output" signals emitted by
  GtkSpinButton. All the normal work is done by the spin button, we
  simply modify how the text in the GtkEntry is treated.
*/
static gboolean dia_unit_spinner_input(DiaUnitSpinner *self, gdouble *value);
static gboolean dia_unit_spinner_output(DiaUnitSpinner *self);

GtkWidget *
dia_unit_spinner_new(GtkAdjustment *adjustment, DiaUnit adj_unit)
{
  DiaUnitSpinner *self;

  if (adjustment) {
    g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);
  }

  self = g_object_new(dia_unit_spinner_get_type(), NULL);
  gtk_entry_set_activates_default(GTK_ENTRY(self), TRUE);
  self->unit_num = adj_unit;

  gtk_spin_button_configure (GTK_SPIN_BUTTON (self),
                             adjustment, 0.0, dia_unit_get_digits (adj_unit));

  g_signal_connect(GTK_SPIN_BUTTON(self), "output",
                   G_CALLBACK(dia_unit_spinner_output),
                   NULL);
  g_signal_connect(GTK_SPIN_BUTTON(self), "input",
                   G_CALLBACK(dia_unit_spinner_input),
                   NULL);

  return GTK_WIDGET(self);
}


static gboolean
dia_unit_spinner_input (DiaUnitSpinner *self, double *value)
{
  double val, factor = 1.0;
  char *extra = NULL;

  val = g_strtod (gtk_entry_get_text (GTK_ENTRY (self)), &extra);

  /* get rid of extra white space after number */
  while (*extra && g_ascii_isspace (*extra)) {
    extra++;
  }

  if (*extra) {
    for (int i = 0; i < DIA_LAST_UNIT; i++) {
      if (!g_ascii_strcasecmp (dia_unit_get_symbol (i), extra)) {
        factor = dia_unit_get_factor (i) /
                  dia_unit_get_factor (self->unit_num);
        break;
      }
    }
  }

  /* convert to prefered units */
  val *= factor;

  /* Store value in the location provided by the signal emitter. */
  *value = val;

  /* Return true, so that the default input function is not invoked. */
  return TRUE;
}


static gboolean
dia_unit_spinner_output (DiaUnitSpinner *self)
{
  char buf[256];
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON (self);
  GtkAdjustment *adjustment = gtk_spin_button_get_adjustment (sbutton);

  g_snprintf (buf,
              sizeof(buf),
              "%0.*f %s",
              gtk_spin_button_get_digits(sbutton),
              gtk_adjustment_get_value(adjustment),
              dia_unit_get_symbol (self->unit_num));
  gtk_entry_set_text (GTK_ENTRY(self), buf);

  /* Return true, so that the default output function is not invoked. */
  return TRUE;
}


/**
 * dia_unit_spinner_set_value:
 * @self: the DiaUnitSpinner
 * @val: value in %DIA_UNIT_CENTIMETER
 *
 * Set the value (in cm).
 *
 * Since: dawn-of-time
 */
void
dia_unit_spinner_set_value (DiaUnitSpinner *self, double val)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  gtk_spin_button_set_value (sbutton,
                             val /
                             (dia_unit_get_factor (self->unit_num) /
                              (dia_unit_get_factor (DIA_UNIT_CENTIMETER))));
}


/**
 * dia_unit_spinner_get_value:
 * @self: the DiaUnitSpinner
 *
 * Get the value (in cm)
 *
 * Returns: The value in %DIA_UNIT_CENTIMETER
 *
 * Since: dawn-of-time
 */
double
dia_unit_spinner_get_value (DiaUnitSpinner *self)
{
  GtkSpinButton *sbutton = GTK_SPIN_BUTTON(self);

  return gtk_spin_button_get_value (sbutton) *
                    (dia_unit_get_factor (self->unit_num) /
                     dia_unit_get_factor (DIA_UNIT_CENTIMETER));
}


/**
 * dia_unit_spinner_set_upper:
 * @self: the DiaUnitSpinner
 * @val: value in %DIA_UNIT_CENTIMETER
 *
 * Must manipulate the limit values through this to also consider unit.
 *
 * Given value is in centimeter.
 *
 * Since: dawn-of-time
 */
void
dia_unit_spinner_set_upper (DiaUnitSpinner *self, double val)
{
  val /= (dia_unit_get_factor (self->unit_num) /
          dia_unit_get_factor (DIA_UNIT_CENTIMETER));

  gtk_adjustment_set_upper (
    gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (self)), val);
}


/* ************************ Misc. util functions ************************ */
struct image_pair { GtkWidget *on; GtkWidget *off; };

static void
dia_toggle_button_swap_images(GtkToggleButton *widget,
			      gpointer data)
{
  struct image_pair *images = (struct image_pair *)data;
  if (gtk_toggle_button_get_active(widget)) {
    gtk_container_remove(GTK_CONTAINER(widget),
			 gtk_bin_get_child(GTK_BIN(widget)));
    gtk_container_add(GTK_CONTAINER(widget),
		      images->on);

  } else {
    gtk_container_remove(GTK_CONTAINER(widget),
			 gtk_bin_get_child(GTK_BIN(widget)));
    gtk_container_add(GTK_CONTAINER(widget),
		      images->off);
  }
}


static void
dia_toggle_button_destroy (GtkWidget *widget, gpointer data)
{
  struct image_pair *images = (struct image_pair *) data;

  g_clear_object (&images->on);
  g_clear_object (&images->off);
  g_clear_pointer (&images, g_free);

}


/** Create a toggle button given two image widgets for on and off */
static GtkWidget *
dia_toggle_button_new(GtkWidget *on_widget, GtkWidget *off_widget)
{
  GtkWidget *button = gtk_toggle_button_new();
  GtkRcStyle *rcstyle;
  struct image_pair *images;

  images = g_new(struct image_pair, 1);
  /* Since these may not be added at any point, make sure to
   * sink them. */
  images->on = on_widget;
  g_object_ref_sink(images->on);
  gtk_widget_show(images->on);

  images->off = off_widget;
  g_object_ref_sink(images->off);
  gtk_widget_show(images->off);

  /* Make border as small as possible */
  gtk_misc_set_padding (GTK_MISC (images->on), 0, 0);
  gtk_misc_set_padding (GTK_MISC (images->off), 0, 0);
  gtk_widget_set_can_focus (GTK_WIDGET (button), FALSE);
  gtk_widget_set_can_default (GTK_WIDGET (button), FALSE);

  rcstyle = gtk_rc_style_new ();
  rcstyle->xthickness = rcstyle->ythickness = 0;
  gtk_widget_modify_style (button, rcstyle);
  g_clear_object (&rcstyle);

  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  /*  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);*/
  gtk_container_set_border_width(GTK_CONTAINER(button), 0);

  gtk_container_add(GTK_CONTAINER(button), images->off);

  g_signal_connect(G_OBJECT(button), "toggled",
		   G_CALLBACK(dia_toggle_button_swap_images), images);
  g_signal_connect(G_OBJECT(button), "destroy",
		   G_CALLBACK(dia_toggle_button_destroy), images);

  return button;
}


/*
 * Create a toggle button with two icons (created with gdk-pixbuf-csource,
 * for instance).  The icons represent on and off.
 */


/* GTK3: This is built-in (new_from_resource, add_resource_path....) */
/* Adapted from Gtk */
GdkPixbuf *
pixbuf_from_resource (const char *path)
{
  GdkPixbufLoader *loader = NULL;
  GdkPixbuf *pixbuf = NULL;
  GBytes *bytes;

  g_return_val_if_fail (path != NULL, NULL);

  bytes = g_resources_lookup_data (path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);

  if (!bytes) {
    g_critical ("Missing resource %s", path);
    goto out;
  }

  loader = gdk_pixbuf_loader_new ();

  if (!gdk_pixbuf_loader_write_bytes (loader, bytes, NULL))
    goto out;

  if (!gdk_pixbuf_loader_close (loader, NULL))
    goto out;

  pixbuf = g_object_ref (gdk_pixbuf_loader_get_pixbuf (loader));

 out:
  gdk_pixbuf_loader_close (loader, NULL);
  g_clear_object (&loader);
  g_bytes_unref (bytes);

  return pixbuf;
}


GtkWidget *
dia_toggle_button_new_with_icon_names (const char *on,
                                       const char *off)
{
  GtkWidget *on_img, *off_img;

  on_img = gtk_image_new_from_pixbuf (pixbuf_from_resource (g_strdup_printf ("/org/gnome/Dia/icons/%s.png", on)));
  off_img = gtk_image_new_from_pixbuf (pixbuf_from_resource (g_strdup_printf ("/org/gnome/Dia/icons/%s.png", off)));

  return dia_toggle_button_new (on_img, off_img);
}
