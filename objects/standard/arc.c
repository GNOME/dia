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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "connection.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "arrows.h"

#include "pixmaps/arc.xpm"

#define DEFAULT_WIDTH 0.25

#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef struct _ArcPropertiesDialog ArcPropertiesDialog;
typedef struct _ArcDefaultsDialog ArcDefaultsDialog;
typedef struct _ArcState ArcState;

struct _ArcState {
  ObjectState obj_state;
  
  Color arc_color;
  real curve_distance;
  real line_width;
  LineStyle line_style;
  real dashlength;
  Arrow start_arrow, end_arrow;
};

typedef struct _Arc {
  Connection connection;

  Handle middle_handle;

  Color arc_color;
  real curve_distance;
  real line_width;
  LineStyle line_style;
  real dashlength;
  Arrow start_arrow, end_arrow;

  /* Calculated parameters: */
  real radius;
  Point center;
  real angle1, angle2;

} Arc;

typedef struct _ArcProperties {
  Color line_color;
  real line_width;
  LineStyle line_style;
  real dashlength;
  Arrow start_arrow, end_arrow;
} ArcProperties;


struct _ArcPropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *line_width;
  DiaColorSelector *color;
  DiaLineStyleSelector *line_style;
  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;
};

/*struct _ArcDefaultsDialog {
  GtkWidget *vbox;

  DiaLineStyleSelector *line_style;
  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;
  };*/

static ArcPropertiesDialog *arc_properties_dialog;
/* static ArcDefaultsDialog *arc_defaults_dialog;
   static ArcProperties default_properties; */


