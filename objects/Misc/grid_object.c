/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Grid object
 * Copyright (C) 2008 Don Blaheta
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

#include <math.h>
#include <string.h>
#include <glib.h>
#include <time.h>
#include <stdio.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "color.h"
#include "properties.h"

#include "pixmaps/grid_object.xpm"

#define GRID_OBJECT_BASE_CONNECTION_POINTS 9

typedef struct _Grid_Object {
  Element element;

  ConnectionPoint base_cps[9];
  gint cells_rows;
  gint cells_cols;
  ConnectionPoint *cells;

  Color border_color;
  real border_line_width;
  Color inner_color;
  gboolean show_background;
  gint grid_rows;
  gint grid_cols;
  Color gridline_color;
  real gridline_width;
} Grid_Object;

static real grid_object_distance_from(Grid_Object *grid_object,
                                       Point *point);

static void grid_object_select(Grid_Object *grid_object,
                                Point *clicked_point,
                                DiaRenderer *interactive_renderer);
static DiaObjectChange* grid_object_move_handle(Grid_Object *grid_object,
					      Handle *handle, Point *to,
					      ConnectionPoint *cp, HandleMoveReason reason,
                                     ModifierKeys modifiers);
static DiaObjectChange* grid_object_move(Grid_Object *grid_object, Point *to);
static void grid_object_draw(Grid_Object *grid_object, DiaRenderer *renderer);
static void grid_object_update_data(Grid_Object *grid_object);
static DiaObject *grid_object_create(Point *startpoint,
                                   void *user_data,
                                   Handle **handle1,
                                   Handle **handle2);
static void grid_object_reallocate_cells (Grid_Object* grid_object);
static void grid_object_destroy(Grid_Object *grid_object);
static DiaObject *grid_object_load(ObjectNode obj_node, int version,
                                   DiaContext *ctx);
static PropDescription *grid_object_describe_props(
  Grid_Object *grid_object);
static void grid_object_get_props(Grid_Object *grid_object,
                                   GPtrArray *props);
static void grid_object_set_props(Grid_Object *grid_object,
                                   GPtrArray *props);

static ObjectTypeOps grid_object_type_ops =
{
  (CreateFunc) grid_object_create,
  (LoadFunc)   grid_object_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType grid_object_type =
{
  "Misc - Grid",       /* name */
  0,                /* version */
  grid_object_xpm,   /* pixmap */
  &grid_object_type_ops /* ops */
};

static ObjectOps grid_object_ops = {
  (DestroyFunc)         grid_object_destroy,
  (DrawFunc)            grid_object_draw,
  (DistanceFunc)        grid_object_distance_from,
  (SelectFunc)          grid_object_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            grid_object_move,
  (MoveHandleFunc)      grid_object_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   grid_object_describe_props,
  (GetPropsFunc)        grid_object_get_props,
  (SetPropsFunc)        grid_object_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropNumData rows_columns_range = { 1, G_MAXINT, 1 };

static PropDescription grid_object_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,

  { "grid_rows", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
    N_("Rows"), NULL, &rows_columns_range },
  { "grid_cols", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
    N_("Columns"), NULL, &rows_columns_range },
  { "gridline_colour", PROP_TYPE_COLOUR, PROP_FLAG_VISIBLE,
    N_("Grid line color"), NULL, NULL },
  { "gridline_width", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Grid line width"), NULL, &prop_std_line_width_data },

  {NULL}
};

static PropDescription *
grid_object_describe_props(Grid_Object *grid_object)
{
  if (grid_object_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(grid_object_props);
  }
  return grid_object_props;
}

static PropOffset grid_object_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Grid_Object, border_line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Grid_Object, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Grid_Object,inner_color) },
  { "show_background", PROP_TYPE_BOOL,offsetof(Grid_Object,show_background) },
  { "grid_rows", PROP_TYPE_INT, offsetof(Grid_Object, grid_rows) },
  { "grid_cols", PROP_TYPE_INT, offsetof(Grid_Object, grid_cols) },
  { "gridline_colour", PROP_TYPE_COLOUR, offsetof(Grid_Object, gridline_color) },
  { "gridline_width", PROP_TYPE_REAL, offsetof(Grid_Object,
                                                 gridline_width) },

  {NULL}
};

static void
grid_object_get_props(Grid_Object *grid_object, GPtrArray *props)
{
  object_get_props_from_offsets(&grid_object->element.object,
                                grid_object_offsets,props);
}

static void
grid_object_set_props(Grid_Object *grid_object, GPtrArray *props)
{
  DiaObject *obj = &grid_object->element.object;

  object_set_props_from_offsets(obj, grid_object_offsets,props);

  grid_object_reallocate_cells(grid_object);

  grid_object_update_data(grid_object);
}

