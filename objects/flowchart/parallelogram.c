/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Flowchart toolbox -- objects for drawing flowcharts.
 * Copyright (C) 1999 James Henstridge.
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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "widgets.h"
#include "message.h"
#include "sheet.h"

#include "pixmaps/pgram.xpm"

/* used when resizing to decide which side of the shape to expand/shrink */
typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

typedef struct _Pgram Pgram;
typedef struct _PgramPropertiesDialog PgramPropertiesDialog;
typedef struct _PgramDefaultsDialog PgramDefaultsDialog;
typedef struct _PgramState PgramState;

struct _PgramState {
  ObjectState obj_state;
  
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
  real shear_angle;

  real padding;
  TextAttributes text_attrib;
};

struct _Pgram {
  Element element;

  ConnectionPoint connections[16];
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
  real shear_angle, shear_grad;

  Text *text;
  real padding;
};

typedef struct _PgramProperties {
  Color *fg_color;
  Color *bg_color;
  gboolean show_background;
  real border_width;
  real shear_angle;

  real padding;
  Font *font;
  real font_size;
  Color *font_color;
} PgramProperties;

struct _PgramPropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;
  GtkToggleButton *show_background;
  DiaLineStyleSelector *line_style;
  GtkSpinButton *shear_angle;

  GtkSpinButton *padding;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
  DiaColorSelector *font_color;

  Pgram *pgram;
};

struct _PgramDefaultsDialog {
  GtkWidget *vbox;

  GtkToggleButton *show_background;
  GtkSpinButton *shear_angle;

  GtkSpinButton *padding;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
};


static PgramPropertiesDialog *pgram_properties_dialog;
static PgramDefaultsDialog *pgram_defaults_dialog;
static PgramProperties default_properties;

static real pgram_distance_from(Pgram *pgram, Point *point);
static void pgram_select(Pgram *pgram, Point *clicked_point,
		       Renderer *interactive_renderer);
