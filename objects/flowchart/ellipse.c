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

#include "pixmaps/ellipse.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

/* used when resizing to decide which side of the shape to expand/shrink */
typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef struct _Ellipse Ellipse;
typedef struct _EllipsePropertiesDialog EllipsePropertiesDialog;
typedef struct _EllipseDefaultsDialog EllipseDefaultsDialog;
typedef struct _EllipseState EllipseState;

struct _EllipseState {
  ObjectState obj_state;
  
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;

  real padding;
  TextAttributes text_attrib;
};

struct _Ellipse {
  Element element;

  ConnectionPoint connections[16];
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;

  Text *text;
  real padding;
};

typedef struct _EllipseProperties {
  Color *fg_color;
  Color *bg_color;
  gboolean show_background;
  real border_width;

  real padding;
  Font *font;
  real font_size;
  Color *font_color;
} EllipseProperties;

struct _EllipsePropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;
  GtkToggleButton *show_background;
  DiaLineStyleSelector *line_style;

  GtkSpinButton *padding;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
  DiaColorSelector *font_color;

  Ellipse *ellipse;
};

struct _EllipseDefaultsDialog {
  GtkWidget *vbox;

  GtkToggleButton *show_background;

  GtkSpinButton *padding;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
};


static EllipsePropertiesDialog *ellipse_properties_dialog;
static EllipseDefaultsDialog *ellipse_defaults_dialog;
static EllipseProperties default_properties;

static real ellipse_distance_from(Ellipse *ellipse, Point *point);
static void ellipse_select(Ellipse *ellipse, Point *clicked_point,
		       Renderer *interactive_renderer);
