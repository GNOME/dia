/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * n_gon.c -- an equilateral polygon or star (polygram?)
 * Copyright (C) 2014  Hans Breuer <hans@breuer.org>
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "object.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "prop_inttypes.h" /* PropEventHandler needs internals */
#include "boundingbox.h"
#include "element.h"
#include "pattern.h"

#include "pixmaps/n_gon.xpm"

typedef enum {
  NGON_CONVEX = 0, /*!< triangle, square, hexagon, ... */
  NGON_CONCAVE,    /*!< concave aka. a star (num_rays >= 5) */
  NGON_CROSSING    /*!< concave with crossing rays  */
} NgonKind;

#define NUM_HANDLES     9
#define NUM_CONNECTIONS 1

typedef struct _Ngon Ngon;

/*!
 * \brief Equilateral Polygon
 * \extends _Element
 */
struct _Ngon {
  /*! \protected _Element must be first because this is a 'subclass' of it. */
  Element    element;

  ConnectionPoint connections[NUM_CONNECTIONS];
  Handle center_handle;

  int        num_rays;
  NgonKind   kind;
  int        density;
  int        last_density; /*!< last value to decide direction */

  DiaLineStyle line_style;
  DiaLineJoin  line_join;
  double     dashlength;
  double     line_width;
  Color      stroke;
  Color      fill;
  gboolean   show_background;
  DiaPattern *pattern;

  /*! To recalculate the points, it's also the object.position */
  Point       center;
  /*! The radius of the circumscribed circle */
  real        ray_len;
  /*! \private Calculated points cache */
  GArray     *points;
  /*! A generated name */
  char       *name;
};

static DiaObject *
_ngon_create (Point *startpoint,
	      void *user_data,
	      Handle **handle1,
	      Handle **handle2);
static DiaObject *
_ngon_load (ObjectNode obj_node, int version, DiaContext *ctx);
static void
_ngon_save (DiaObject *obj, ObjectNode obj_node, DiaContext *ctx);

static ObjectTypeOps _ngon_type_ops =
{
  (CreateFunc) _ngon_create,
  (LoadFunc)   _ngon_load, /* can't use object_load_using_properties, signature mismatch */
  (SaveFunc)   _ngon_save, /* overwrite for filename normalization */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static void _ngon_update_data (Ngon *ng);

static PropEnumData _ngon_type_data[] = {
  { N_("Convex"), NGON_CONVEX },
  { N_("Concave"), NGON_CONCAVE },
  { N_("Crossing"), NGON_CROSSING },
  { NULL, }
};
static PropNumData _num_rays_range_data = { 3, 360, 1 };
static PropNumData _density_range_data = { 2, 180, 1 };

/* PropEventHandler to keep the density property valid */
static gboolean _ngon_density_constraints_handler (DiaObject *obj, Property *prop);

static PropDescription _ngon_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "ngon_kind", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("N-gon kind"), NULL, &_ngon_type_data },
  { "num_rays", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
    N_("Number of rays"), NULL, &_num_rays_range_data },
  { "density", PROP_TYPE_INT, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Density"), N_("Winding number for Crossing"), &_density_range_data,
    &_ngon_density_constraints_handler },
  { "center", PROP_TYPE_POINT, PROP_FLAG_NO_DEFAULTS, /* no property widget, but still to be serialized */
    N_("Center position"), NULL, NULL },
  { "ray_len", PROP_TYPE_REAL, PROP_FLAG_NO_DEFAULTS, /* no property widget, but still to be serialized */
    N_("Ray length"), NULL, NULL },
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_STYLE_OPTIONAL,
  PROP_STD_LINE_JOIN_OPTIONAL,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_STD_PATTERN,
  { "name", PROP_TYPE_STRING, PROP_FLAG_DONT_SAVE| PROP_FLAG_OPTIONAL | PROP_FLAG_NO_DEFAULTS,
  N_("Name"), NULL, NULL },
  PROP_DESC_END
};
static PropDescription *
_ngon_describe_props(Ngon *ng)
{
  if (_ngon_props[0].quark == 0)
    prop_desc_list_calculate_quarks(_ngon_props);
  return _ngon_props;
}
static PropOffset _ngon_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "ngon_kind", PROP_TYPE_ENUM, offsetof(Ngon, kind) },
  { "num_rays", PROP_TYPE_INT, offsetof(Ngon, num_rays) },
  { "density", PROP_TYPE_INT, offsetof(Ngon, density) },
  { "center", PROP_TYPE_POINT, offsetof(Ngon, center) },
  { "ray_len", PROP_TYPE_REAL, offsetof(Ngon, ray_len) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Ngon, line_width) },
  { "line_style", PROP_TYPE_LINESTYLE, offsetof(Ngon, line_style), offsetof(Ngon, dashlength) },
  { "line_join", PROP_TYPE_ENUM, offsetof(Ngon, line_join) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Ngon, stroke) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Ngon, fill) },
  { "show_background", PROP_TYPE_BOOL,offsetof(Ngon, show_background) },
  { "pattern", PROP_TYPE_PATTERN, offsetof(Ngon, pattern) },
  { "name", PROP_TYPE_STRING, offsetof(Ngon, name) },
  { NULL }
};
static void
_ngon_get_props(Ngon *ng, GPtrArray *props)
{
  object_get_props_from_offsets(&ng->element.object, _ngon_offsets, props);
}
static void
_ngon_set_props(Ngon *ng, GPtrArray *props)
{
  ng->last_density = ng->density;
  object_set_props_from_offsets(&ng->element.object, _ngon_offsets, props);
  _ngon_update_data(ng);
}
static real
_ngon_distance_from (Ngon *ng, Point *point)
{
  g_return_val_if_fail (ng->points->len >= 3, 1.0);
  return distance_polygon_point(&g_array_index (ng->points, Point, 0), ng->points->len,
				ng->line_width, point);
}
static void
_ngon_select(Ngon *ng, Point *clicked_point, DiaRenderer *interactive_renderer)
{
  element_update_handles(&ng->element);
}