static void arc_move_handle(Arc *arc, Handle *handle,
			    Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void arc_move(Arc *arc, Point *to);
static void arc_select(Arc *arc, Point *clicked_point,
		       Renderer *interactive_renderer);
static void arc_draw(Arc *arc, Renderer *renderer);
static Object *arc_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static real arc_distance_from(Arc *arc, Point *point);
static void arc_update_data(Arc *arc);
static void arc_update_handles(Arc *arc);
static void arc_destroy(Arc *arc);
static Object *arc_copy(Arc *arc);
static GtkWidget *arc_get_properties(Arc *arc);
static ObjectChange *arc_apply_properties(Arc *arc);

static ArcState *arc_get_state(Arc *arc);
static void arc_set_state(Arc *arc, ArcState *state);

static void arc_save(Arc *arc, ObjectNode obj_node, const char *filename);
static Object *arc_load(ObjectNode obj_node, int version, const char *filename);
/* static GtkWidget *arc_get_defaults();
   static void arc_apply_defaults(); */

static ObjectTypeOps arc_type_ops =
{
  (CreateFunc) arc_create,
  (LoadFunc)   arc_load,
  (SaveFunc)   arc_save,
  (GetDefaultsFunc)   NULL /*arc_get_defaults*/,
  (ApplyDefaultsFunc) NULL /*arc_apply_defaults*/
};

ObjectType arc_type =
{
  "Standard - Arc",  /* name */
  0,                 /* version */
  (char **) arc_xpm, /* pixmap */
  
  &arc_type_ops      /* ops */
};

ObjectType *_arc_type = (ObjectType *) &arc_type;

static ObjectOps arc_ops = {
  (DestroyFunc)         arc_destroy,
  (DrawFunc)            arc_draw,
  (DistanceFunc)        arc_distance_from,
  (SelectFunc)          arc_select,
  (CopyFunc)            arc_copy,
  (MoveFunc)            arc_move,
  (MoveHandleFunc)      arc_move_handle,
  (GetPropertiesFunc)   arc_get_properties,
  (ApplyPropertiesFunc) arc_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
arc_apply_properties(Arc *arc)
{
  ObjectState *old_state;

  old_state = (ObjectState *)arc_get_state(arc);
  
  arc->line_width = gtk_spin_button_get_value_as_float(arc_properties_dialog->line_width);
  dia_color_selector_get_color(arc_properties_dialog->color, &arc->arc_color);
  dia_line_style_selector_get_linestyle(arc_properties_dialog->line_style,
					&arc->line_style, &arc->dashlength);
  arc->start_arrow = dia_arrow_selector_get_arrow(arc_properties_dialog->start_arrow);
  arc->end_arrow = dia_arrow_selector_get_arrow(arc_properties_dialog->end_arrow);
  
  
  arc_update_data(arc);
  return new_object_state_change((Object *)arc, old_state, 
				 (GetStateFunc)arc_get_state,
				 (SetStateFunc)arc_set_state);
}

static GtkWidget *
arc_get_properties(Arc *arc)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *linestyle;
  GtkWidget *arrow;
  GtkWidget *line_width;
  GtkWidget *align;
  GtkAdjustment *adj;

  if (arc_properties_dialog == NULL) {
  
    arc_properties_dialog = g_new(ArcPropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    arc_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    line_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(line_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(line_width), TRUE);
    arc_properties_dialog->line_width = GTK_SPIN_BUTTON(line_width);
    gtk_box_pack_start(GTK_BOX (hbox), line_width, TRUE, TRUE, 0);
    gtk_widget_show (line_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    arc_properties_dialog->color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    arc_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Start arrow:"));
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    arc_properties_dialog->start_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("End arrow:"));
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    arc_properties_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    gtk_widget_show (vbox);
  }

  gtk_spin_button_set_value(arc_properties_dialog->line_width, arc->line_width);
  dia_color_selector_set_color(arc_properties_dialog->color, &arc->arc_color);
  dia_line_style_selector_set_linestyle(arc_properties_dialog->line_style,
					arc->line_style, arc->dashlength);
  dia_arrow_selector_set_arrow(arc_properties_dialog->start_arrow,
			       arc->start_arrow);
  dia_arrow_selector_set_arrow(arc_properties_dialog->end_arrow,
			       arc->end_arrow);
  
  return arc_properties_dialog->vbox;
}

/*
static void
arc_init_defaults(void)
{
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.start_arrow.length = 0.5;
    default_properties.start_arrow.width = 0.5;
    default_properties.end_arrow.length = 0.5;
    default_properties.end_arrow.width = 0.5;
    defaults_initialized = 1;
  }
}

static void
arc_apply_defaults(void)
{
  arc_init_defaults();
  dia_line_style_selector_get_linestyle(arc_defaults_dialog->line_style,
					&default_properties.line_style, NULL);
  default_properties.start_arrow = dia_arrow_selector_get_arrow(arc_defaults_dialog->start_arrow);
  default_properties.end_arrow = dia_arrow_selector_get_arrow(arc_defaults_dialog->end_arrow);
}

static GtkWidget *
arc_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *arrow;
  GtkWidget *align;
  GtkWidget *linestyle;

  if (arc_defaults_dialog == NULL) {
  
    arc_init_defaults();

    arc_defaults_dialog = g_new(ArcDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    arc_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    arc_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Start arrow:"));
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    arc_defaults_dialog->start_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("End arrow:"));
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    arc_defaults_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  dia_line_style_selector_set_linestyle(arc_defaults_dialog->line_style,
					default_properties.line_style, 1.0);
  dia_arrow_selector_set_arrow(arc_defaults_dialog->start_arrow,
					 default_properties.start_arrow);
  dia_arrow_selector_set_arrow(arc_defaults_dialog->end_arrow,
					 default_properties.end_arrow);

  return arc_defaults_dialog->vbox;
}
*/

static int
in_angle(real angle, real startangle, real endangle)
{
  if (startangle > endangle) {  /* passes 360 degrees */
    endangle += 360.0;
    if (angle<startangle)
      angle += 360;
  }
  return (angle>=startangle) && (angle<=endangle);
}

static real
arc_distance_from(Arc *arc, Point *point)
{
  Point *endpoints;
  Point from_center;
  real angle;
  real d, d2;
  
  endpoints = &arc->connection.endpoints[0];

  from_center = *point;
  point_sub(&from_center, &arc->center);

  angle = -atan2(from_center.y, from_center.x)*180.0/M_PI;
  if (angle<0)
    angle+=360.0;

  if (in_angle(angle, arc->angle1, arc->angle2)) {
    d = fabs(sqrt(point_dot(&from_center, &from_center)) - arc->radius);
    d -= arc->line_width/2.0;
    if (d<0)
      d = 0.0;
    return d;
  } else {
    d = distance_point_point(&endpoints[0], point);
    d2 = distance_point_point(&endpoints[1], point);

    return MIN(d,d2);
  }
}

static void
arc_select(Arc *arc, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  arc_update_handles(arc);
}

static void
arc_update_handles(Arc *arc)
{
  Point *middle_pos;
  real dist,dx,dy;

  Connection *conn = &arc->connection;

  connection_update_handles(conn);
  
  middle_pos = &arc->middle_handle.pos;

  dx = conn->endpoints[1].x - conn->endpoints[0].x;
  dy = conn->endpoints[1].y - conn->endpoints[0].y;
  
  dist = sqrt(dx*dx + dy*dy);
  middle_pos->x =
    (conn->endpoints[0].x + conn->endpoints[1].x) / 2.0 -
    arc->curve_distance*dy/dist;
  middle_pos->y =
    (conn->endpoints[0].y + conn->endpoints[1].y) / 2.0 +
    arc->curve_distance*dx/dist;
}


static void
arc_move_handle(Arc *arc, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(arc!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MIDDLE) {
    Point a,b;
    real tmp;

    b = *to;
    point_sub(&b, &arc->connection.endpoints[0]);
   
    a = arc->connection.endpoints[1];
    point_sub(&a, &arc->connection.endpoints[0]);

    tmp = point_dot(&a,&b);
    arc->curve_distance =
      sqrt(fabs(point_dot(&b,&b) - tmp*tmp/point_dot(&a,&a)));
    
    if (a.x*b.y - a.y*b.x < 0) 
      arc->curve_distance = -arc->curve_distance;

  } else {
    connection_move_handle(&arc->connection, handle->id, to, reason);
  }

  arc_update_data(arc);
}

static void
arc_move(Arc *arc, Point *to)
{
  Point start_to_end;
  Point *endpoints = &arc->connection.endpoints[0]; 

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  arc_update_data(arc);
}

static void
arc_draw(Arc *arc, Renderer *renderer)
{
  Point *endpoints;
  real width;
    
  assert(arc != NULL);
  assert(renderer != NULL);

  endpoints = &arc->connection.endpoints[0];

  renderer->ops->set_linewidth(renderer, arc->line_width);
  renderer->ops->set_linestyle(renderer, arc->line_style);
  renderer->ops->set_dashlength(renderer, arc->dashlength);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);
  
  /* Special case when almost line: */
  if (fabs(arc->curve_distance) <= 0.0001) {
    renderer->ops->draw_line(renderer,
			     &endpoints[0], &endpoints[1],
			     &arc->arc_color);
    return;
  }
  
  width = 2*arc->radius;
  
  renderer->ops->draw_arc(renderer,
			  &arc->center,
			  width, width,
			  arc->angle1, arc->angle2,
			  &arc->arc_color);

  if (arc->start_arrow.type != ARROW_NONE ||
      arc->end_arrow.type != ARROW_NONE) {
    Point reversepoint, centervec;
    Point controlpoint0, controlpoint1;
    real len, len2;

    centervec = endpoints[0];
    point_sub(&centervec, &endpoints[1]);
    point_scale(&centervec, 0.5);
    len = centervec.x*centervec.x+centervec.y*centervec.y;
    point_add(&centervec, &endpoints[1]);
    /* Centervec should now be the midpoint between [0] and [1] */
    reversepoint = centervec;
    point_sub(&centervec, &arc->center);
    len2 = centervec.x*centervec.x+centervec.y*centervec.y;
    if (len2 != 0) {
      point_scale(&centervec, 1/len2);
    }
    point_scale(&centervec, len);
    point_add(&reversepoint, &centervec);
    controlpoint0 = controlpoint1 = reversepoint;
    len = arc->angle2-arc->angle1;
    if (len > 180 || (len < 0.0 && len > -180)) {
      centervec = endpoints[0];
      point_sub(&centervec, &reversepoint);
      point_add(&controlpoint0, &centervec);
      point_add(&controlpoint0, &centervec);
      centervec = endpoints[1];
      point_sub(&centervec, &reversepoint);
      point_add(&controlpoint1, &centervec);
      point_add(&controlpoint1, &centervec);
    }
    if (arc->start_arrow.type != ARROW_NONE) {
      arrow_draw(renderer, arc->start_arrow.type,
		 &endpoints[0],&controlpoint0,
		 arc->start_arrow.length, arc->start_arrow.width,
		 arc->line_width,
		 &arc->arc_color, &color_white);
    }
    if (arc->end_arrow.type != ARROW_NONE) {
      arrow_draw(renderer, arc->end_arrow.type,
		 &endpoints[1], &controlpoint1,
		 arc->end_arrow.length, arc->end_arrow.width,
		 arc->line_width,
		 &arc->arc_color, &color_white);
    }
  }
}

static Object *
arc_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Arc *arc;
  Connection *conn;
  Object *obj;
  Point defaultlen = { 1.0, 1.0 };

  /*arc_init_defaults();*/

  arc = g_malloc(sizeof(Arc));

  arc->line_width =  attributes_get_default_linewidth();
  arc->curve_distance = 1.0;
  arc->arc_color = attributes_get_foreground(); 
  attributes_get_default_line_style(&arc->line_style, &arc->dashlength);
  arc->start_arrow = attributes_get_default_start_arrow();
  arc->end_arrow = attributes_get_default_end_arrow();

  conn = &arc->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = (Object *) arc;
  
  obj->type = &arc_type;;
  obj->ops = &arc_ops;
  
  connection_init(conn, 3, 0);

  obj->handles[2] = &arc->middle_handle;
  arc->middle_handle.id = HANDLE_MIDDLE;
  arc->middle_handle.type = HANDLE_MINOR_CONTROL;
  arc->middle_handle.connect_type = HANDLE_NONCONNECTABLE;
  arc->middle_handle.connected_to = NULL;

  arc_update_data(arc);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (Object *)arc;
}