static void ellipse_move_handle(Ellipse *ellipse, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
static void ellipse_move(Ellipse *ellipse, Point *to);
static void ellipse_draw(Ellipse *ellipse, Renderer *renderer);
static void ellipse_update_data(Ellipse *ellipse, AnchorShape h,AnchorShape v);
static Object *ellipse_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void ellipse_destroy(Ellipse *ellipse);
static Object *ellipse_copy(Ellipse *ellipse);
static GtkWidget *ellipse_get_properties(Ellipse *ellipse);
static ObjectChange *ellipse_apply_properties(Ellipse *ellipse);

static EllipseState *ellipse_get_state(Ellipse *ellipse);
static void ellipse_set_state(Ellipse *ellipse, EllipseState *state);

static void ellipse_save(Ellipse *ellipse, ObjectNode obj_node, const char *filename);
static Object *ellipse_load(ObjectNode obj_node, int version, const char *filename);
static GtkWidget *ellipse_get_defaults();
static void ellipse_apply_defaults();

static ObjectTypeOps ellipse_type_ops =
{
  (CreateFunc) ellipse_create,
  (LoadFunc)   ellipse_load,
  (SaveFunc)   ellipse_save,
  (GetDefaultsFunc)   ellipse_get_defaults,
  (ApplyDefaultsFunc) ellipse_apply_defaults
};

ObjectType fc_ellipse_type =
{
  "Flowchart - Ellipse",  /* name */
  0,                 /* version */
  (char **) ellipse_xpm, /* pixmap */

  &ellipse_type_ops      /* ops */
};

static ObjectOps ellipse_ops = {
  (DestroyFunc)         ellipse_destroy,
  (DrawFunc)            ellipse_draw,
  (DistanceFunc)        ellipse_distance_from,
  (SelectFunc)          ellipse_select,
  (CopyFunc)            ellipse_copy,
  (MoveFunc)            ellipse_move,
  (MoveHandleFunc)      ellipse_move_handle,
  (GetPropertiesFunc)   ellipse_get_properties,
  (ApplyPropertiesFunc) ellipse_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
ellipse_apply_properties(Ellipse *ellipse)
{
  ObjectState *old_state;
  Font *font;
  Color col;

  if (ellipse != ellipse_properties_dialog->ellipse) {
    message_warning("Ellipse dialog problem:  %p != %p\n", ellipse,
		    ellipse_properties_dialog->ellipse);
    ellipse = ellipse_properties_dialog->ellipse;
  }

  old_state = (ObjectState *)ellipse_get_state(ellipse);
  
  ellipse->border_width = gtk_spin_button_get_value_as_float(ellipse_properties_dialog->border_width);
  dia_color_selector_get_color(ellipse_properties_dialog->fg_color, &ellipse->border_color);
  dia_color_selector_get_color(ellipse_properties_dialog->bg_color, &ellipse->inner_color);
  ellipse->show_background = gtk_toggle_button_get_active(ellipse_properties_dialog->show_background);
  dia_line_style_selector_get_linestyle(ellipse_properties_dialog->line_style, &ellipse->line_style, &ellipse->dashlength);

  ellipse->padding = gtk_spin_button_get_value_as_float(ellipse_properties_dialog->padding);
  font = dia_font_selector_get_font(ellipse_properties_dialog->font);
  text_set_font(ellipse->text, font);
  text_set_height(ellipse->text, gtk_spin_button_get_value_as_float(
					ellipse_properties_dialog->font_size));
  dia_color_selector_get_color(ellipse_properties_dialog->font_color, &col);
  text_set_color(ellipse->text, &col);
  
  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return new_object_state_change((Object *)ellipse, old_state, 
				 (GetStateFunc)ellipse_get_state,
				 (SetStateFunc)ellipse_set_state);
}

static GtkWidget *
ellipse_get_properties(Ellipse *ellipse)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *checkellipse;
  GtkWidget *linestyle;
  GtkWidget *border_width;
  GtkWidget *padding;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkAdjustment *adj;

  if (ellipse_properties_dialog == NULL) {
    ellipse_properties_dialog = g_new(EllipsePropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    ellipse_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Border width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    ellipse_properties_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Foreground color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    ellipse_properties_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Background color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    ellipse_properties_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkellipse = gtk_check_button_new_with_label(_("Draw background"));
    ellipse_properties_dialog->show_background = GTK_TOGGLE_BUTTON( checkellipse );
    gtk_widget_show(checkellipse);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkellipse, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    ellipse_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
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
    ellipse_properties_dialog->padding = GTK_SPIN_BUTTON(padding);
    gtk_box_pack_start(GTK_BOX (hbox), padding, TRUE, TRUE, 0);
    gtk_widget_show (padding);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    ellipse_properties_dialog->font = DIAFONTSELECTOR(font);
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
    ellipse_properties_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    ellipse_properties_dialog->font_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  ellipse_properties_dialog->ellipse = ellipse;
    
  gtk_spin_button_set_value(ellipse_properties_dialog->border_width,
			    ellipse->border_width);
  dia_color_selector_set_color(ellipse_properties_dialog->fg_color,
			       &ellipse->border_color);
  dia_color_selector_set_color(ellipse_properties_dialog->bg_color,
			       &ellipse->inner_color);
  gtk_toggle_button_set_active(ellipse_properties_dialog->show_background, 
			       ellipse->show_background);
  dia_line_style_selector_set_linestyle(ellipse_properties_dialog->line_style,
					ellipse->line_style,
					ellipse->dashlength);

  gtk_spin_button_set_value(ellipse_properties_dialog->padding,
			    ellipse->padding);
  dia_font_selector_set_font(ellipse_properties_dialog->font, ellipse->text->font);
  gtk_spin_button_set_value(ellipse_properties_dialog->font_size,
			    ellipse->text->height);
  dia_color_selector_set_color(ellipse_properties_dialog->font_color,
			       &ellipse->text->color);
  
  return ellipse_properties_dialog->vbox;
}
static void
ellipse_apply_defaults()
{
  default_properties.show_background = gtk_toggle_button_get_active(ellipse_defaults_dialog->show_background);

  default_properties.padding = gtk_spin_button_get_value_as_float(ellipse_defaults_dialog->padding);
  default_properties.font = dia_font_selector_get_font(ellipse_defaults_dialog->font);
  default_properties.font_size = gtk_spin_button_get_value_as_float(ellipse_defaults_dialog->font_size);
}

static void
init_default_values() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    default_properties.padding = 0.5 * M_SQRT1_2;
    default_properties.font = font_getfont("Courier");
    default_properties.font_size = 0.8;
    defaults_initialized = 1;
  }
}

static GtkWidget *
ellipse_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *checkellipse;
  GtkWidget *padding;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkAdjustment *adj;

  if (ellipse_defaults_dialog == NULL) {
  
    init_default_values();

    ellipse_defaults_dialog = g_new(EllipseDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    ellipse_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    checkellipse = gtk_check_button_new_with_label(_("Draw background"));
    ellipse_defaults_dialog->show_background = GTK_TOGGLE_BUTTON( checkellipse );
    gtk_widget_show(checkellipse);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkellipse, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Text padding:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.0, 10.0, 0.1, 0.0, 0.0);
    padding = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(padding), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(padding), TRUE);
    ellipse_defaults_dialog->padding = GTK_SPIN_BUTTON(padding);
    gtk_box_pack_start(GTK_BOX (hbox), padding, TRUE, TRUE, 0);
    gtk_widget_show (padding);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    ellipse_defaults_dialog->font = DIAFONTSELECTOR(font);
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
    ellipse_defaults_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
    gtk_widget_show (vbox);
  }

  gtk_toggle_button_set_active(ellipse_defaults_dialog->show_background, 
			       default_properties.show_background);

  gtk_spin_button_set_value(ellipse_defaults_dialog->padding,
			    default_properties.padding);
  dia_font_selector_set_font(ellipse_defaults_dialog->font,
			     default_properties.font);
  gtk_spin_button_set_value(ellipse_defaults_dialog->font_size,
			    default_properties.font_size);

  return ellipse_defaults_dialog->vbox;
}