static DiaObjectChange *
_ngon_move_handle (Ngon             *ng,
                   Handle           *handle,
                   Point            *to,
                   ConnectionPoint  *cp,
                   HandleMoveReason  reason,
                   ModifierKeys      modifiers)
{
  Element *elem = &ng->element;
  DiaObjectChange *change = NULL; /* stays NULL for fully reversible move handle */

  g_return_val_if_fail (handle!=NULL, NULL);
  g_return_val_if_fail (to!=NULL, NULL);

  /* resize or moving center handle? */
  if (handle->id == HANDLE_CUSTOM1) {
    ng->center = *to;
  } else {
    /* handle is not moved yet(?), so we can use that to scale towards to */
    real d0 = distance_point_point (&handle->pos, &ng->center);
    real d1 = distance_point_point (to, &ng->center);
    ng->ray_len *= (d1 / d0);
    /* not sure if this is useful at all, but we must not do it with our center_handle */
    change = element_move_handle (elem, handle->id, to, cp, reason, modifiers);
  }

  _ngon_update_data (ng);

  return change;
}


/*!
 * \brief Move the object, to is relative to former object position
 */
static DiaObjectChange *
_ngon_move (Ngon *ng, Point *to)
{
  ng->center = *to;
  _ngon_update_data (ng);

  return NULL;
}


static void
_ngon_draw (Ngon *ng, DiaRenderer *renderer)
{
  gboolean pattern_fill = ng->show_background
              && ng->pattern != NULL
              && dia_renderer_is_capable_of (renderer, RENDER_PATTERN);
  Color fill;

  g_return_if_fail (ng->points->len);

  dia_renderer_set_linewidth (renderer, ng->line_width);
  dia_renderer_set_linestyle (renderer, ng->line_style, ng->dashlength);
  dia_renderer_set_linejoin (renderer, ng->line_join);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);
  if (ng->pattern) {
    dia_pattern_get_fallback_color (ng->pattern, &fill);
  } else {
    fill = ng->fill;
  }
  if (pattern_fill) {
    dia_renderer_set_pattern (renderer, ng->pattern);
  }
  dia_renderer_draw_polygon (renderer,
                             &g_array_index (ng->points, Point, 0),
                             ng->points->len,
                             ng->show_background ? &fill : NULL,
                             &ng->stroke);
  if (pattern_fill) {
    dia_renderer_set_pattern (renderer, NULL);
  }
}

/* greatest common divider */
static int
_gcd (int a, int b)
{
  while (b != 0) {
    real t = b;
    b = a % b;
    a = t;
  }
  return a;
}
static int
_calc_step (int a, int b)
{
  if (b > a / 2)
    b = a / 2;
  while (_gcd (a, b) != 1)
    --b;
  return b;
}
static int
_calc_step_up (int a, int b)
{
  while (_gcd (a, b) != 1)
    ++b;
  return b;
}