static void pgram_move_handle(Pgram *pgram, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
static void pgram_move(Pgram *pgram, Point *to);
static void pgram_draw(Pgram *pgram, Renderer *renderer);
static void pgram_update_data(Pgram *pgram, AnchorShape h, AnchorShape v);
static Object *pgram_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void pgram_destroy(Pgram *pgram);
static Object *pgram_copy(Pgram *pgram);
static GtkWidget *pgram_get_properties(Pgram *pgram);
static ObjectChange *pgram_apply_properties(Pgram *pgram);

static PgramState *pgram_get_state(Pgram *pgram);
static void pgram_set_state(Pgram *pgram, PgramState *state);

static void pgram_save(Pgram *pgram, ObjectNode obj_node, const char *filename);
static Object *pgram_load(ObjectNode obj_node, int version, const char *filename);
static GtkWidget *pgram_get_defaults();
static void pgram_apply_defaults();

static ObjectTypeOps pgram_type_ops =
{
  (CreateFunc) pgram_create,
  (LoadFunc)   pgram_load,
  (SaveFunc)   pgram_save,
  (GetDefaultsFunc)   pgram_get_defaults,
  (ApplyDefaultsFunc) pgram_apply_defaults
};

ObjectType pgram_type =
{
  "Flowchart - Parallelogram",  /* name */
  0,                 /* version */
  (char **) pgram_xpm, /* pixmap */

  &pgram_type_ops      /* ops */
};

SheetObject pgram_sheetobj =
{
  "Flowchart - Parallelogram",
  N_("A parallelogram with text inside."),
  (char **)pgram_xpm,
  NULL
};

static ObjectOps pgram_ops = {
  (DestroyFunc)         pgram_destroy,
  (DrawFunc)            pgram_draw,
  (DistanceFunc)        pgram_distance_from,
  (SelectFunc)          pgram_select,
  (CopyFunc)            pgram_copy,
  (MoveFunc)            pgram_move,
  (MoveHandleFunc)      pgram_move_handle,
  (GetPropertiesFunc)   pgram_get_properties,
  (ApplyPropertiesFunc) pgram_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
pgram_apply_properties(Pgram *pgram)
{
  ObjectState *old_state;
  Font *font;
  Color col;

  if (pgram != pgram_properties_dialog->pgram) {
    message_warning("Pgram dialog problem:  %p != %p\n", pgram,
		    pgram_properties_dialog->pgram);
    pgram = pgram_properties_dialog->pgram;
  }

  old_state = (ObjectState *)pgram_get_state(pgram);
  
  pgram->border_width = gtk_spin_button_get_value_as_float(pgram_properties_dialog->border_width);
  dia_color_selector_get_color(pgram_properties_dialog->fg_color, &pgram->border_color);
  dia_color_selector_get_color(pgram_properties_dialog->bg_color, &pgram->inner_color);
  pgram->show_background = gtk_toggle_button_get_active(pgram_properties_dialog->show_background);
  dia_line_style_selector_get_linestyle(pgram_properties_dialog->line_style, &pgram->line_style, &pgram->dashlength);
  pgram->shear_angle = gtk_spin_button_get_value_as_float(pgram_properties_dialog->shear_angle);
  pgram->shear_grad = tan(M_PI/2.0 - M_PI/180.0 * pgram->shear_angle);

  pgram->padding = gtk_spin_button_get_value_as_float(pgram_properties_dialog->padding);
  font = dia_font_selector_get_font(pgram_properties_dialog->font);
  text_set_font(pgram->text, font);
  text_set_height(pgram->text, gtk_spin_button_get_value_as_float(
					pgram_properties_dialog->font_size));
  dia_color_selector_get_color(pgram_properties_dialog->font_color, &col);
  text_set_color(pgram->text, &col);
  
  pgram_update_data(pgram, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return new_object_state_change((Object *)pgram, old_state, 
				 (GetStateFunc)pgram_get_state,
				 (SetStateFunc)pgram_set_state);
}

static GtkWidget *
pgram_get_properties(Pgram *pgram)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *checkpgram;
  GtkWidget *linestyle;
  GtkWidget *border_width;
  GtkWidget *shear_angle;
  GtkWidget *padding;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkAdjustment *adj;

  if (pgram_properties_dialog == NULL) {
    pgram_properties_dialog = g_new(PgramPropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    pgram_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Border width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    pgram_properties_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Foreground color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    pgram_properties_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Background color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    pgram_properties_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkpgram = gtk_check_button_new_with_label(_("Draw background"));
    pgram_properties_dialog->show_background = GTK_TOGGLE_BUTTON( checkpgram );
    gtk_widget_show(checkpgram);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkpgram, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    pgram_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Shear angle:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(60.0, 45.0, 135.0, 1.0,0.0,0.0);
    shear_angle = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(shear_angle), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(shear_angle), TRUE);
    pgram_properties_dialog->shear_angle = GTK_SPIN_BUTTON(shear_angle);
    gtk_box_pack_start(GTK_BOX (hbox), shear_angle, TRUE, TRUE, 0);
    gtk_widget_show (shear_angle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Text padding:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.0, 10.0, 0.1, 0.0, 0.0);
    padding = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(padding), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(padding), TRUE);
    pgram_properties_dialog->padding = GTK_SPIN_BUTTON(padding);
    gtk_box_pack_start(GTK_BOX (hbox), padding, TRUE, TRUE, 0);
    gtk_widget_show (padding);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    pgram_properties_dialog->font = DIAFONTSELECTOR(font);
    gtk_box_pack_start (GTK_BOX (hbox), font, TRUE, TRUE, 0);
    gtk_widget_show (font);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font size:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.1, 10.0, 0.1, 0.0, 0.0);
    font_size = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(font_size), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(font_size), TRUE);
    pgram_properties_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    pgram_properties_dialog->font_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  pgram_properties_dialog->pgram = pgram;
    
  gtk_spin_button_set_value(pgram_properties_dialog->border_width,
			    pgram->border_width);
  dia_color_selector_set_color(pgram_properties_dialog->fg_color,
			       &pgram->border_color);
  dia_color_selector_set_color(pgram_properties_dialog->bg_color,
			       &pgram->inner_color);
  gtk_toggle_button_set_active(pgram_properties_dialog->show_background, 
			       pgram->show_background);
  dia_line_style_selector_set_linestyle(pgram_properties_dialog->line_style,
					pgram->line_style, pgram->dashlength);
  gtk_spin_button_set_value(pgram_properties_dialog->shear_angle,
			    pgram->shear_angle);

  gtk_spin_button_set_value(pgram_properties_dialog->padding,
			    pgram->padding);
  dia_font_selector_set_font(pgram_properties_dialog->font, pgram->text->font);
  gtk_spin_button_set_value(pgram_properties_dialog->font_size,
			    pgram->text->height);
  dia_color_selector_set_color(pgram_properties_dialog->font_color,
			       &pgram->text->color);
  
  return pgram_properties_dialog->vbox;
}
static void
pgram_apply_defaults()
{
  default_properties.shear_angle = M_PI/180.0 * gtk_spin_button_get_value_as_float(pgram_defaults_dialog->shear_angle);
  default_properties.show_background = gtk_toggle_button_get_active(pgram_defaults_dialog->show_background);

  default_properties.padding = gtk_spin_button_get_value_as_float(pgram_defaults_dialog->padding);
  default_properties.font = dia_font_selector_get_font(pgram_defaults_dialog->font);
  default_properties.font_size = gtk_spin_button_get_value_as_float(pgram_defaults_dialog->font_size);
}