/* returns the radius of the ellipse along the ray from the centre of the
 * ellipse to the point (px, py) */
static real
ellipse_radius(Ellipse *ellipse, real px, real py)
{
  Element *elem = &ellipse->element;
  real w2 = elem->width * elem->width;
  real h2 = elem->height * elem->height;
  real scale;

  /* find the point of intersection between line (x=cx+(px-cx)t; y=cy+(py-cy)t)
   * and ellipse ((x-cx)^2)/(w/2)^2 + ((y-cy)^2)/(h/2)^2 = 1 */
  /* radius along ray is sqrt((px-cx)^2 * t^2 + (py-cy)^2 * t^2) */

  /* normalize coordinates ... */
  px -= elem->corner.x + elem->width  / 2;
  py -= elem->corner.y + elem->height / 2;
  /* square them ... */
  px *= px;
  py *= py;

  scale = w2 * h2 / (4*h2*px + 4*w2*py);
  return sqrt((px + py)*scale);
}

static real
ellipse_distance_from(Ellipse *ellipse, Point *point)
{
  Element *elem = &ellipse->element;
  Point c;
  real dist, rad;

  c.x = elem->corner.x + elem->width / 2;
  c.y = elem->corner.y + elem->height/ 2;
  dist = distance_point_point(point, &c);
  rad = ellipse_radius(ellipse, point->x, point->y) + ellipse->border_width/2;

  if (dist <= rad)
    return 0;
  return dist - rad;
}

static void
ellipse_select(Ellipse *ellipse, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  text_set_cursor(ellipse->text, clicked_point, interactive_renderer);
  text_grab_focus(ellipse->text, (Object *)ellipse);

  element_update_handles(&ellipse->element);
}

static void
ellipse_move_handle(Ellipse *ellipse, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  assert(ellipse!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&ellipse->element, handle->id, to, reason);

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
  ellipse_update_data(ellipse, horiz, vert);
}