/*!
 * \brief Event handler called for property change by user
 *
 * The n-gon density value depends on num_rays. Only a limited set of values
 * can be supported, where min/max/step is not enough to reflect it.
 *
 * This function gets called between Ngon::set_props() and
 * Ngon::get_props(). It does need to do anything cause Ngon::update_data()
 * already ensured data consistency. The pure existence of this of this
 * property handler leads to bidirectional communication between property
 * dialog and object's property change.
 */
static gboolean
_ngon_density_constraints_handler (DiaObject *obj, Property *prop)
{
  Ngon *ng = (Ngon *)obj;
  IntProperty *p = (IntProperty *)prop;
  int maxDensity = _calc_step (ng->num_rays, ng->num_rays / 2);

  g_return_val_if_fail (strcmp(prop->descr->type, PROP_TYPE_INT) == 0, FALSE);

  if (p->int_data > maxDensity) {
    ng->density = maxDensity;
  }
  return TRUE;
}

static void
_ngon_make_name (Ngon *ng)
{
  static struct {
    int         v;
    const char *n0;
    const char *n1;
  } _keys[] = {
    {  3, N_("Triangle"), },
    {  4, N_("Square"), },
    {  5, N_("Pentagon"),     N_("Pentagram") },
    {  6, N_("Hexagon"),      N_("Hexagram") },
    {  7, N_("Heptagon"),     N_("Heptagram") },
    {  8, N_("Octagon"),      N_("Octagram") },
    {  9, N_("Enneagon"),     N_("Enneagram") },
    { 10, N_("Decagon"),      N_("Decagram") },
    { 11, N_("Hendecagon"),   N_("Hendecagram") },
    { 12, N_("Dodecagon"),    N_("Dodecagram") },
    { 13, N_("Tridecagon"),   N_("Tridecagram") },
    { 14, N_("Tetradecagon"), N_("Tetradecagram") },
    { 15, N_("Pentadecagon"), N_("Pentadecagram") },
    { 16, N_("Hexadecagon"),  N_("Hexadecagram") },
    { 17, N_("Heptadecagon"), N_("Heptadecagram") },
    { 18, N_("Octadecagon"),  N_("Octadecagram") },
    { 19, N_("Enneadecagon"), N_("Enneadecagram") },
    { 20, N_("Icosagon"),     N_("Icosagram") },
  };
  const char *name = NULL;
  int i = 0;

  g_clear_pointer (&ng->name, g_free);
  for (i = 0; i < G_N_ELEMENTS(_keys); ++i) {
    if (_keys[i].v == ng->num_rays) {
      if (ng->kind == NGON_CONVEX)
	name = _keys[i].n0;
      else if (ng->kind == NGON_CROSSING)
	name = _keys[i].n1;
    }
  }
  if (!name) {
    if (ng->kind == NGON_CONVEX)
      name =  _("N-gon");
    else if (ng->kind == NGON_CROSSING)
      name =  _("N-gram");
    else
      name =  _("Star");
  }

  if (ng->kind == NGON_CONVEX)
    ng->name = g_strdup_printf ("%s {%d}", name, ng->num_rays);
  else
    ng->name = g_strdup_printf ("%s {%d/%d}", name, ng->num_rays, ng->density);
}

/*!
 * \brief Sort points to produce a star with crossing edges
 *
 * We support the different possibilities explained by
 * http://en.wikipedia.org/wiki/Star_polygon
 * via the density parameter mostly.
 */