static void
init_default_values() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    default_properties.shear_angle = 70.0;
    default_properties.padding = 0.5;
    default_properties.font = font_getfont("Courier");
    default_properties.font_size = 0.8;
    defaults_initialized = 1;
  }
}

static GtkWidget *
pgram_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *checkpgram;
  GtkWidget *shear_angle;
  GtkWidget *padding;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkAdjustment *adj;

  if (pgram_defaults_dialog == NULL) {
  
    init_default_values();

    pgram_defaults_dialog = g_new(PgramDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    pgram_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    checkpgram = gtk_check_button_new_with_label(_("Draw background"));
    pgram_defaults_dialog->show_background = GTK_TOGGLE_BUTTON( checkpgram );
    gtk_widget_show(checkpgram);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkpgram, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Shear angle:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(60.0, 45.0, 135.0, 1.0,0.0,0.0);
    shear_angle = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(shear_angle), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(shear_angle), TRUE);
    pgram_defaults_dialog->shear_angle = GTK_SPIN_BUTTON(shear_angle);
    gtk_box_pack_start(GTK_BOX (hbox), shear_angle, TRUE, TRUE, 0);
    gtk_widget_show (shear_angle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Text padding:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.0, 10.0, 0.1, 0.0, 0.0);
    padding = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(padding), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(padding), TRUE);
    pgram_defaults_dialog->padding = GTK_SPIN_BUTTON(padding);
    gtk_box_pack_start(GTK_BOX (hbox), padding, TRUE, TRUE, 0);
    gtk_widget_show (padding);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    pgram_defaults_dialog->font = DIAFONTSELECTOR(font);
    gtk_box_pack_start (GTK_BOX (hbox), font, TRUE, TRUE, 0);
    gtk_widget_show (font);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font size:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.1, 10.0, 0.1, 0.0, 0.0);
    font_size = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(font_size), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(font_size), TRUE);
    pgram_defaults_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
    gtk_widget_show (vbox);
  }

  gtk_toggle_button_set_active(pgram_defaults_dialog->show_background, 
			       default_properties.show_background);
  gtk_spin_button_set_value(pgram_defaults_dialog->shear_angle, 
			    default_properties.shear_angle);

  gtk_spin_button_set_value(pgram_defaults_dialog->padding,
			    default_properties.padding);
  dia_font_selector_set_font(pgram_defaults_dialog->font,
			     default_properties.font);
  gtk_spin_button_set_value(pgram_defaults_dialog->font_size,
			    default_properties.font_size);

  return pgram_defaults_dialog->vbox;
}

