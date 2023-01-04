/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * layout.cpp -  registeration and binding of OGDF layout algoritms as Dia plug-in
 * Copyright (c) 2011 Hans Breuer <hans@breuer.org>
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

/*!
 * \file layout.cpp - plugin for automatic diagram layout
 */
/*!
 * \defgroup LayoutPlugin Graph Layout Plugin
 * \brief Connecting external graph layout libraries with Dia
 *
 * \ingroup Plugins
 *
 * Dia's core facilities to do automatic layout is highly limited. This
 * plug-in allows to connect external C++ libraries with Dia's objects
 * simplified to nodes and edges. Changes done by the layout algorithm
 * are connected with Dia's undo/redo system.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "message.h"
#include "filter.h"
#include "plug-ins.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"

#include "../app/diagram.h"
#include "../app/connectionpoint_ops.h"

#include "ogdf-simple.h"
#include "dia-graph.h"
#include "dia-object-change-list.h"

#include <vector>

static gboolean
maybe_edge (DiaObject *object)
{
  int i, nhc = 0;

  for (i = 0; i < object->num_handles; ++i) {
    if (object->handles[i]->connect_type == HANDLE_CONNECTABLE) {
      ++nhc;
      if (nhc > 1)
        return TRUE;
    }
  }
  return FALSE;
}

/*!
 * Translate from various Dia point representation to OGDF 'bends',
 * the latter not containing the first and last point I think.
 */
static int
_obj_get_bends (DiaObject *obj, std::vector<double>& coords)
{
  Property *prop = NULL;

  coords.resize(0);
  // no need to ask for Standard - Line: start_point, end_point
  // we always drop the very first and last point
  if ((prop = object_prop_by_name(obj, "orth_points")) != NULL ||
      (prop = object_prop_by_name(obj, "poly_points")) != NULL) {
    PointarrayProperty *ptp = (PointarrayProperty *)prop;
    int num = ptp->pointarray_data->len;

    for (int i = 1; i < num-1; ++i) {
      Point *pt = &g_array_index(ptp->pointarray_data, Point, i);
      coords.push_back (pt->x);
      coords.push_back (pt->y);
    }
  } else if ((prop = object_prop_by_name(obj, "bez_points")) != NULL) {
    BezPointarrayProperty *ptp = (BezPointarrayProperty *)prop;
    int num = ptp->bezpointarray_data->len;

    for (int i = 1; i < num-1; ++i) {
      BezPoint *bp = &g_array_index(ptp->bezpointarray_data, BezPoint, i);

      // TODO: better conversion from polyline to bezierline
      if (bp->type == BezPoint::BEZ_CURVE_TO) {
        coords.push_back (bp->p3.x);
        coords.push_back (bp->p3.y);
      } else {
        coords.push_back (bp->p1.x);
        coords.push_back (bp->p1.y);
      }
    }
  }

  if (prop)
    prop->ops->free(prop);
  return coords.size();
}