static real
grid_object_distance_from(Grid_Object *grid_object, Point *point)
{
  DiaObject *obj = &grid_object->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
grid_object_select(Grid_Object *grid_object, Point *clicked_point,
		    DiaRenderer *interactive_renderer)
{
  element_update_handles(&grid_object->element);
}

static DiaObjectChange*
grid_object_move_handle(Grid_Object *grid_object, Handle *handle,
			 Point *to, ConnectionPoint *cp,
			 HandleMoveReason reason, ModifierKeys modifiers)
{
  g_assert(grid_object!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  element_move_handle(&grid_object->element, handle->id, to, cp,
		      reason, modifiers);
  grid_object_update_data(grid_object);

  return NULL;
}

static DiaObjectChange*
grid_object_move(Grid_Object *grid_object, Point *to)
{
  grid_object->element.corner = *to;
  grid_object_update_data(grid_object);

  return NULL;
}

/** Converts 2D indices into 1D.  Currently row-major. */
inline static int grid_cell (int i, int j, int rows, int cols)
{
  return j * cols + i;
}

static void
grid_object_update_data(Grid_Object *grid_object)
{
  Element *elem = &grid_object->element;
  DiaObject *obj = &elem->object;
  ElementBBExtras *extra = &elem->extra_spacing;

  real inset = (grid_object->border_line_width - grid_object->gridline_width)/2.0;
  real cell_width = (elem->width - 2.0 * inset) / grid_object->grid_cols;
  real cell_height = (elem->height - 2.0 * inset) / grid_object->grid_rows;
  int i, j;
  double left, top;

  extra->border_trans = grid_object->border_line_width / 2.0;
  element_update_boundingbox(elem);
  element_update_handles(elem);
  element_update_connections_rectangle(elem, grid_object->base_cps);

  obj->position = elem->corner;
  left = obj->position.x;
  top = obj->position.y;
  for (i = 0; i < grid_object->grid_cols; ++i)
    for (j = 0; j < grid_object->grid_rows; ++j)
    {
      int cell = grid_cell(i, j, grid_object->grid_rows, grid_object->grid_cols);
      grid_object->cells[cell].pos.x =
			left + inset + i*cell_width + cell_width/2.0;
      grid_object->cells[cell].pos.y =
			top + inset + j*cell_height + cell_height/2.0;
    }
}

static void
grid_object_draw_gridlines (Grid_Object *grid_object,
                            DiaRenderer *renderer,
                            Point       *lr_corner)
{
  Element *elem;
  Point st, fn;
  real cell_size;
  unsigned i;
  real inset;

  elem = &grid_object->element;

  /* The goal is to have all cells equal size; if the border line is
   * much wider than the gridline, the outer course of cells will be
   * visibly smaller.  So we "inset" them by a little bit: */
  inset = (grid_object->border_line_width - grid_object->gridline_width)/2;

  /* horizontal gridlines */
  st.x = elem->corner.x;
  st.y = elem->corner.y + inset;
  fn.x = elem->corner.x + elem->width;
  fn.y = elem->corner.y + inset;

  cell_size = (elem->height - 2 * inset)
              / grid_object->grid_rows;
  if (cell_size < 0)
    cell_size = 0;
  for (i = 1; i < grid_object->grid_rows; ++i) {
    st.y += cell_size;
    fn.y += cell_size;
    dia_renderer_draw_line (renderer,&st,&fn,&grid_object->gridline_color);
  }

  /* vertical gridlines */
  st.x = elem->corner.x + inset;
  st.y = elem->corner.y;
  fn.x = elem->corner.x + inset;
  fn.y = elem->corner.y + elem->height;

  cell_size = (elem->width - 2 * inset)
              / grid_object->grid_cols;
  if (cell_size < 0)
    cell_size = 0;
  for (i = 1; i < grid_object->grid_cols; ++i) {
    st.x += cell_size;
    fn.x += cell_size;
    dia_renderer_draw_line (renderer, &st, &fn, &grid_object->gridline_color);
  }
}

static void
grid_object_draw (Grid_Object *grid_object, DiaRenderer *renderer)
{
  Element *elem;
  Point lr_corner;

  g_assert(grid_object != NULL);
  g_assert(renderer != NULL);

  elem = &grid_object->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  /* draw gridlines */
  dia_renderer_set_linewidth (renderer,
                              grid_object->gridline_width);
  grid_object_draw_gridlines (grid_object,
                              renderer,
                              &lr_corner);

  /* draw outline */
  dia_renderer_set_linewidth (renderer,
                              grid_object->border_line_width);
  dia_renderer_draw_rect (renderer,
                          &elem->corner,
                          &lr_corner,
                          (grid_object->show_background) ? &grid_object->inner_color : NULL,
                          &grid_object->border_color);
}


static DiaObject *
grid_object_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Grid_Object *grid_object;
  Element *elem;
  DiaObject *obj;
  unsigned i;

  grid_object = g_new0(Grid_Object,1);
  elem = &(grid_object->element);

  obj = &(grid_object->element.object);
  obj->type = &grid_object_type;
  obj->ops = &grid_object_ops;

  elem->corner = *startpoint;
  elem->width = 4.0;
  elem->height = 4.0;

  element_init(elem, 8, 9);

  grid_object->border_color = attributes_get_foreground();
  grid_object->border_line_width = attributes_get_default_linewidth();
  grid_object->inner_color = attributes_get_background();
  grid_object->show_background = TRUE;
  grid_object->grid_rows = 3;
  grid_object->grid_cols = 4;
  grid_object->gridline_color.red = 0.5;
  grid_object->gridline_color.green = 0.5;
  grid_object->gridline_color.blue = 0.5;
  grid_object->gridline_color.alpha = 1.0;
  grid_object->gridline_width = attributes_get_default_linewidth();

  for (i = 0; i < 9; ++i)
  {
    obj->connections[i] = &grid_object->base_cps[i];
    grid_object->base_cps[i].object = obj;
    grid_object->base_cps[i].connected = NULL;
  }
  grid_object->base_cps[8].flags = CP_FLAGS_MAIN;

  grid_object->cells_rows = 0;
  grid_object->cells_cols = 0;
  grid_object->cells = NULL;
  grid_object_reallocate_cells(grid_object);

  grid_object_update_data(grid_object);

  *handle1 = NULL;
  *handle2 = obj->handles[7];

  return &grid_object->element.object;
}

static void
connectionpoint_init (ConnectionPoint* cp, DiaObject* obj)
{
  cp->object = obj;
  cp->connected = NULL;
  cp->directions = DIR_ALL;
  cp->flags = 0;
}

static void
connectionpoint_update(ConnectionPoint* newcp, ConnectionPoint* oldcp)
{
  GList* cur;
  newcp->connected = oldcp->connected;

  cur = newcp->connected;  /* GList of DO* */
  while (cur != NULL)
  {
    DiaObject* connecting_obj = g_list_nth_data(cur, 0);
    int i;
    for (i = 0; i < connecting_obj->num_handles; ++i)
    {
      if (connecting_obj->handles[i]->connected_to == oldcp)
      { /* this is the inbound connection */
        connecting_obj->handles[i]->connected_to = newcp;
      }
    }

    cur = g_list_next(cur);
  }
}

/** Adjusts allocations for connection points; does not actually compute
  * them.  This call should always be followed with a call to update_data. */
static void
grid_object_reallocate_cells (Grid_Object* grid_object)
{
  DiaObject* obj = &(grid_object->element.object);
  int old_rows = grid_object->cells_rows;
  int old_cols = grid_object->cells_cols;
  int new_rows = grid_object->grid_rows;
  int new_cols = grid_object->grid_cols;
  int i, j;
  ConnectionPoint* new_cells;

  if (old_rows == new_rows && old_cols == new_cols)
    return;  /* no reallocation necessary */

  /* If either new dimension is smaller, some connpoints will have to
   * be disconnected before reallocating */

  /* implicit: if (new_rows < old_rows) */
  for (j = new_rows; j < old_rows; ++j)
    for (i = 0; i < old_cols; ++i)
    {
      int cell = grid_cell(i, j, old_rows, old_cols);
      object_remove_connections_to(&grid_object->cells[cell]);
    }

  /* implicit: if (new_cols < old_cols) */
  for (i = new_cols; i < old_cols; ++i)
    for (j = 0; j < old_rows && j < new_rows; ++j) /* don't double-delete */
    {
      int cell = grid_cell(i, j, old_rows, old_cols);
      object_remove_connections_to(&grid_object->cells[cell]);
    }

  /* must be done after disconnecting */
  /* obj->connections doesn't own the pointers, so just realloc; values
   * will be updated later */
  obj->num_connections = GRID_OBJECT_BASE_CONNECTION_POINTS + new_rows*new_cols;
  obj->connections = g_renew (ConnectionPoint *,
                              obj->connections,
                              obj->num_connections);

  /* Can't use realloc; if grid has different dims, memory lays out
   * differently.  Must copy by hand. */

  new_cells = g_new0 (ConnectionPoint, new_rows * new_cols);
  for (i = 0; i < new_cols; ++i)
    for (j = 0; j < new_rows; ++j)
    {
      int oldloc = grid_cell(i,j,old_rows,old_cols);
      int newloc = grid_cell(i,j,new_rows,new_cols);
      connectionpoint_init(&new_cells[newloc], obj);
      obj->connections[GRID_OBJECT_BASE_CONNECTION_POINTS + newloc] =
	&new_cells[newloc];
      if (i < old_cols && j < old_rows)
      {
	connectionpoint_update(&new_cells[newloc], &grid_object->cells[oldloc]);
      }
    }

  g_clear_pointer (&grid_object->cells, g_free);
  grid_object->cells = new_cells;
  grid_object->cells_rows = new_rows;
  grid_object->cells_cols = new_cols;
}

static void
grid_object_destroy(Grid_Object *grid_object)
{
  element_destroy(&grid_object->element);
  g_clear_pointer (&grid_object->cells, g_free);
}

static DiaObject *
grid_object_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&grid_object_type,
                                      obj_node,version,ctx);
}