static real
pgram_distance_from(Pgram *pgram, Point *point)
{
  Element *elem = &pgram->element;
  Rectangle rect;

  rect.left = elem->corner.x - pgram->border_width/2;
  rect.right = elem->corner.x + elem->width + pgram->border_width/2;
  rect.top = elem->corner.y - pgram->border_width/2;
  rect.bottom = elem->corner.y + elem->height + pgram->border_width/2;

  /* we do some fiddling with the left/right values to get good accuracy
   * without having to write a new distance checking routine */
  if (rect.top > point->y) {
    /* point above parallelogram */
    if (pgram->shear_grad > 0)
      rect.left  += pgram->shear_grad * (rect.bottom - rect.top);
    else
      rect.right += pgram->shear_grad * (rect.bottom - rect.top);
  } else if (rect.bottom < point->y) {
    /* point below parallelogram */
    if (pgram->shear_grad > 0)
      rect.right -= pgram->shear_grad * (rect.bottom - rect.top);
    else
      rect.left  -= pgram->shear_grad * (rect.bottom - rect.top);
  } else {
    /* point withing vertical interval of parallelogram -- modify
     * left and right sides to `unshear' the parallelogram.  This
     * increases accuracy for points near the  */
    if (pgram->shear_grad > 0) {
      rect.left  += pgram->shear_grad * (rect.bottom - point->y);
      rect.right -= pgram->shear_grad * (point->y - rect.top);
    } else {
      rect.left  -= pgram->shear_grad * (point->y - rect.top);
      rect.right += pgram->shear_grad * (rect.bottom - point->y);
    }
  }

  return distance_rectangle_point(&rect, point);
}

static void
pgram_select(Pgram *pgram, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  text_set_cursor(pgram->text, clicked_point, interactive_renderer);
  text_grab_focus(pgram->text, (Object *)pgram);

  element_update_handles(&pgram->element);
}

static void
pgram_move_handle(Pgram *pgram, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  assert(pgram!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&pgram->element, handle->id, to, reason);

  switch (handle->id) {
  case HANDLE_RESIZE_NW:
    horiz = ANCHOR_END; vert = ANCHOR_END; break;
  case HANDLE_RESIZE_N:
    vert = ANCHOR_END; break;
  case HANDLE_RESIZE_NE:
    horiz = ANCHOR_START; vert = ANCHOR_END; break;
  case HANDLE_RESIZE_E:
    horiz = ANCHOR_START; break;
  case HANDLE_RESIZE_SE:
    horiz = ANCHOR_START; vert = ANCHOR_START; break;
  case HANDLE_RESIZE_S:
    vert = ANCHOR_START; break;
  case HANDLE_RESIZE_SW:
    horiz = ANCHOR_END; vert = ANCHOR_START; break;
  case HANDLE_RESIZE_W:
    horiz = ANCHOR_END; break;
  default:
    break;
  }
  pgram_update_data(pgram, horiz, vert);
}

