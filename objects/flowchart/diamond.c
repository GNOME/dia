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
#include "properties.h"

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
typedef struct _DiamondDefaultsDialog DiamondDefaultsDialog;

struct _Diamond {
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

typedef struct _DiamondProperties {
  Color *fg_color;
  Color *bg_color;
  gboolean show_background;
  real border_width;

  real padding;
  Color *font_color;
} DiamondProperties;

struct _DiamondDefaultsDialog {
  GtkWidget *vbox;

  GtkToggleButton *show_background;

  GtkSpinButton *padding;
  DiaFontSelector *font;
  GtkSpinButton *font_size;
};


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

static PropDescription *diamond_describe_props(Diamond *diamond);
static void diamond_get_props(Diamond *diamond, Property *props, guint nprops);
static void diamond_set_props(Diamond *diamond, Property *props, guint nprops);

static void diamond_save(Diamond *diamond, ObjectNode obj_node, const char *filename);
static Object *diamond_load(ObjectNode obj_node, int version, const char *filename);
static GtkWidget *diamond_get_defaults(void);
static void diamond_apply_defaults(void);

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

static ObjectOps diamond_ops = {
  (DestroyFunc)         diamond_destroy,
  (DrawFunc)            diamond_draw,
  (DistanceFunc)        diamond_distance_from,
  (SelectFunc)          diamond_select,
  (CopyFunc)            diamond_copy,
  (MoveFunc)            diamond_move,
  (MoveHandleFunc)      diamond_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   diamond_describe_props,
  (GetPropsFunc)        diamond_get_props,
  (SetPropsFunc)        diamond_set_props,
};

static PropNumData text_padding_data = { 0.0, 10.0, 0.1 };

static PropDescription diamond_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_STD_LINE_STYLE,
  { "padding", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Text padding"), NULL, &text_padding_data },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_TEXT,
  
  { NULL, 0, 0, NULL, NULL, NULL, 0}
};

static PropDescription *
diamond_describe_props(Diamond *diamond)
{
  if (diamond_props[0].quark == 0)
    prop_desc_list_calculate_quarks(diamond_props);
  return diamond_props;
}

static PropOffset diamond_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Diamond, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Diamond, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Diamond, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Diamond, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Diamond, line_style), offsetof(Diamond, dashlength) },
  { "padding", PROP_TYPE_REAL, offsetof(Diamond, padding) },
  { NULL, 0, 0 },
};

static struct { const gchar *name; GQuark q; } quarks[] = {
  { "text_font" },
  { "text_height" },
  { "text_colour" },
  { "text" }
};

static void
diamond_get_props(Diamond *diamond, Property *props, guint nprops)
{
  guint i;

  if (object_get_props_from_offsets((Object *)diamond, diamond_offsets,
				    props, nprops))
    return;
  /* these props can't be handled as easily */
  if (quarks[0].q == 0)
    for (i = 0; i < 4; i++)
      quarks[i].q = g_quark_from_static_string(quarks[i].name);
  for (i = 0; i < nprops; i++) {
    GQuark pquark = g_quark_from_string(props[i].name);

    if (pquark == quarks[0].q) {
      props[i].type = PROP_TYPE_FONT;
      PROP_VALUE_FONT(props[i]) = diamond->text->font;
    } else if (pquark == quarks[1].q) {
      props[i].type = PROP_TYPE_REAL;
      PROP_VALUE_REAL(props[i]) = diamond->text->height;
    } else if (pquark == quarks[2].q) {
      props[i].type = PROP_TYPE_COLOUR;
      PROP_VALUE_COLOUR(props[i]) = diamond->text->color;
    } else if (pquark == quarks[3].q) {
      props[i].type = PROP_TYPE_STRING;
      g_free(PROP_VALUE_STRING(props[i]));
      PROP_VALUE_STRING(props[i]) = text_get_string_copy(diamond->text);
    }
  }
}

