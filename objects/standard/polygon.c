/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
#include "polyshape.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "diamenu.h"
#include "message.h"
#include "properties.h"

#include "pixmaps/polygon.xpm"

#define DEFAULT_WIDTH 0.15

typedef struct _PolygonProperties PolygonProperties;
typedef struct _PolygonDefaultsDialog PolygonDefaultsDialog;

typedef struct _Polygon {
  PolyShape poly;

  Color line_color;
  LineStyle line_style;
  Color inner_color;
  gboolean show_background;
  real dashlength;
  real line_width;
} Polygon;

struct _PolygonProperties {
  gboolean show_background;
};

struct _PolygonDefaultsDialog {
  GtkWidget *vbox;

  GtkToggleButton *show_background;
};


static PolygonDefaultsDialog *polygon_defaults_dialog;
static PolygonProperties default_properties;


static void polygon_move_handle(Polygon *polygon, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void polygon_move(Polygon *polygon, Point *to);
static void polygon_select(Polygon *polygon, Point *clicked_point,
			      Renderer *interactive_renderer);
static void polygon_draw(Polygon *polygon, Renderer *renderer);
static Object *polygon_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real polygon_distance_from(Polygon *polygon, Point *point);
static void polygon_update_data(Polygon *polygon);
static void polygon_destroy(Polygon *polygon);
static Object *polygon_copy(Polygon *polygon);

static PropDescription *polygon_describe_props(Polygon *polygon);
static void polygon_get_props(Polygon *polygon, Property *props,
			       guint nprops);
static void polygon_set_props(Polygon *polygon, Property *props,
			       guint nprops);

static void polygon_save(Polygon *polygon, ObjectNode obj_node,
			  const char *filename);
static Object *polygon_load(ObjectNode obj_node, int version,
			     const char *filename);
static DiaMenu *polygon_get_object_menu(Polygon *polygon, Point *clickedpoint);

static GtkWidget *polygon_get_defaults();
static void polygon_apply_defaults();

static ObjectTypeOps polygon_type_ops =
{
  (CreateFunc)polygon_create,   /* create */
  (LoadFunc)  polygon_load,     /* load */
  (SaveFunc)  polygon_save,      /* save */
  (GetDefaultsFunc)   polygon_get_defaults,
  (ApplyDefaultsFunc) polygon_apply_defaults
};

static ObjectType polygon_type =
{
  "Standard - Polygon",   /* name */
  0,                         /* version */
  (char **) polygon_xpm,      /* pixmap */
  
  &polygon_type_ops       /* ops */
};

ObjectType *_polygon_type = (ObjectType *) &polygon_type;


static ObjectOps polygon_ops = {
  (DestroyFunc)         polygon_destroy,
  (DrawFunc)            polygon_draw,
  (DistanceFunc)        polygon_distance_from,
  (SelectFunc)          polygon_select,
  (CopyFunc)            polygon_copy,
  (MoveFunc)            polygon_move,
  (MoveHandleFunc)      polygon_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      polygon_get_object_menu,
  (DescribePropsFunc)   polygon_describe_props,
  (GetPropsFunc)        polygon_get_props,
  (SetPropsFunc)        polygon_set_props,
};

static PropDescription polygon_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_DESC_END
};

static PropDescription *
polygon_describe_props(Polygon *polygon)
{
  if (polygon_props[0].quark == 0)
    prop_desc_list_calculate_quarks(polygon_props);
  return polygon_props;
}

static PropOffset polygon_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Polygon, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Polygon, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Polygon, line_style), offsetof(Polygon, dashlength) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Polygon, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Polygon, show_background) },
  { NULL, 0, 0 }
};

static void
polygon_get_props(Polygon *polygon, Property *props, guint nprops)
{
  object_get_props_from_offsets((Object *)polygon, polygon_offsets,
				props, nprops);
}