static void
pgram_move(Pgram *pgram, Point *to)
{
  pgram->element.corner = *to;
  
  pgram_update_data(pgram, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
pgram_draw(Pgram *pgram, Renderer *renderer)
{
  Point pts[4];
  Element *elem;
  real offs;
  
  assert(pgram != NULL);
  assert(renderer != NULL);

  elem = &pgram->element;

  pts[0] = pts[1] = pts[2] = pts[3] = elem->corner;
  pts[1].x += elem->width;
  pts[2].x += elem->width;
  pts[2].y += elem->height;
  pts[3].y += elem->height;

  offs = elem->height * pgram->shear_grad;
  if (offs > 0) {
    pts[0].x += offs;
    pts[2].x -= offs;
  } else {
    pts[1].x += offs;
    pts[3].x -= offs;
  }

  if (pgram->show_background) {
    renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  
    renderer->ops->fill_polygon(renderer, 
				pts, 4,
				&pgram->inner_color);
  }

  renderer->ops->set_linewidth(renderer, pgram->border_width);
  renderer->ops->set_linestyle(renderer, pgram->line_style);
  renderer->ops->set_dashlength(renderer, pgram->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_polygon(renderer, 
			      pts, 4,
			      &pgram->border_color);

  text_draw(pgram->text, renderer);
}

static PgramState *
pgram_get_state(Pgram *pgram)
{
  PgramState *state = g_new(PgramState, 1);

  state->obj_state.free = NULL;
  
  state->border_width = pgram->border_width;
  state->border_color = pgram->border_color;
  state->inner_color = pgram->inner_color;
  state->show_background = pgram->show_background;
  state->line_style = pgram->line_style;
  state->dashlength = pgram->dashlength;
  state->shear_angle = pgram->shear_angle;
  state->padding = pgram->padding;
  text_get_attributes(pgram->text, &state->text_attrib);

  return state;
}

static void
pgram_set_state(Pgram *pgram, PgramState *state)
{
  pgram->border_width = state->border_width;
  pgram->border_color = state->border_color;
  pgram->inner_color = state->inner_color;
  pgram->show_background = state->show_background;
  pgram->line_style = state->line_style;
  pgram->dashlength = state->dashlength;
  pgram->shear_angle = state->shear_angle;
  pgram->shear_grad = tan(M_PI/2.0 - M_PI/180.0 * pgram->shear_angle);
  pgram->padding = state->padding;
  text_set_attributes(pgram->text, &state->text_attrib);

  g_free(state);
  
  pgram_update_data(pgram, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static void
pgram_update_data(Pgram *pgram, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &pgram->element;
  Object *obj = (Object *) pgram;
  Point center, bottom_right;
  Point p;
  real offs;
  real width, height;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  /* this takes the shearing of the parallelogram into account, so the
   * text can be extend to the edges of the parallelogram */
  height = pgram->text->height * pgram->text->numlines + pgram->padding*2 +
    pgram->border_width;
  if (height > elem->height) elem->height = height;

  width = pgram->text->max_width + pgram->padding*2 + pgram->border_width +
    fabs(pgram->shear_grad) * (elem->height + pgram->text->height
			       * pgram->text->numlines);
  if (width > elem->width) elem->width = width;
  
  /* move shape if necessary ... */
  switch (horiz) {
  case ANCHOR_MIDDLE:
    elem->corner.x = center.x - elem->width/2; break;
  case ANCHOR_END:
    elem->corner.x = bottom_right.x - elem->width; break;
  default:
    break;
  }
  switch (vert) {
  case ANCHOR_MIDDLE:
    elem->corner.y = center.y - elem->height/2; break;
  case ANCHOR_END:
    elem->corner.y = bottom_right.y - elem->height; break;
  default:
    break;
  }

  p = elem->corner;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 - pgram->text->height * pgram->text->numlines / 2 +
    font_ascent(pgram->text->font, pgram->text->height);
  text_set_position(pgram->text, &p);

  /* Update connections: */
  pgram->connections[0].pos.x = elem->corner.x;
  pgram->connections[0].pos.y = elem->corner.y;
  pgram->connections[1].pos.x = elem->corner.x + elem->width / 4.0;
  pgram->connections[1].pos.y = elem->corner.y;
  pgram->connections[2].pos.x = elem->corner.x + elem->width / 2.0;
  pgram->connections[2].pos.y = elem->corner.y;
  pgram->connections[3].pos.x = elem->corner.x + elem->width * 3.0 / 4.0;
  pgram->connections[3].pos.y = elem->corner.y;
  pgram->connections[4].pos.x = elem->corner.x + elem->width;
  pgram->connections[4].pos.y = elem->corner.y;
  pgram->connections[5].pos.x = elem->corner.x;
  pgram->connections[5].pos.y = elem->corner.y + elem->height / 4.0;
  pgram->connections[6].pos.x = elem->corner.x + elem->width;
  pgram->connections[6].pos.y = elem->corner.y + elem->height / 4.0;
  pgram->connections[7].pos.x = elem->corner.x;
  pgram->connections[7].pos.y = elem->corner.y + elem->height / 2.0;
  pgram->connections[8].pos.x = elem->corner.x + elem->width;
  pgram->connections[8].pos.y = elem->corner.y + elem->height / 2.0;
  pgram->connections[9].pos.x = elem->corner.x;
  pgram->connections[9].pos.y = elem->corner.y + elem->height * 3.0 / 4.0;
  pgram->connections[10].pos.x = elem->corner.x + elem->width;
  pgram->connections[10].pos.y = elem->corner.y + elem->height * 3.0 / 4.0;
  pgram->connections[11].pos.x = elem->corner.x;
  pgram->connections[11].pos.y = elem->corner.y + elem->height;
  pgram->connections[12].pos.x = elem->corner.x + elem->width / 4.0;
  pgram->connections[12].pos.y = elem->corner.y + elem->height;
  pgram->connections[13].pos.x = elem->corner.x + elem->width / 2.0;
  pgram->connections[13].pos.y = elem->corner.y + elem->height;
  pgram->connections[14].pos.x = elem->corner.x + elem->width * 3.0 / 4.0;
  pgram->connections[14].pos.y = elem->corner.y + elem->height;
  pgram->connections[15].pos.x = elem->corner.x + elem->width;
  pgram->connections[15].pos.y = elem->corner.y + elem->height;

  offs = elem->height / 4.0 * pgram->shear_grad;
  if (offs > 0) {
    pgram->connections[0].pos.x += 4*offs;
    pgram->connections[1].pos.x += 3*offs;
    pgram->connections[2].pos.x += 2*offs;
    pgram->connections[3].pos.x += offs;
    pgram->connections[5].pos.x += 3*offs;
    pgram->connections[6].pos.x -= offs;
    pgram->connections[7].pos.x += 2*offs;
    pgram->connections[8].pos.x -= 2*offs;
    pgram->connections[9].pos.x += offs;
    pgram->connections[10].pos.x -= 3*offs;
    pgram->connections[12].pos.x -= offs;
    pgram->connections[13].pos.x -= 2*offs;
    pgram->connections[14].pos.x -= 3*offs;
    pgram->connections[15].pos.x -= 4*offs;
  } else {
    pgram->connections[1].pos.x += offs;
    pgram->connections[2].pos.x += 2*offs;
    pgram->connections[3].pos.x += 3*offs;
    pgram->connections[4].pos.x += 4*offs;
    pgram->connections[5].pos.x -= offs;
    pgram->connections[6].pos.x += 3*offs;
    pgram->connections[7].pos.x -= 2*offs;
    pgram->connections[8].pos.x += 2*offs;
    pgram->connections[9].pos.x -= 3*offs;
    pgram->connections[10].pos.x += offs;
    pgram->connections[11].pos.x -= 4*offs;
    pgram->connections[12].pos.x -= 3*offs;
    pgram->connections[13].pos.x -= 2*offs;
    pgram->connections[14].pos.x -= offs;
  }

  element_update_boundingbox(elem);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= pgram->border_width/2;
  obj->bounding_box.left -= pgram->border_width/2;
  obj->bounding_box.bottom += pgram->border_width/2;
  obj->bounding_box.right += pgram->border_width/2;
  
  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static Object *
pgram_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Pgram *pgram;
  Element *elem;
  Object *obj;
  Point p;
  int i;

  init_default_values();

  pgram = g_malloc(sizeof(Pgram));
  elem = &pgram->element;
  obj = (Object *) pgram;
  
  obj->type = &pgram_type;

  obj->ops = &pgram_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  pgram->border_width =  attributes_get_default_linewidth();
  pgram->border_color = attributes_get_foreground();
  pgram->inner_color = attributes_get_background();
  pgram->show_background = default_properties.show_background;
  attributes_get_default_line_style(&pgram->line_style, &pgram->dashlength);
  pgram->shear_angle = default_properties.shear_angle;
  pgram->shear_grad = tan(M_PI/2.0 - M_PI/180.0 * pgram->shear_angle);

  pgram->padding = default_properties.padding;
  
  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + default_properties.font_size / 2;
  pgram->text = new_text("", default_properties.font,
		       default_properties.font_size, &p, &pgram->border_color,
		       ALIGN_CENTER);

  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &pgram->connections[i];
    pgram->connections[i].object = obj;
    pgram->connections[i].connected = NULL;
  }

  pgram_update_data(pgram, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return (Object *)pgram;
}

static void
pgram_destroy(Pgram *pgram)
{
  text_destroy(pgram->text);

  element_destroy(&pgram->element);
}

static Object *
pgram_copy(Pgram *pgram)
{
  int i;
  Pgram *newpgram;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &pgram->element;
  
  newpgram = g_malloc(sizeof(Pgram));
  newelem = &newpgram->element;
  newobj = (Object *) newpgram;

  element_copy(elem, newelem);

  newpgram->border_width = pgram->border_width;
  newpgram->border_color = pgram->border_color;
  newpgram->inner_color = pgram->inner_color;
  newpgram->show_background = pgram->show_background;
  newpgram->line_style = pgram->line_style;
  newpgram->dashlength = pgram->dashlength;
  newpgram->shear_angle = pgram->shear_angle;
  newpgram->shear_grad = pgram->shear_grad;
  newpgram->padding = pgram->padding;

  newpgram->text = text_copy(pgram->text);
  
  for (i=0;i<16;i++) {
    newobj->connections[i] = &newpgram->connections[i];
    newpgram->connections[i].object = newobj;
    newpgram->connections[i].connected = NULL;
    newpgram->connections[i].pos = pgram->connections[i].pos;
    newpgram->connections[i].last_pos = pgram->connections[i].last_pos;
  }

  return (Object *)newpgram;
}

static void
pgram_save(Pgram *pgram, ObjectNode obj_node, const char *filename)
{
  element_save(&pgram->element, obj_node);

  if (pgram->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  pgram->border_width);
  
  if (!color_equals(&pgram->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &pgram->border_color);
  
  if (!color_equals(&pgram->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &pgram->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"), pgram->show_background);

  if (pgram->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  pgram->line_style);

  if (pgram->line_style != LINESTYLE_SOLID &&
      pgram->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  pgram->dashlength);

  data_add_real(new_attribute(obj_node, "shear_angle"),
		pgram->shear_angle);

  data_add_text(new_attribute(obj_node, "text"), pgram->text);
}

static Object *
pgram_load(ObjectNode obj_node, int version, const char *filename)
{
  Pgram *pgram;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  pgram = g_malloc(sizeof(Pgram));
  elem = &pgram->element;
  obj = (Object *) pgram;
  
  obj->type = &pgram_type;
  obj->ops = &pgram_ops;

  element_load(elem, obj_node);
  
  pgram->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    pgram->border_width =  data_real( attribute_first_data(attr) );

  pgram->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &pgram->border_color);
  
  pgram->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &pgram->inner_color);
  
  pgram->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    pgram->show_background = data_boolean( attribute_first_data(attr) );

  pgram->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    pgram->line_style =  data_enum( attribute_first_data(attr) );

  pgram->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    pgram->dashlength = data_real(attribute_first_data(attr));

  pgram->shear_angle = 0.0;
  attr = object_find_attribute(obj_node, "shear_angle");
  if (attr != NULL)
    pgram->shear_angle =  data_real( attribute_first_data(attr) );
  pgram->shear_grad = tan(M_PI/2.0 - M_PI/180.0 * pgram->shear_angle);

  pgram->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    pgram->text = data_text(attribute_first_data(attr));

  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &pgram->connections[i];
    pgram->connections[i].object = obj;
    pgram->connections[i].connected = NULL;
  }

  pgram_update_data(pgram, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return (Object *)pgram;
}
