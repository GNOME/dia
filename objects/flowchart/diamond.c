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

#include "pixmaps/diamond.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

/* used when resizing to decide which side of the shape to expand/shrink */
typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef struct _Diamond Diamond;
typedef struct _DiamondPropertiesDialog DiamondPropertiesDialog;
typedef struct _DiamondDefaultsDialog DiamondDefaultsDialog;
typedef struct _DiamondState DiamondState;

struct _DiamondState {
  ObjectState obj_state;
  
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;

  real padding;
  TextAttributes text_attrib;
};

struct _Diamond {
  Element element;

  ConnectionPoint connections[16];
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;

  Text *text;
  real padding;
};

typedef struct _DiamondProperties {
  Color *fg_color;
  Color *bg_color;
  gboolean show_background;
  real border_width;
  LineStyle line_style;

  real padding;
  Font *font;
  real font_size;
  Color *font_color;
} DiamondProperties;

struct _DiamondPropertiesDialog {
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

  Diamond *diamond;
};

struct _DiamondDefaultsDialog {
  GtkWidget *vbox;

  GtkToggleButton *show_background;
  DiaLineStyleSelector *line_style;

  GtkSpinButton *padding;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
};


static DiamondPropertiesDialog *diamond_properties_dialog;
static DiamondDefaultsDialog *diamond_defaults_dialog;
static DiamondProperties default_properties;

static real diamond_distance_from(Diamond *diamond, Point *point);
static void diamond_select(Diamond *diamond, Point *clicked_point,
		       Renderer *interactive_renderer);