static void
polygon_set_props(Polygon *polygon, Property *props, guint nprops)
{
  object_set_props_from_offsets((Object *)polygon, polygon_offsets,
				props, nprops);
  polygon_update_data(polygon);
}

static void
polygon_apply_defaults()
{
  default_properties.show_background = gtk_toggle_button_get_active(polygon_defaults_dialog->show_background);
}

static void
init_default_values() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    defaults_initialized = 1;
  }
}

static GtkWidget *
polygon_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *checkbox;
  GtkWidget *corner_radius;
  GtkAdjustment *adj;

  if (polygon_defaults_dialog == NULL) {
  
    init_default_values();

    polygon_defaults_dialog = g_new(PolygonDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    polygon_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Draw background"));
    polygon_defaults_dialog->show_background = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  gtk_toggle_button_set_active(polygon_defaults_dialog->show_background, 
			       default_properties.show_background);

  return polygon_defaults_dialog->vbox;
}


static real
polygon_distance_from(Polygon *polygon, Point *point)
{
  return polyshape_distance_from(&polygon->poly, point, polygon->line_width);
}

static Handle *polygon_closest_handle(Polygon *polygon, Point *point) {
  return polyshape_closest_handle(&polygon->poly, point);
}

static int polygon_closest_segment(Polygon *polygon, Point *point) {
  return polyshape_closest_segment(&polygon->poly, point, polygon->line_width);
}

static void
polygon_select(Polygon *polygon, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  polyshape_update_data(&polygon->poly);
}

static void
polygon_move_handle(Polygon *polygon, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(polygon!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  polyshape_move_handle(&polygon->poly, handle, to, reason);
  polygon_update_data(polygon);
}


static void
polygon_move(Polygon *polygon, Point *to)
{
  polyshape_move(&polygon->poly, to);
  polygon_update_data(polygon);
}

static void
polygon_draw(Polygon *polygon, Renderer *renderer)
{
  PolyShape *poly = &polygon->poly;
  Point *points;
  int n;
  
  points = &poly->points[0];
  n = poly->numpoints;

  renderer->ops->set_linewidth(renderer, polygon->line_width);
  renderer->ops->set_linestyle(renderer, polygon->line_style);
  renderer->ops->set_dashlength(renderer, polygon->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  if (polygon->show_background)
    renderer->ops->fill_polygon(renderer, points, n, &polygon->inner_color);

  renderer->ops->draw_polygon(renderer, points, n, &polygon->line_color);
}

static Object *
polygon_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Polygon *polygon;
  PolyShape *poly;
  Object *obj;
  Point defaultx = { 1.0, 0.0 };
  Point defaulty = { 1.0, 0.0 };

  /*polygon_init_defaults();*/
  polygon = g_malloc(sizeof(Polygon));
  poly = &polygon->poly;
  obj = (Object *) polygon;
  
  obj->type = &polygon_type;
  obj->ops = &polygon_ops;

  polyshape_init(poly);

  poly->points[0] = *startpoint;
  poly->points[1] = *startpoint;
  point_add(&poly->points[1], &defaultx);
  poly->points[2] = *startpoint;
  point_add(&poly->points[2], &defaulty);

  polygon_update_data(polygon);

  polygon->line_width =  attributes_get_default_linewidth();
  polygon->line_color = attributes_get_foreground();
  attributes_get_default_line_style(&polygon->line_style,
				    &polygon->dashlength);
  polygon->show_background = default_properties.show_background;

  *handle1 = poly->object.handles[0];
  *handle2 = poly->object.handles[2];
  return (Object *)polygon;
}

static void
polygon_destroy(Polygon *polygon)
{
  polyshape_destroy(&polygon->poly);
}

static Object *
polygon_copy(Polygon *polygon)
{
  Polygon *newpolygon;
  PolyShape *poly, *newpoly;
  Object *newobj;
  
  poly = &polygon->poly;
 
  newpolygon = g_malloc(sizeof(Polygon));
  newpoly = &newpolygon->poly;
  newobj = (Object *) newpolygon;

  polyshape_copy(poly, newpoly);

  newpolygon->line_color = polygon->line_color;
  newpolygon->line_width = polygon->line_width;
  newpolygon->line_style = polygon->line_style;
  newpolygon->dashlength = polygon->dashlength;
  newpolygon->show_background = polygon->show_background;

  return (Object *)newpolygon;
}


static void
polygon_update_data(Polygon *polygon)
{
  PolyShape *poly = &polygon->poly;
  Object *obj = (Object *) polygon;

  polyshape_update_data(poly);
  
  polyshape_update_boundingbox(poly);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= polygon->line_width/2;
  obj->bounding_box.left -= polygon->line_width/2;
  obj->bounding_box.bottom += polygon->line_width/2;
  obj->bounding_box.right += polygon->line_width/2;

  obj->position = poly->points[0];
}

static void
polygon_save(Polygon *polygon, ObjectNode obj_node,
	      const char *filename)
{
  polyshape_save(&polygon->poly, obj_node);

  if (!color_equals(&polygon->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &polygon->line_color);
  
  if (polygon->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  polygon->line_width);
  
  if (!color_equals(&polygon->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &polygon->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"),
		   polygon->show_background);

  if (polygon->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  polygon->line_style);

  if (polygon->line_style != LINESTYLE_SOLID &&
      polygon->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  polygon->dashlength);
  
}

static Object *
polygon_load(ObjectNode obj_node, int version, const char *filename)
{
  Polygon *polygon;
  PolyShape *poly;
  Object *obj;
  AttributeNode attr;

  polygon = g_malloc(sizeof(Polygon));

  poly = &polygon->poly;
  obj = (Object *) polygon;
  
  obj->type = &polygon_type;
  obj->ops = &polygon_ops;

  polyshape_load(poly, obj_node);

  polygon->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &polygon->line_color);

  polygon->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    polygon->line_width = data_real(attribute_first_data(attr));

  polygon->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &polygon->inner_color);
  
  polygon->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    polygon->show_background = data_boolean( attribute_first_data(attr) );

  polygon->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    polygon->line_style = data_enum(attribute_first_data(attr));

  polygon->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    polygon->dashlength = data_real(attribute_first_data(attr));

  polygon_update_data(polygon);

  return (Object *)polygon;
}

static ObjectChange *
polygon_add_corner_callback (Object *obj, Point *clicked, gpointer data)
{
  Polygon *poly = (Polygon*) obj;
  int segment;
  ObjectChange *change;

  segment = polygon_closest_segment(poly, clicked);
  change = polyshape_add_point(&poly->poly, segment, clicked);
  polygon_update_data(poly);
  return change;
}

static ObjectChange *
polygon_delete_corner_callback (Object *obj, Point *clicked, gpointer data)
{
  Handle *handle;
  int handle_nr, i;
  Polygon *poly = (Polygon*) obj;
  ObjectChange *change;
  
  handle = polygon_closest_handle(poly, clicked);

  for (i = 0; i < obj->num_handles; i++) {
    if (handle == obj->handles[i]) break;
  }
  handle_nr = i;
  change = polyshape_remove_point(&poly->poly, handle_nr);
  polygon_update_data(poly);
  return change;
}


static DiaMenuItem polygon_menu_items[] = {
  { N_("Add Corner"), polygon_add_corner_callback, NULL, 1 },
  { N_("Delete Corner"), polygon_delete_corner_callback, NULL, 1 },
};

static DiaMenu polygon_menu = {
  "Polygon",
  sizeof(polygon_menu_items)/sizeof(DiaMenuItem),
  polygon_menu_items,
  NULL
};

static DiaMenu *
polygon_get_object_menu(Polygon *polygon, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  polygon_menu_items[0].active = 1;
  polygon_menu_items[1].active = polygon->poly.numpoints > 2;
  return &polygon_menu;
}