static void
_ngon_adjust_for_crossing (Ngon *ng)
{
  int n = ng->points->len, i, step = _calc_step (ng->num_rays, ng->density);
  GArray *points = g_array_new (FALSE /* zero_terminated */,
			        FALSE /* clear_ */, sizeof(Point));

  /* copy already preset points */
  g_array_insert_vals (points, 0,
		       &g_array_index (ng->points, Point, 0),
		       ng->points->len);
  if (1 == step && n > 5 && (n % 2) == 0) {
    /* calculate the crossing of edges as extra point */
    Point crossing;
    if (!line_line_intersection (&crossing,
				 &g_array_index (points, Point, 0), &g_array_index (points, Point, n-2),
				 &g_array_index (points, Point, 1), &g_array_index (points, Point, n-1)))
      g_warning ("No intersection?");
    step = 2;
    for (i = 0; i < n/2; ++i) {
      int j = (i * step) % n;
      g_array_index (ng->points, Point, i).x = g_array_index (points, Point, j).x;
      g_array_index (ng->points, Point, i).y = g_array_index (points, Point, j).y;
    }
    /* go backward for the second half */
    for (i = 0; i < n/2; ++i) {
      int j = (n - 1 - i * step) % n;
      g_array_index (ng->points, Point, i + n/2).x = g_array_index (points, Point, j).x;
      g_array_index (ng->points, Point, i + n/2).y = g_array_index (points, Point, j).y;
    }
    /* insert the crossing point at the end */
    g_array_insert_val (ng->points, n, crossing);
    /* insert the crossing point in the middle */
    g_array_insert_val (ng->points, n/2, crossing);
  } else {
    for (i = 1; i < n; ++i) {
      int j = (i * step) % n;
      g_array_index (ng->points, Point, i).x = g_array_index (points, Point, j).x;
      g_array_index (ng->points, Point, i).y = g_array_index (points, Point, j).y;
    }
  }
  g_array_free (points, TRUE);
}
static void
_ngon_update_data (Ngon *ng)
{
  Element *elem;
  int n, i;
  real r;

  elem = &ng->element;
  if (ng->ray_len < 0.01) /* ensure minimum size ... */
    ng->ray_len = 0.01;   /* otherwise we can't scale it anymore */
  r = ng->ray_len;

  if (ng->kind != NGON_CONCAVE)
    n = ng->num_rays;
  else
    n = ng->num_rays * 2;

  /* ensure density stays in range */
  if (ng->last_density > ng->density) {
    real temp = _calc_step (ng->num_rays, ng->density);
    /* special case for Hexagram and above */
    if (temp == 1 && ng->kind == NGON_CROSSING && ng->num_rays > 5)
      temp = 2;
    ng->density = temp;
  } else {
    ng->density = _calc_step_up (ng->num_rays, ng->density);
  }
  _ngon_make_name (ng);
  if (1 || n != ng->points->len) {
    /* recalculate all points */
    Point ct = ng->center;
    real da = 2 * M_PI / ng->num_rays;

    g_array_set_size (ng->points, n);

    if (ng->kind != NGON_CONCAVE) {
      for (i = 0; i < ng->num_rays; ++i) {
	real a = i * da;
	g_array_index (ng->points, Point, i).x =  sin(a) * r + ct.x;
	g_array_index (ng->points, Point, i).y = -cos(a) * r + ct.y;
      }
      /* sort/extend points for other form */
      if (ng->kind == NGON_CROSSING)
	_ngon_adjust_for_crossing (ng);
    } else {
      da /= 2.0; /* we are doing bigger steps here */
      for (i = 0; i < n; i+=2) {
	real a = i * da;
	g_array_index (ng->points, Point, i).x =  sin(a) * r + ct.x;
	g_array_index (ng->points, Point, i).y = -cos(a) * r + ct.y;
	g_array_index (ng->points, Point, i+1).x =  sin(a+da) * r * .4 + ct.x;
	g_array_index (ng->points, Point, i+1).y = -cos(a+da) * r * .4 + ct.y;
      }
    }
  }
  /* update bounding box */
  {
    PolyBBExtras extra;
    extra.start_trans = extra.end_trans = 0;
    extra.middle_trans = ng->line_width / 2.0;
    extra.start_long = extra.end_long = 0;
    polyline_bbox (&g_array_index (ng->points, Point, 0),
		   ng->points->len,
		   &extra, TRUE,
		   &elem->object.bounding_box);
  }
  elem->object.position = ng->center;
  ng->center_handle.pos = ng->center;
  ng->connections[NUM_CONNECTIONS-1].pos = ng->center;
  /* update element members based on circumscribed circle */
  elem->corner.x = ng->center.x - ng->ray_len;
  elem->corner.y = ng->center.y - ng->ray_len;
  elem->width = elem->height = 2 * ng->ray_len;
  element_update_handles(elem);

}


static void
_ngon_destroy(Ngon *ng)
{
  g_array_free (ng->points, TRUE);
  g_clear_object (&ng->pattern);
  g_clear_pointer (&ng->name, g_free);
  element_destroy(&ng->element);
}