static void
arc_destroy(Arc *arc)
{
  connection_destroy(&arc->connection);
}

static Object *
arc_copy(Arc *arc)
{
  Arc *newarc;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &arc->connection;
  
  newarc = g_malloc(sizeof(Arc));
  newconn = &newarc->connection;
  newobj = (Object *) newarc;

  connection_copy(conn, newconn);

  newarc->arc_color = arc->arc_color;
  newarc->curve_distance = arc->curve_distance;
  newarc->line_width = arc->line_width;
  newarc->line_style = arc->line_style;
  newarc->dashlength = arc->dashlength;
  newarc->start_arrow = arc->start_arrow;
  newarc->end_arrow = arc->end_arrow;
  newarc->radius = arc->radius;
  newarc->center = arc->center;
  newarc->angle1 = arc->angle1;
  newarc->angle2 = arc->angle2;

  newobj->handles[2] = &newarc->middle_handle;
  
  newarc->middle_handle = arc->middle_handle;

  return (Object *)newarc;
}

static ArcState *
arc_get_state(Arc *arc)
{
  ArcState *state = g_new(ArcState, 1);

  state->obj_state.free = NULL;
  
  state->line_width = arc->line_width;
  state->arc_color = arc->arc_color;
  state->curve_distance = arc->curve_distance;
  state->line_style = arc->line_style;
  state->dashlength = arc->dashlength;
  state->start_arrow = arc->start_arrow;
  state->end_arrow = arc->end_arrow;

  return state;
}