static void
diamond_set_props(Diamond *diamond, Property *props, guint nprops)
{
  if (!object_set_props_from_offsets((Object *)diamond, diamond_offsets,
                                     props, nprops)) {
    guint i;

    if (quarks[0].q == 0)
      for (i = 0; i < 4; i++)
        quarks[i].q = g_quark_from_static_string(quarks[i].name);

    for (i = 0; i < nprops; i++) {
      GQuark pquark = g_quark_from_string(props[i].name);

      if (pquark == quarks[0].q && props[i].type == PROP_TYPE_FONT) {
        text_set_font(diamond->text, PROP_VALUE_FONT(props[i]));
      } else if (pquark == quarks[1].q && props[i].type == PROP_TYPE_REAL) {
        text_set_height(diamond->text, PROP_VALUE_REAL(props[i]));
      } else if (pquark == quarks[2].q && props[i].type == PROP_TYPE_COLOUR) {
        text_set_color(diamond->text, &PROP_VALUE_COLOUR(props[i]));
      } else if (pquark == quarks[3].q && props[i].type == PROP_TYPE_STRING) {
        text_set_string(diamond->text, PROP_VALUE_STRING(props[i]));
      }
    }
  }
  diamond_update_data(diamond, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
diamond_apply_defaults()
{
  default_properties.show_background = gtk_toggle_button_get_active(diamond_defaults_dialog->show_background);

  default_properties.padding = gtk_spin_button_get_value_as_float(diamond_defaults_dialog->padding);
  attributes_set_default_font(
      dia_font_selector_get_font(diamond_defaults_dialog->font),
      gtk_spin_button_get_value_as_float(diamond_defaults_dialog->font_size));
}

static void
init_default_values() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    default_properties.padding = 0.5 * M_SQRT1_2;
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
  GtkWidget *padding;
  GtkWidget *fontsel;
  GtkWidget *font_size;
  GtkAdjustment *adj;
  Font *font;
  real font_height;

  if (diamond_defaults_dialog == NULL) {
  
    init_default_values();

    diamond_defaults_dialog = g_new(DiamondDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    diamond_defaults_dialog->vbox = vbox;

    gtk_object_ref(GTK_OBJECT(vbox));
    gtk_object_sink(GTK_OBJECT(vbox));

    hbox = gtk_hbox_new(FALSE, 5);
    checkdiamond = gtk_check_button_new_with_label(_("Draw background"));
    diamond_defaults_dialog->show_background = GTK_TOGGLE_BUTTON( checkdiamond );
    gtk_widget_show(checkdiamond);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkdiamond, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

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
    fontsel = dia_font_selector_new();
    diamond_defaults_dialog->font = DIAFONTSELECTOR(fontsel);
    gtk_box_pack_start (GTK_BOX (hbox), fontsel, TRUE, TRUE, 0);
    gtk_widget_show (fontsel);
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

  gtk_spin_button_set_value(diamond_defaults_dialog->padding,
			    default_properties.padding);
  attributes_get_default_font(&font, &font_height);
  dia_font_selector_set_font(diamond_defaults_dialog->font, font);
  gtk_spin_button_set_value(diamond_defaults_dialog->font_size, font_height);

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
    return point->y - rect.bottom +
      fabs(point->x - elem->corner.x + elem->width / 2.0);
  else if (rect.left > point->x)
    return rect.left - point->x +
      fabs(point->y - elem->corner.y + elem->height / 2.0);
  else if (point->x > rect.right)
    return point->x - rect.right +
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
  renderer->ops->set_dashlength(renderer, diamond->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_polygon(renderer, 
			      pts, 4,
			      &diamond->border_color);

  text_draw(diamond->text, renderer);
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
  int i;
  Font *font;
  real font_height;

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
  attributes_get_default_line_style(&diamond->line_style,&diamond->dashlength);

  diamond->padding = default_properties.padding;

  attributes_get_default_font(&font, &font_height);
  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + font_height / 2;
  diamond->text = new_text("", font, font_height, &p, &diamond->border_color,
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
  newdiamond->dashlength = diamond->dashlength;
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

  if (diamond->line_style != LINESTYLE_SOLID &&
      diamond->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  diamond->dashlength);

  data_add_real(new_attribute(obj_node, "padding"), diamond->padding);
  
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

  diamond->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    diamond->dashlength = data_real(attribute_first_data(attr));

  diamond->padding = default_properties.padding;
  attr = object_find_attribute(obj_node, "padding");
  if (attr != NULL)
    diamond->padding =  data_real( attribute_first_data(attr) );
  
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