static void
ellipse_move(Ellipse *ellipse, Point *to)
{
  ellipse->element.corner = *to;
  
  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
ellipse_draw(Ellipse *ellipse, Renderer *renderer)
{
  Element *elem;
  Point center;
  
  assert(ellipse != NULL);
  assert(renderer != NULL);

  elem = &ellipse->element;

  center.x = elem->corner.x + elem->width/2;
  center.y = elem->corner.y + elem->height/2;

  if (ellipse->show_background) {
    renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  
    renderer->ops->fill_ellipse(renderer, &center,
				elem->width, elem->height,
				&ellipse->inner_color);
  }

  renderer->ops->set_linewidth(renderer, ellipse->border_width);
  renderer->ops->set_linestyle(renderer, ellipse->line_style);
  renderer->ops->set_dashlength(renderer, ellipse->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_ellipse(renderer, &center,
			      elem->width, elem->height,
			      &ellipse->border_color);

  text_draw(ellipse->text, renderer);
}

static EllipseState *
ellipse_get_state(Ellipse *ellipse)
{
  EllipseState *state = g_new(EllipseState, 1);

  state->obj_state.free = NULL;
  
  state->border_width = ellipse->border_width;
  state->border_color = ellipse->border_color;
  state->inner_color = ellipse->inner_color;
  state->show_background = ellipse->show_background;
  state->line_style = ellipse->line_style;
  state->dashlength = ellipse->dashlength;
  state->padding = ellipse->padding;
  text_get_attributes(ellipse->text, &state->text_attrib);

  return state;
}

static void
ellipse_set_state(Ellipse *ellipse, EllipseState *state)
{
  ellipse->border_width = state->border_width;
  ellipse->border_color = state->border_color;
  ellipse->inner_color = state->inner_color;
  ellipse->show_background = state->show_background;
  ellipse->line_style = state->line_style;
  ellipse->dashlength = state->dashlength;
  ellipse->padding = state->padding;
  text_set_attributes(ellipse->text, &state->text_attrib);

  g_free(state);
  
  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static void
ellipse_update_data(Ellipse *ellipse, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &ellipse->element;
  Object *obj = (Object *) ellipse;
  Point center, bottom_right;
  Point p, c;
  real dw, dh;
  real width, height;
  real radius1, radius2;
  int i;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  width = ellipse->text->max_width + 2 * ellipse->padding;
  height = ellipse->text->height * ellipse->text->numlines +
    2 * ellipse->padding;

  /* stop ellipse from getting infinite width/height */
  if (elem->width / elem->height > 4)
    elem->width = elem->height * 4;
  else if (elem->height / elem->width > 4)
    elem->height = elem->width * 4;

  c.x = elem->corner.x + elem->width / 2;
  c.y = elem->corner.y + elem->height / 2;
  p.x = c.x - width  / 2;
  p.y = c.y - height / 2;
  radius1 = ellipse_radius(ellipse, p.x, p.y) - ellipse->border_width/2;
  radius2 = distance_point_point(&c, &p);
  
  if (radius1 < radius2) {
    /* increase size of the ellipse while keeping its aspect ratio */
    elem->width  *= radius2 / radius1;
    elem->height *= radius2 / radius1;
  }

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
  p.y += elem->height / 2.0 - ellipse->text->height*ellipse->text->numlines/2 +
    font_ascent(ellipse->text->font, ellipse->text->height);
  text_set_position(ellipse->text, &p);

  /* Update connections: */
  c.x = elem->corner.x + elem->width / 2;
  c.y = elem->corner.y + elem->height / 2;
  dw = elem->width  / 2.0;
  dh = elem->height / 2.0;
  for (i = 0; i < 16; i++) {
    real theta = M_PI / 8.0 * i;
    ellipse->connections[i].pos.x = c.x + dw * cos(theta);
    ellipse->connections[i].pos.y = c.y - dh * sin(theta);
  }

  element_update_boundingbox(elem);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= ellipse->border_width/2;
  obj->bounding_box.left -= ellipse->border_width/2;
  obj->bounding_box.bottom += ellipse->border_width/2;
  obj->bounding_box.right += ellipse->border_width/2;
  
  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static Object *
ellipse_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Ellipse *ellipse;
  Element *elem;
  Object *obj;
  Point p;
  int i;

  init_default_values();

  ellipse = g_malloc(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = (Object *) ellipse;
  
  obj->type = &fc_ellipse_type;

  obj->ops = &ellipse_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  ellipse->border_width =  attributes_get_default_linewidth();
  ellipse->border_color = attributes_get_foreground();
  ellipse->inner_color = attributes_get_background();
  ellipse->show_background = default_properties.show_background;
  attributes_get_default_line_style(&ellipse->line_style,&ellipse->dashlength);

  ellipse->padding = default_properties.padding;
  
  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + default_properties.font_size / 2;
  ellipse->text = new_text("", default_properties.font,
		       default_properties.font_size, &p, &ellipse->border_color,
		       ALIGN_CENTER);

  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }

  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return (Object *)ellipse;
}

static void
ellipse_destroy(Ellipse *ellipse)
{
  text_destroy(ellipse->text);

  element_destroy(&ellipse->element);
}

static Object *
ellipse_copy(Ellipse *ellipse)
{
  int i;
  Ellipse *newellipse;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &ellipse->element;
  
  newellipse = g_malloc(sizeof(Ellipse));
  newelem = &newellipse->element;
  newobj = (Object *) newellipse;

  element_copy(elem, newelem);

  newellipse->border_width = ellipse->border_width;
  newellipse->border_color = ellipse->border_color;
  newellipse->inner_color = ellipse->inner_color;
  newellipse->show_background = ellipse->show_background;
  newellipse->line_style = ellipse->line_style;
  newellipse->dashlength = ellipse->dashlength;
  newellipse->padding = ellipse->padding;

  newellipse->text = text_copy(ellipse->text);
  
  for (i=0;i<16;i++) {
    newobj->connections[i] = &newellipse->connections[i];
    newellipse->connections[i].object = newobj;
    newellipse->connections[i].connected = NULL;
    newellipse->connections[i].pos = ellipse->connections[i].pos;
    newellipse->connections[i].last_pos = ellipse->connections[i].last_pos;
  }

  return (Object *)newellipse;
}

static void
ellipse_save(Ellipse *ellipse, ObjectNode obj_node, const char *filename)
{
  element_save(&ellipse->element, obj_node);

  if (ellipse->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  ellipse->border_width);
  
  if (!color_equals(&ellipse->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &ellipse->border_color);
  
  if (!color_equals(&ellipse->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &ellipse->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"), ellipse->show_background);

  if (ellipse->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  ellipse->line_style);

  if (ellipse->line_style != LINESTYLE_SOLID &&
      ellipse->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  ellipse->dashlength);

  data_add_real(new_attribute(obj_node, "padding"), ellipse->padding);

  data_add_text(new_attribute(obj_node, "text"), ellipse->text);
}

static Object *
ellipse_load(ObjectNode obj_node, int version, const char *filename)
{
  Ellipse *ellipse;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  ellipse = g_malloc(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = (Object *) ellipse;
  
  obj->type = &fc_ellipse_type;
  obj->ops = &ellipse_ops;

  element_load(elem, obj_node);
  
  ellipse->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    ellipse->border_width =  data_real( attribute_first_data(attr) );

  ellipse->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->border_color);
  
  ellipse->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->inner_color);
  
  ellipse->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    ellipse->show_background = data_boolean( attribute_first_data(attr) );

  ellipse->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    ellipse->line_style =  data_enum( attribute_first_data(attr) );

  ellipse->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    ellipse->dashlength = data_real(attribute_first_data(attr));

  ellipse->padding = default_properties.padding;
  attr = object_find_attribute(obj_node, "padding");
  if (attr != NULL)
    ellipse->padding =  data_real( attribute_first_data(attr) );
  
  ellipse->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    ellipse->text = data_text(attribute_first_data(attr));

  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }

  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return (Object *)ellipse;
}