static void
arc_set_state(Arc *arc, ArcState *state)
{
  arc->line_width = state->line_width;
  arc->arc_color = state->arc_color;
  arc->curve_distance = state->curve_distance;
  arc->line_style = state->line_style;
  arc->dashlength = state->dashlength;
  arc->start_arrow = state->start_arrow;
  arc->end_arrow = state->end_arrow;

  g_free(state);
  
  arc_update_data(arc);
}

static void
arc_update_data(Arc *arc)
{
  Connection *conn = &arc->connection;
  Object *obj = (Object *) arc;
  Point *endpoints;
  real x1,y1,x2,y2,xc,yc;
  real lensq, alpha, radius;
  real angle1, angle2;
  
  endpoints = &arc->connection.endpoints[0];
  x1 = endpoints[0].x;
  y1 = endpoints[0].y;
  x2 = endpoints[1].x;
  y2 = endpoints[1].y;
  
  lensq = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
  radius = lensq/(8*arc->curve_distance) + arc->curve_distance/2.0;

  alpha = (radius - arc->curve_distance) / sqrt(lensq);
  
  xc = (x1 + x2) / 2.0 + (y2 - y1)*alpha;
  yc = (y1 + y2) / 2.0 + (x1 - x2)*alpha;

  angle1 = -atan2(y1-yc, x1-xc)*180.0/M_PI;
  if (angle1<0)
    angle1+=360.0;
  angle2 = -atan2(y2-yc, x2-xc)*180.0/M_PI;
  if (angle2<0)
    angle2+=360.0;

  if (radius<0.0) {
    real tmp;
    tmp = angle1;
    angle1 = angle2;
    angle2 = tmp;
    radius = -radius;
  }
  
  arc->radius = radius;
  arc->center.x = xc; arc->center.y = yc;
  arc->angle1 = angle1;
  arc->angle2 = angle2;
  
  connection_update_boundingbox(conn);
  /* fix boundingbox for line_width: */
  if (in_angle(0, arc->angle1, arc->angle2)) {
    obj->bounding_box.right = arc->center.x + arc->radius;
  }
  if (in_angle(90, arc->angle1, arc->angle2)) {
    obj->bounding_box.top = arc->center.y - arc->radius;
  }
  if (in_angle(180, arc->angle1, arc->angle2)) {
    obj->bounding_box.left = arc->center.x - arc->radius;
  }
  if (in_angle(270, arc->angle1, arc->angle2)) {
    obj->bounding_box.bottom = arc->center.y + arc->radius;
  }
  
  /* Fix boundingbox for arrowheads */
  if (arc->start_arrow.type != ARROW_NONE ||
      arc->end_arrow.type != ARROW_NONE) {
    real arrow_width = 0.0;
    if (arc->start_arrow.type != ARROW_NONE)
      arrow_width = arc->start_arrow.width;
    if (arc->end_arrow.type != ARROW_NONE)
      arrow_width = MAX(arrow_width, arc->start_arrow.width);

    obj->bounding_box.top -= arrow_width;
    obj->bounding_box.left -= arrow_width;
    obj->bounding_box.bottom += arrow_width;
    obj->bounding_box.right += arrow_width;
  }
  obj->bounding_box.top -= arc->line_width/2;
  obj->bounding_box.left -= arc->line_width/2;
  obj->bounding_box.bottom += arc->line_width/2;
  obj->bounding_box.right += arc->line_width/2;

  obj->position = conn->endpoints[0];
  
  arc_update_handles(arc);
}