static DiaObject *
_ngon_copy(Ngon *from)
{
  DiaObject *newobj = object_copy_using_properties (&from->element.object);
  Ngon *ng = (Ngon *)newobj;
  int i;
  /* setup our extra handle */
  newobj->handles[NUM_HANDLES-1] = &ng->center_handle;
  /* and connections */
  for (i=0; i<NUM_CONNECTIONS; i++) {
    newobj->connections[i] = &ng->connections[i];
    ng->connections[i].object = newobj;
    ng->connections[i].connected = NULL;
  }
  ng->connections[NUM_CONNECTIONS-1].flags = CP_FLAGS_MAIN;

  return newobj;
}

static ObjectOps _ngon_ops = {
  (DestroyFunc)         _ngon_destroy,
  (DrawFunc)            _ngon_draw,
  (DistanceFunc)        _ngon_distance_from,
  (SelectFunc)          _ngon_select,
  (CopyFunc)            _ngon_copy,
  (MoveFunc)            _ngon_move,
  (MoveHandleFunc)      _ngon_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   _ngon_describe_props,
  (GetPropsFunc)        _ngon_get_props,
  (SetPropsFunc)        _ngon_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

DiaObjectType _ngon_type =
{
  "Misc - Ngon",  /* name */
  1,           /* version */
  n_gon_xpm,    /* pixmap */
  &_ngon_type_ops, /* ops */
  NULL,    /* pixmap_file */
  0  /* default_uder_data */
};

/*! factory function */
static DiaObject *
_ngon_create (Point *startpoint,
	      void *user_data,
	      Handle **handle1,
	      Handle **handle2)
{
  /* XXX: think about user_data handling for alias e.g. of some Assorted */
  Ngon *ng;
  Element *elem;
  DiaObject *obj;
  int i;

  ng = g_new0 (Ngon, 1);
  obj = &ng->element.object;
  obj->type = &_ngon_type;
  obj->ops = &_ngon_ops;
  elem = &ng->element;

  element_init (elem, NUM_HANDLES, NUM_CONNECTIONS);
  obj->handles[NUM_HANDLES-1] = &ng->center_handle;
  obj->handles[NUM_HANDLES-1]->id = HANDLE_CUSTOM1;
  obj->handles[NUM_HANDLES-1]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[NUM_HANDLES-1]->connected_to = NULL;
  obj->handles[NUM_HANDLES-1]->connect_type = HANDLE_NONCONNECTABLE;

  for (i=0; i<NUM_CONNECTIONS; i++) {
    obj->connections[i] = &ng->connections[i];
    ng->connections[i].object = obj;
    ng->connections[i].connected = NULL;
  }
  ng->connections[NUM_CONNECTIONS-1].flags = CP_FLAGS_MAIN;

  ng->points = g_array_new (FALSE /* zero_terminated */,
			    FALSE /* clear_ */, sizeof(Point));
  ng->kind = NGON_CONVEX;
  ng->num_rays = 5;
  ng->last_density = ng->density = _calc_step (ng->num_rays, ng->num_rays / 2);
  ng->ray_len = 1.0; /* for intial object size */
  ng->center = *startpoint;

  ng->line_width =  attributes_get_default_linewidth();
  ng->stroke = attributes_get_foreground();
  ng->fill = attributes_get_background();
  attributes_get_default_line_style(&ng->line_style, &ng->dashlength);

  _ngon_update_data(ng);

  *handle1 = obj->handles[NUM_HANDLES-1]; /* center point */
  *handle2 = obj->handles[NUM_HANDLES-2]; /* lower right corner */
  return &ng->element.object;
}

static DiaObject *
_ngon_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  DiaObject *obj;
  Ngon *ng;

  obj = object_load_using_properties (&_ngon_type, obj_node, version, ctx);
  ng = (Ngon *)obj;
  if (version == 0) { /* default to maximum */
    ng->last_density = ng->density = _calc_step (ng->num_rays, ng->num_rays/2);
    _ngon_update_data(ng);
  }
  /* the density value is optional, so calculate if not valid */
  if (_calc_step (ng->num_rays, ng->density) != ng->density)
    ng->density = _calc_step (ng->num_rays, ng->num_rays/2);
  return obj;
}

static void
_ngon_save (DiaObject *obj, ObjectNode obj_node, DiaContext *ctx)
{
  object_save_using_properties (obj, obj_node, ctx);
}