static DiaObjectChange *
_obj_set_bends (DiaObject *obj, std::vector<double>& coords)
{
  Property *prop = NULL;

  if ((prop = object_prop_by_name(obj, "poly_points")) != NULL) {
    PointarrayProperty *ptp = (PointarrayProperty *)prop;
    int num = ptp->pointarray_data->len;
    Point last  = g_array_index(ptp->pointarray_data, Point, num-1);
    // we keep the first and last point (the connected ones) and overwrite the rest
    num = coords.size()/2+2;

    g_array_set_size(ptp->pointarray_data, num);
    for (int i = 1; i < num-1; ++i) {
      Point *pt = &g_array_index(ptp->pointarray_data, Point, i);
      pt->x = coords[(i-1)*2];
      pt->y = coords[(i-1)*2+1];
    }
    g_array_index(ptp->pointarray_data, Point, num-1) = last;
  } else if ((prop = object_prop_by_name(obj, "orth_points")) != NULL) {
    PointarrayProperty *ptp = (PointarrayProperty *)prop;
    int num = ptp->pointarray_data->len;
    Point last  = g_array_index(ptp->pointarray_data, Point, num-1);
    // we keep the first and last point (the connected ones) and overwrite the rest
    num = coords.size()/2+2;

    // there must be at least 3 points with an orthconn, so we may have to create one
    // TODO: also maintain the orthogonality?
    if (num == 2) {
      Point first  = g_array_index(ptp->pointarray_data, Point, 0);
      Point pt = { (first.x + last.x) / 2, (first.y + last.y) / 2 };
      ++num;
      g_array_set_size(ptp->pointarray_data, num);
      g_array_index(ptp->pointarray_data, Point, num-2) = pt;
    } else {
      g_array_set_size(ptp->pointarray_data, num);
      for (int i = 1; i < num-1; ++i) {
	Point *pt = &g_array_index(ptp->pointarray_data, Point, i);
	pt->x = coords[(i-1)*2];
	pt->y = coords[(i-1)*2+1];
      }
    }
    g_array_index(ptp->pointarray_data, Point, num-1) = last;
  } else if ((prop = object_prop_by_name(obj, "bez_points")) != NULL) {
    BezPointarrayProperty *ptp = (BezPointarrayProperty *)prop;
    int num = ptp->bezpointarray_data->len;
    BezPoint last = g_array_index(ptp->bezpointarray_data, BezPoint, num-1);

    // we keep the first and last point (the connected ones) and overwrite the rest
    num = coords.size()/2+1;
    if (num == 1) {
      // still want to have two points - a straight line
      g_array_set_size(ptp->bezpointarray_data, 2);
      last.p1 = last.p3;
      last.p2 = g_array_index(ptp->bezpointarray_data, BezPoint, 0).p1;
      g_array_index(ptp->bezpointarray_data, BezPoint, 1) = last;
    } else {
      // the bends are used for control points ...
      Point p1;

      p1.x = coords[0];
      p1.y = coords[1];
      g_array_set_size(ptp->bezpointarray_data, num);
      for (int i = 1; i < num-1; ++i) {
	BezPoint *bp = &g_array_index(ptp->bezpointarray_data, BezPoint, i);

	// TODO: better conversion from polyline to bezierline?
	bp->type = BezPoint::BEZ_CURVE_TO;
	bp->p1 = p1;
	bp->p2 = p1;
	// ... and an extra point on every segment center
	bp->p3.x = (p1.x + coords[i*2]) / 2;
	bp->p3.y = (p1.y + coords[i*2+1]) / 2;
	// next control point
	p1.x = coords[i*2];
	p1.y = coords[i*2+1];
      }
      last.type = BezPoint::BEZ_CURVE_TO;
      last.p1 = p1;
      last.p2 = p1;
      g_array_index(ptp->bezpointarray_data, BezPoint, num-1) = last;
    }
  }

  if (prop) {
    GPtrArray *props = prop_list_from_single (prop);
    return object_apply_props (obj, props);
  }

  return NULL;
}


typedef IGraph *(*GraphCreateFunc)();

/*!
 * \brief Calback function invoking layout algorithms from Dia's menu
 * \ingroup LayoutPlugin
 */