static void diamond_move_handle(Diamond *diamond, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
static void diamond_move(Diamond *diamond, Point *to);
static void diamond_draw(Diamond *diamond, Renderer *renderer);
static void diamond_update_data(Diamond *diamond, AnchorShape h,AnchorShape v);
static Object *diamond_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void diamond_destroy(Diamond *diamond);
static Object *diamond_copy(Diamond *diamond);
static GtkWidget *diamond_get_properties(Diamond *diamond);
static ObjectChange *diamond_apply_properties(Diamond *diamond);

static DiamondState *diamond_get_state(Diamond *diamond);
static void diamond_set_state(Diamond *diamond, DiamondState *state);

static void diamond_save(Diamond *diamond, ObjectNode obj_node, const char *filename);
static Object *diamond_load(ObjectNode obj_node, int version, const char *filename);
static GtkWidget *diamond_get_defaults();
static void diamond_apply_defaults();

static ObjectTypeOps diamond_type_ops =
{
  (CreateFunc) diamond_create,
  (LoadFunc)   diamond_load,
  (SaveFunc)   diamond_save,
  (GetDefaultsFunc)   diamond_get_defaults,
  (ApplyDefaultsFunc) diamond_apply_defaults
};

ObjectType diamond_type =
{
  "Flowchart - Diamond",  /* name */
  0,                 /* version */
  (char **) diamond_xpm, /* pixmap */

  &diamond_type_ops      /* ops */
};

SheetObject diamond_sheetobj =
{
  "Flowchart - Diamond",
  N_("A diamond with text inside."),
  (char **)diamond_xpm,
  NULL
};

static ObjectOps diamond_ops = {
  (DestroyFunc)         diamond_destroy,
  (DrawFunc)            diamond_draw,
  (DistanceFunc)        diamond_distance_from,
  (SelectFunc)          diamond_select,
  (CopyFunc)            diamond_copy,
  (MoveFunc)            diamond_move,
  (MoveHandleFunc)      diamond_move_handle,
  (GetPropertiesFunc)   diamond_get_properties,
  (ApplyPropertiesFunc) diamond_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
diamond_apply_properties(Diamond *diamond)
{
  ObjectState *old_state;
  Font *font;
  Color col;

  if (diamond != diamond_properties_dialog->diamond) {
    message_warning("Diamond dialog problem:  %p != %p\n", diamond,
		    diamond_properties_dialog->diamond);
    diamond = diamond_properties_dialog->diamond;
  }

  old_state = (ObjectState *)diamond_get_state(diamond);
  
  diamond->border_width = gtk_spin_button_get_value_as_float(diamond_properties_dialog->border_width);
  dia_color_selector_get_color(diamond_properties_dialog->fg_color, &diamond->border_color);
  dia_color_selector_get_color(diamond_properties_dialog->bg_color, &diamond->inner_color);
  diamond->show_background = gtk_toggle_button_get_active(diamond_properties_dialog->show_background);
  dia_line_style_selector_get_linestyle(diamond_properties_dialog->line_style, &diamond->line_style, NULL);

  diamond->padding = gtk_spin_button_get_value_as_float(diamond_properties_dialog->padding);
  font = dia_font_selector_get_font(diamond_properties_dialog->font);
  text_set_font(diamond->text, font);
  text_set_height(diamond->text, gtk_spin_button_get_value_as_float(
					diamond_properties_dialog->font_size));
  dia_color_selector_get_color(diamond_properties_dialog->font_color, &col);
  text_set_color(diamond->text, &col);
  
  diamond_update_data(diamond, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return new_object_state_change((Object *)diamond, old_state, 
				 (GetStateFunc)diamond_get_state,
				 (SetStateFunc)diamond_set_state);
}

static GtkWidget *
diamond_get_properties(Diamond *diamond)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *checkdiamond;
  GtkWidget *linestyle;
  GtkWidget *border_width;
  GtkWidget *padding;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkWidget *font_color;
  GtkAdjustment *adj;

  if (diamond_properties_dialog == NULL) {
    diamond_properties_dialog = g_new(DiamondPropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    diamond_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Border width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    diamond_properties_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Foreground color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    diamond_properties_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Background color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    diamond_properties_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkdiamond = gtk_check_button_new_with_label(_("Draw background"));
    diamond_properties_dialog->show_background = GTK_TOGGLE_BUTTON( checkdiamond );
    gtk_widget_show(checkdiamond);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkdiamond, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    diamond_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
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
    diamond_properties_dialog->padding = GTK_SPIN_BUTTON(padding);
    gtk_box_pack_start(GTK_BOX (hbox), padding, TRUE, TRUE, 0);
    gtk_widget_show (padding);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    diamond_properties_dialog->font = DIAFONTSELECTOR(font);
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
    diamond_properties_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    diamond_properties_dialog->font_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  diamond_properties_dialog->diamond = diamond;
    
  gtk_spin_button_set_value(diamond_properties_dialog->border_width,
			    diamond->border_width);
  dia_color_selector_set_color(diamond_properties_dialog->fg_color,
			       &diamond->border_color);
  dia_color_selector_set_color(diamond_properties_dialog->bg_color,
			       &diamond->inner_color);
  gtk_toggle_button_set_active(diamond_properties_dialog->show_background, 
			       diamond->show_background);
  dia_line_style_selector_set_linestyle(diamond_properties_dialog->line_style,
					diamond->line_style, 1.0);

  gtk_spin_button_set_value(diamond_properties_dialog->padding,
			    diamond->padding);
  dia_font_selector_set_font(diamond_properties_dialog->font, diamond->text->font);
  gtk_spin_button_set_value(diamond_properties_dialog->font_size,
			    diamond->text->height);
  dia_color_selector_set_color(diamond_properties_dialog->font_color,
			       &diamond->text->color);
  
  return diamond_properties_dialog->vbox;
}
static void
diamond_apply_defaults()
{
  dia_line_style_selector_get_linestyle(diamond_defaults_dialog->line_style,
					&default_properties.line_style, NULL);
  default_properties.show_background = gtk_toggle_button_get_active(diamond_defaults_dialog->show_background);

  default_properties.padding = gtk_spin_button_get_value_as_float(diamond_defaults_dialog->padding);
  default_properties.font = dia_font_selector_get_font(diamond_defaults_dialog->font);
  default_properties.font_size = gtk_spin_button_get_value_as_float(diamond_defaults_dialog->font_size);
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
diamond_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *checkdiamond;
  GtkWidget *linestyle;
  GtkWidget *padding;
  GtkWidget *font;
  GtkWidget *font_size;
  GtkAdjustment *adj;

  if (diamond_defaults_dialog == NULL) {
  
    init_default_values();

    diamond_defaults_dialog = g_new(DiamondDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    diamond_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    checkdiamond = gtk_check_button_new_with_label(_("Draw background"));
    diamond_defaults_dialog->show_background = GTK_TOGGLE_BUTTON( checkdiamond );
    gtk_widget_show(checkdiamond);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkdiamond, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    diamond_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
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
    diamond_defaults_dialog->padding = GTK_SPIN_BUTTON(padding);
    gtk_box_pack_start(GTK_BOX (hbox), padding, TRUE, TRUE, 0);
    gtk_widget_show (padding);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Font:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    font = dia_font_selector_new();
    diamond_defaults_dialog->font = DIAFONTSELECTOR(font);
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
    diamond_defaults_dialog->font_size = GTK_SPIN_BUTTON(font_size);
    gtk_box_pack_start(GTK_BOX (hbox), font_size, TRUE, TRUE, 0);
    gtk_widget_show (font_size);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
    gtk_widget_show (vbox);
  }

  gtk_toggle_button_set_active(diamond_defaults_dialog->show_background, 
			       default_properties.show_background);
  dia_line_style_selector_set_linestyle(diamond_defaults_dialog->line_style,
					default_properties.line_style, 1.0);

  gtk_spin_button_set_value(diamond_defaults_dialog->padding,
			    default_properties.padding);
  dia_font_selector_set_font(diamond_defaults_dialog->font,
			     default_properties.font);
  gtk_spin_button_set_value(diamond_defaults_dialog->font_size,
			    default_properties.font_size);

  return diamond_defaults_dialog->vbox;
}

static real
diamond_distance_from(Diamond *diamond, Point *point)
{
  Element *elem = &diamond->element;
  Rectangle rect;

  rect.left = elem->corner.x - diamond->border_width/2;
  rect.right = elem->corner.x + elem->width + diamond->border_width/2;
  rect.top = elem->corner.y - diamond->border_width/2;
  rect.bottom = elem->corner.y + elem->height + diamond->border_width/2;

  if (rect.top > point->y)
    return rect.top - point->y +
      fabs(point->x - elem->corner.x + elem->width / 2.0);
  else if (point->y > rect.bottom)
    return rect.bottom-point->y +
      fabs(point->x - elem->corner.x + elem->width / 2.0);
  else if (rect.left > point->x)
    return rect.left - point->y +
      fabs(point->y - elem->corner.y + elem->height / 2.0);
  else if (point->x > rect.right)
    return point->y - rect.right +
      fabs(point->y - elem->corner.y + elem->height / 2.0);
  else {
    /* inside the bounding box of diamond ... this is where it gets harder */
    real x = point->x, y = point->y;
    real dx, dy;

    /* reflect point into upper left quadrant of diamond */
    if (x > elem->corner.x + elem->width / 2.0)
      x = 2 * (elem->corner.x + elem->width / 2.0) - x;
    if (y > elem->corner.y + elem->height / 2.0)
      y = 2 * (elem->corner.y + elem->height / 2.0) - y;

    dx = -x + elem->corner.x + elem->width / 2.0 -
      elem->width/elem->height * (y-elem->corner.y) - diamond->border_width/2;
    dy = -y + elem->corner.y + elem->height / 2.0 -
      elem->height/elem->width * (x-elem->corner.x) - diamond->border_width/2;
    if (dx <= 0 || dy <= 0)
      return 0;
    return MIN(dx, dy);
  }
}

static void
diamond_select(Diamond *diamond, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  text_set_cursor(diamond->text, clicked_point, interactive_renderer);
  text_grab_focus(diamond->text, (Object *)diamond);

  element_update_handles(&diamond->element);
}

static void
diamond_move_handle(Diamond *diamond, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  assert(diamond!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&diamond->element, handle->id, to, reason);

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
  diamond_update_data(diamond, horiz, vert);
}

static void
diamond_move(Diamond *diamond, Point *to)
{
  diamond->element.corner = *to;
  
  diamond_update_data(diamond, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
diamond_draw(Diamond *diamond, Renderer *renderer)
{
  Point pts[4];
  Element *elem;
  
  assert(diamond != NULL);
  assert(renderer != NULL);

  elem = &diamond->element;

  pts[0] = pts[1] = pts[2] = pts[3] = elem->corner;
  pts[0].x += elem->width / 2.0;
  pts[1].x += elem->width;
  pts[1].y += elem->height / 2.0;
  pts[2].x += elem->width / 2.0;
  pts[2].y += elem->height;
  pts[3].y += elem->height / 2.0;

  if (diamond->show_background) {
    renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  
    renderer->ops->fill_polygon(renderer, 
				pts, 4,
				&diamond->inner_color);
  }

  renderer->ops->set_linewidth(renderer, diamond->border_width);
  renderer->ops->set_linestyle(renderer, diamond->line_style);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_polygon(renderer, 
			      pts, 4,
			      &diamond->border_color);

  text_draw(diamond->text, renderer);
}

static DiamondState *
diamond_get_state(Diamond *diamond)
{
  DiamondState *state = g_new(DiamondState, 1);

  state->obj_state.free = NULL;
  
  state->border_width = diamond->border_width;
  state->border_color = diamond->border_color;
  state->inner_color = diamond->inner_color;
  state->show_background = diamond->show_background;
  state->line_style = diamond->line_style;
  state->padding = diamond->padding;
  text_get_attributes(diamond->text, &state->text_attrib);

  return state;
}

static void
diamond_set_state(Diamond *diamond, DiamondState *state)
{
  diamond->border_width = state->border_width;
  diamond->border_color = state->border_color;
  diamond->inner_color = state->inner_color;
  diamond->show_background = state->show_background;
  diamond->line_style = state->line_style;
  diamond->padding = state->padding;
  text_set_attributes(diamond->text, &state->text_attrib);

  g_free(state);
  
  diamond_update_data(diamond, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static void
diamond_update_data(Diamond *diamond, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &diamond->element;
  Object *obj = (Object *) diamond;
  Point center, bottom_right;
  Point p;
  real dw, dh;
  real width, height;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  width = diamond->text->max_width + 2*diamond->padding+diamond->border_width;
  height = diamond->text->height * diamond->text->numlines +
    2 * diamond->padding + diamond->border_width;

  if (height > (elem->width - width) * elem->height / elem->width) {
    /* increase size of the parallelogram while keeping its aspect ratio */
    real grad = elem->width/elem->height;
    if (grad < 1.0/4) grad = 1.0/4;
    if (grad > 4)     grad = 4;
    elem->width  = width  + height * grad;
    elem->height = height + width  / grad;
  }

  /* move shape if necessary ... */
  switch (horiz) {
  case ANCHOR_MIDDLE:
    elem->corner.x = center.x - elem->width/2; break;
  case ANCHOR_END:
    elem->corner.x = bottom_right.x - elem->width; break;
  }
  switch (vert) {
  case ANCHOR_MIDDLE:
    elem->corner.y = center.y - elem->height/2; break;
  case ANCHOR_END:
    elem->corner.y = bottom_right.y - elem->height; break;
  }

  p = elem->corner;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 - diamond->text->height*diamond->text->numlines/2 +
    font_ascent(diamond->text->font, diamond->text->height);
  text_set_position(diamond->text, &p);

  dw = elem->width / 8.0;
  dh = elem->height / 8.0;
  /* Update connections: */
  diamond->connections[0].pos.x = elem->corner.x + 4*dw;
  diamond->connections[0].pos.y = elem->corner.y;
  diamond->connections[1].pos.x = elem->corner.x + 5*dw;
  diamond->connections[1].pos.y = elem->corner.y + dh;
  diamond->connections[2].pos.x = elem->corner.x + 6*dw;
  diamond->connections[2].pos.y = elem->corner.y + 2*dh;
  diamond->connections[3].pos.x = elem->corner.x + 7*dw;
  diamond->connections[3].pos.y = elem->corner.y + 3*dh;
  diamond->connections[4].pos.x = elem->corner.x + elem->width;
  diamond->connections[4].pos.y = elem->corner.y + 4*dh;
  diamond->connections[5].pos.x = elem->corner.x + 7*dw;
  diamond->connections[5].pos.y = elem->corner.y + 5*dh;
  diamond->connections[6].pos.x = elem->corner.x + 6*dw;
  diamond->connections[6].pos.y = elem->corner.y + 6*dh;
  diamond->connections[7].pos.x = elem->corner.x + 5*dw;
  diamond->connections[7].pos.y = elem->corner.y + 7*dh;
  diamond->connections[8].pos.x = elem->corner.x + 4*dw;
  diamond->connections[8].pos.y = elem->corner.y + elem->height;
  diamond->connections[9].pos.x = elem->corner.x + 3*dw;
  diamond->connections[9].pos.y = elem->corner.y + 7*dh;
  diamond->connections[10].pos.x = elem->corner.x + 2*dw;
  diamond->connections[10].pos.y = elem->corner.y + 6*dh;
  diamond->connections[11].pos.x = elem->corner.x + dw;
  diamond->connections[11].pos.y = elem->corner.y + 5*dh;
  diamond->connections[12].pos.x = elem->corner.x;
  diamond->connections[12].pos.y = elem->corner.y + 4*dh;
  diamond->connections[13].pos.x = elem->corner.x + dw;
  diamond->connections[13].pos.y = elem->corner.y + 3*dh;
  diamond->connections[14].pos.x = elem->corner.x + 2*dw;
  diamond->connections[14].pos.y = elem->corner.y + 2*dh;
  diamond->connections[15].pos.x = elem->corner.x + 3*dw;
  diamond->connections[15].pos.y = elem->corner.y + dh;

  element_update_boundingbox(elem);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= diamond->border_width/2;
  obj->bounding_box.left -= diamond->border_width/2;
  obj->bounding_box.bottom += diamond->border_width/2;
  obj->bounding_box.right += diamond->border_width/2;
  
  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static Object *
diamond_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Diamond *diamond;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;

  init_default_values();

  diamond = g_malloc(sizeof(Diamond));
  elem = &diamond->element;
  obj = (Object *) diamond;
  
  obj->type = &diamond_type;

  obj->ops = &diamond_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  diamond->border_width =  attributes_get_default_linewidth();
  diamond->border_color = attributes_get_foreground();
  diamond->inner_color = attributes_get_background();
  diamond->show_background = default_properties.show_background;
  diamond->line_style = default_properties.line_style;

  diamond->padding = default_properties.padding;
  
  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + default_properties.font_size / 2;
  diamond->text = new_text("", default_properties.font,
		       default_properties.font_size, &p, &diamond->border_color,
		       ALIGN_CENTER);

  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &diamond->connections[i];
    diamond->connections[i].object = obj;
    diamond->connections[i].connected = NULL;
  }

  diamond_update_data(diamond, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return (Object *)diamond;
}

static void
diamond_destroy(Diamond *diamond)
{
  text_destroy(diamond->text);

  element_destroy(&diamond->element);
}

static Object *
diamond_copy(Diamond *diamond)
{
  int i;
  Diamond *newdiamond;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &diamond->element;
  
  newdiamond = g_malloc(sizeof(Diamond));
  newelem = &newdiamond->element;
  newobj = (Object *) newdiamond;

  element_copy(elem, newelem);

  newdiamond->border_width = diamond->border_width;
  newdiamond->border_color = diamond->border_color;
  newdiamond->inner_color = diamond->inner_color;
  newdiamond->show_background = diamond->show_background;
  newdiamond->line_style = diamond->line_style;
  newdiamond->padding = diamond->padding;

  newdiamond->text = text_copy(diamond->text);
  
  for (i=0;i<16;i++) {
    newobj->connections[i] = &newdiamond->connections[i];
    newdiamond->connections[i].object = newobj;
    newdiamond->connections[i].connected = NULL;
    newdiamond->connections[i].pos = diamond->connections[i].pos;
    newdiamond->connections[i].last_pos = diamond->connections[i].last_pos;
  }

  return (Object *)newdiamond;
}

static void
diamond_save(Diamond *diamond, ObjectNode obj_node, const char *filename)
{
  element_save(&diamond->element, obj_node);

  if (diamond->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  diamond->border_width);
  
  if (!color_equals(&diamond->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &diamond->border_color);
  
  if (!color_equals(&diamond->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &diamond->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"), diamond->show_background);

  if (diamond->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  diamond->line_style);

  data_add_text(new_attribute(obj_node, "text"), diamond->text);
}

static Object *
diamond_load(ObjectNode obj_node, int version, const char *filename)
{
  Diamond *diamond;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  diamond = g_malloc(sizeof(Diamond));
  elem = &diamond->element;
  obj = (Object *) diamond;
  
  obj->type = &diamond_type;
  obj->ops = &diamond_ops;

  element_load(elem, obj_node);
  
  diamond->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    diamond->border_width =  data_real( attribute_first_data(attr) );

  diamond->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &diamond->border_color);
  
  diamond->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &diamond->inner_color);
  
  diamond->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    diamond->show_background = data_boolean( attribute_first_data(attr) );

  diamond->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    diamond->line_style =  data_enum( attribute_first_data(attr) );

  diamond->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    diamond->text = data_text(attribute_first_data(attr));

  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &diamond->connections[i];
    diamond->connections[i].object = obj;
    diamond->connections[i].connected = NULL;
  }

  diamond_update_data(diamond, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return (Object *)diamond;
}