static void
arc_save(Arc *arc, ObjectNode obj_node, const char *filename)
{
  connection_save(&arc->connection, obj_node);

  if (!color_equals(&arc->arc_color, &color_black))
    data_add_color(new_attribute(obj_node, "arc_color"),
		   &arc->arc_color);
  
  if (arc->curve_distance != 0.1)
    data_add_real(new_attribute(obj_node, "curve_distance"),
		  arc->curve_distance);
  
  if (arc->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  arc->line_width);
  
  if (arc->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  arc->line_style);

  if (arc->line_style != LINESTYLE_SOLID &&
      arc->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  arc->dashlength);
  
  if (arc->start_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "start_arrow"),
		  arc->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  arc->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  arc->start_arrow.width);
  }
  if (arc->end_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "end_arrow"),
		  arc->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  arc->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  arc->end_arrow.width);
  }
}

static Object *
arc_load(ObjectNode obj_node, int version, const char *filename)
{
  Arc *arc;
  Connection *conn;
  Object *obj;
  AttributeNode attr;

  arc = g_malloc(sizeof(Arc));

  conn = &arc->connection;
  obj = (Object *) arc;

  obj->type = &arc_type;;
  obj->ops = &arc_ops;

  connection_load(conn, obj_node);

  arc->arc_color = color_black;
  attr = object_find_attribute(obj_node, "arc_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &arc->arc_color);

  arc->curve_distance = 0.1;
  attr = object_find_attribute(obj_node, "curve_distance");
  if (attr != NULL)
    arc->curve_distance = data_real(attribute_first_data(attr));

  arc->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    arc->line_width = data_real(attribute_first_data(attr));

  arc->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    arc->line_style = data_enum(attribute_first_data(attr));

  arc->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    arc->dashlength = data_real(attribute_first_data(attr));


  arc->start_arrow.type = ARROW_NONE;
  arc->start_arrow.length = 0.8;
  arc->start_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    arc->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    arc->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    arc->start_arrow.width = data_real(attribute_first_data(attr));

  arc->end_arrow.type = ARROW_NONE;
  arc->end_arrow.length = 0.8;
  arc->end_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    arc->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    arc->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    arc->end_arrow.width = data_real(attribute_first_data(attr));

  connection_init(conn, 3, 0);

  obj->handles[2] = &arc->middle_handle;
  arc->middle_handle.id = HANDLE_MIDDLE;
  arc->middle_handle.type = HANDLE_MINOR_CONTROL;
  arc->middle_handle.connect_type = HANDLE_NONCONNECTABLE;
  arc->middle_handle.connected_to = NULL;

  arc_update_data(arc);

  return (Object *)arc;
}