static DiaObjectChange *
layout_callback (DiagramData     *data,
                 const char      *filename,
                 guint            flags, /* further additions */
                 void            *user_data,
                 GraphCreateFunc  func)
{
  DiaObjectChange *changes = NULL;
  GList *nodes = NULL, *edges = NULL, *list;
  const char *algorithm = (const char*)user_data;

  /* from the selection create two lists */
  list = data_get_sorted_selected (data);
  while (list) {
    DiaObject *o = (DiaObject *)list->data;
    if (!maybe_edge (o))
      nodes = g_list_append (nodes, o);
    //FIXME: neither 1 nor num_handles-1 is guaranteed to be the second connection
    // it entirely depends on the objects implementation
    else if (   o->num_handles > 1 && o->handles[0]->connected_to
             && (o->handles[1]->connected_to || o->handles[o->num_handles-1]->connected_to))
      edges = g_list_append (edges, o);
    list = g_list_next(list);
  }
  if (g_list_length (edges) < 1 || g_list_length (nodes) < 2) {
    message_warning (_("Please select edges and nodes to layout."));
  } else {
    IGraph *g = func ? func () : NULL;

    if (!g)
      message_error (_("Graph creation failed"));
    else {
      std::vector<double> coords;

      /* transfer nodes and edges */
      for (list = nodes; list != NULL; list = g_list_next(list)) {
        DiaObject *o = (DiaObject *)list->data;
        const DiaRectangle *bbox = dia_object_get_bounding_box (o);
        g->AddNode (bbox->left, bbox->top, bbox->right, bbox->bottom);
      }
      for (list = edges; list != NULL; list = g_list_next(list)) {
        DiaObject *o = (DiaObject *)list->data;
        DiaObject *src = o->handles[0]->connected_to->object;
        // see above: there is no guarantee ...
        DiaObject *dst = o->handles[1]->connected_to ?
          o->handles[1]->connected_to->object : o->handles[o->num_handles-1]->connected_to->object;

        if (_obj_get_bends (o, coords))
          g->AddEdge (g_list_index (nodes, src), g_list_index (nodes, dst), &coords[0], coords.size());
        else
          g->AddEdge (g_list_index (nodes, src), g_list_index (nodes, dst), NULL, 0);
      }

      IGraph::eResult res;
      if ((res = g->Layout (algorithm)) != IGraph::SUCCESS) {
        const char *sErr;
        switch (res) {
          case IGraph::NO_MODULE:
            sErr = _("No such module.");
            break;
          case IGraph::OUT_OF_MEMORY:
            sErr = _("Out of memory.");
            break;
          case IGraph::NO_TREE:
            sErr = _("Not a tree.");
            break;
          case IGraph::NO_FOREST:
            sErr = _("Not a forest.");
            break;
          case IGraph::FAILED_ALGORITHM:
            sErr = _("Failed algorithm.");
            break;
          case IGraph::FAILED_PRECONDITION:
            sErr = _("Failed precondition.");
            break;
          case IGraph::CRASHED:
            sErr = _("OGDF crashed.");
            break;
          default:
            sErr = _("Unknown reason");
            break;
        }
        message_warning (_("Layout '%s' failed.\n%s"),
                         (const char*) user_data,
                         sErr);
      } else {
        changes = dia_object_change_list_new ();

        /* transfer back information */
        int n;
        for (n = 0, list = nodes;
             list != NULL;
             list = g_list_next (list), ++n) {
          Point pt;
          if (g->GetNodePosition (n, &pt.x, &pt.y)) {
            DiaObject *o = DIA_OBJECT (list->data);
            GPtrArray *props = g_ptr_array_new ();

            // FIXME: can't use "obj_pos", it is not read in usual update_data
            // impementations "elem_corner" will only work for Element derived
            // classes, but that covers most of the cases here ...
            prop_list_add_point (props, "elem_corner", &pt);

            dia_object_change_list_add (DIA_OBJECT_CHANGE_LIST (changes),
                                        object_apply_props (o, props));
          }
        }

        // first update to reuse the connected points
        diagram_update_connections_selection (DIA_DIAGRAM (data));

        /* use edge bends, if any */
        int e;
        for (e = 0, list = edges;
             list != NULL;
             list = g_list_next (list), ++e) {
          DiaObject *o = DIA_OBJECT (list->data);
          // number of bends / 2 is the number of points
          int n = g->GetEdgeBends (e, NULL, 0);
          if (n >= 0) { // with 0 it is just a reset of the exisiting line
            try {
              coords.resize (n);
            } catch (std::bad_alloc& ex) {
              g_warning ("%s", ex.what());
              continue;
            }
            g->GetEdgeBends (e, &coords[0], n);
            dia_object_change_list_add (DIA_OBJECT_CHANGE_LIST (changes),
                                        _obj_set_bends (o, coords));
          }
        }
        /* update view */
        diagram_update_connections_selection (DIA_DIAGRAM (data));
      }
      g->Release ();
    }
  }
  g_list_free (nodes);
  g_list_free (edges);

  return changes;
}


static DiaObjectChange *
layout_callback1 (DiagramData *data,
                  const gchar *filename,
                  guint flags, /* further additions */
                  void *user_data)
{
  return layout_callback (data, filename, flags, user_data, dia_graph_create);
}

/* FIXME: Apparently unused?
static ObjectChange *
layout_callback2 (DiagramData *data,
                  const gchar *filename,
                  guint flags, / * further additions * /
                  void *user_data)
{
#ifdef HAVE_OGDF
  return layout_callback (data, filename, flags, user_data, CreateGraph);
#else
  return NULL;
#endif
}
*/

#define AN_ENTRY(group, name, func) \
    { \
      #name "Layout", \
      N_(#name), \
      "/DisplayMenu/Layout/" #group "/" #group "LayoutFirst", \
      layout_callback ##func, \
      (void*)#name \
    }

static DiaCallbackFilter cb_layout[] = {
#ifdef HAVE_OGDF
    AN_ENTRY(Test, NotAvailable, 2),
    AN_ENTRY(Misc, Balloon, 2),
    AN_ENTRY(Misc, Circular, 2),
    AN_ENTRY(Energy-based, DavidsonHarel, 2),
    AN_ENTRY(Upward, Dominance, 2),
    //Borked(crash): AN_ENTRY(Fast, 2),
    AN_ENTRY(Multilevel, FMME, 2),
    AN_ENTRY(Multilevel, FMMM, 2),
    AN_ENTRY(Planar, FPP, 2),
    AN_ENTRY(Energy-based, GEM, 2),
    AN_ENTRY(Multilevel, MixedForce, 2),
    AN_ENTRY(Planar, MixedModel, 2),
    //Borked(crash): AN_ENTRY(Nice, 2),
    //Borked(crash): AN_ENTRY(NoTwist, 2),
    AN_ENTRY(Orthogonal, Planarization, 2),
    AN_ENTRY(Planar, PlanarDraw, 2),
    AN_ENTRY(Planar, PlanarStraight, 2),
    AN_ENTRY(Orthogonal, PlanarizationGrid, 2),
    AN_ENTRY(Tree, RadialTree, 2),
    AN_ENTRY(Planar, Schnyder, 2),
    AN_ENTRY(Energy-based, SpringEmbedderFR, 2),
    AN_ENTRY(Energy-based, SpringEmbedderKK, 2),
    //Borked(huge):
    AN_ENTRY(Energy-based, StressMajorization, 2),
    AN_ENTRY(Upward, Sugiyama, 2),
    AN_ENTRY(Tree, TreeStraight, 2),
    AN_ENTRY(Tree, TreeOrthogonal, 2),
    AN_ENTRY(Upward, UpwardPlanarization, 2),
    AN_ENTRY(Upward, Visibility, 2),
#endif
    AN_ENTRY(Size, Grow, 1),
    AN_ENTRY(Size, Shrink, 1),
    AN_ENTRY(Size, Heighten, 1),
    AN_ENTRY(Size, Widen, 1),
};


#if 0
// Quick 'n Nasty hack to mark strings as translatable
char *hack_a = N_("Grow");
char *hack_b = N_("Shrink");
char *hack_c = N_("Heighten");
char *hack_d = N_("Widen");
#endif

static gboolean
_plugin_can_unload (PluginInfo *info)
{
  /* there is no filter_unregister_callback yet */
  return FALSE;
}

static void
_plugin_unload (PluginInfo *info)
{
  /* todo */
}

/* --- dia plug-in interface --- */
extern "C" {
DIA_PLUGIN_CHECK_INIT
}

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  int i;

  if (!dia_plugin_info_init(info, "Layout",
                            _("OGDF Layout Algorithms"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  // to have it sorted like above we have to start from the end
  for (i = G_N_ELEMENTS (cb_layout) - 1; i >= 0; --i) {
    filter_register_callback (&cb_layout[i]);
  }

  return DIA_PLUGIN_INIT_OK;
}
