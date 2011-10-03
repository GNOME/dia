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
#include <config.h>

#include "intl.h"
#include "message.h"
#include "filter.h"
#include "plug-ins.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"

#include "diagram.h"
#include "connectionpoint_ops.h"

#include "ogdf-simple.h"

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
static ObjectChange *
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

static ObjectChange *
layout_callback (DiagramData *data,
                 const gchar *filename,
                 guint flags, /* further additions */
                 void *user_data)
{
  ObjectChange *changes = NULL;
  GList *nodes = NULL, *edges = NULL, *list;

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
#ifdef HAVE_OGDF
    IGraph *g = CreateGraph ();
#else
    IGraph *g = NULL;
#endif
    if (!g)
      message_error (_("Graph creation failed"));
    else {
      std::vector<double> coords;

      /* transfer nodes and edges */
      for (list = nodes; list != NULL; list = g_list_next(list)) {
        DiaObject *o = (DiaObject *)list->data;
        const Rectangle *bbox = dia_object_get_bounding_box (o);
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
      if ((res = g->Layout ((const char*)user_data)) != IGraph::SUCCESS) {
	const char *sErr;
	switch (res) {
	case IGraph::NO_MODULE : sErr = _("No such module."); break;
	case IGraph::OUT_OF_MEMORY : sErr = _("Out of memory."); break;
	case IGraph::NO_TREE: sErr = _("Not a tree."); break;
	case IGraph::NO_FOREST: sErr = _("Not a forest."); break;
	case IGraph::FAILED_ALGORITHM: sErr = _("Failed algorithm."); break;
	case IGraph::FAILED_PRECONDITION: sErr = _("Failed precondition."); break;
	case IGraph::CRASHED : sErr = _("OGDF crashed."); break;
	default : sErr = _("Unknown reason"); break;
	}
        message_warning (_("Layout '%s' failed.\n%s"), (const char*)user_data, sErr);
      } else {
        changes = change_list_create ();
	/* transfer back information */
	int n;
	for (n = 0, list = nodes; list != NULL; list = g_list_next (list), ++n) {
	  Point pt;
	  if (g->GetNodePosition (n, &pt.x, &pt.y)) {
	    DiaObject *o = (DiaObject *)list->data;
	    GPtrArray *props = g_ptr_array_new ();
	    
	    //FIXME: can't use "obj_pos", it is not read in usual update_data impementations
	    // "elem_corner" will only work for Element derived classes, but that covers most
	    // of the cases here ...
	    prop_list_add_point (props, "elem_corner", &pt);
	    change_list_add (changes, object_apply_props (o, props));
	  }
	}
	// first update to reuse the connected points
	diagram_update_connections_selection(DIA_DIAGRAM (data));
	/* use edge bends, if any */
	int e;
	for (e = 0, list = edges; list != NULL; list = g_list_next (list), ++e) {
          DiaObject *o = (DiaObject *)list->data;
	  // number of bends / 2 is the number of points
	  int n = g->GetEdgeBends (e, NULL, 0);
	  if (n >= 0) { // with 0 it is just a reset of the exisiting line
	    try {
	      coords.resize (n);
	    } catch (std::bad_alloc& ex) {
	      g_warning (ex.what());
	      continue;
	    }
	    g->GetEdgeBends (e, &coords[0], n);
	    change_list_add (changes, _obj_set_bends (o, coords));
	  }
	}
	/* update view */
	diagram_update_connections_selection(DIA_DIAGRAM (data));
      }
      g->Release ();
    }
  }
  g_list_free (nodes);
  g_list_free (edges);

  return changes;
}

#define AN_ENTRY(name) \
    { \
      #name "Layout", \
      N_(#name), \
      "/DisplayMenu/Layout/LayoutFirst", \
      layout_callback, \
      #name \
    }
      
static DiaCallbackFilter cb_layout[] = {
    AN_ENTRY(Balloon),
    AN_ENTRY(Circular),
    AN_ENTRY(DavidsonHarel),
    AN_ENTRY(Dominance),
    //Borked: AN_ENTRY(Fast),
    AN_ENTRY(FMME),
    AN_ENTRY(FMMM),
    AN_ENTRY(GEM),
    AN_ENTRY(MixedForce),
    AN_ENTRY(MixedModel),
    //Borked: AN_ENTRY(Nice),
    //Borked: AN_ENTRY(NoTwist),
    AN_ENTRY(Planarization),
    AN_ENTRY(PlanarDraw),
    AN_ENTRY(PlanarStraight),
    AN_ENTRY(PlanarizationGrid),
    AN_ENTRY(RadialTree),
    AN_ENTRY(SpringEmbedderFR),
    AN_ENTRY(SpringEmbedderKK),
    //Borked: AN_ENTRY(StressMajorization),
    AN_ENTRY(Sugiyama),
    AN_ENTRY(Tree),
    AN_ENTRY(UpwardPlanarization),
    //Borked: AN_ENTRY(Visibility),
    AN_ENTRY(NotAvailable),
    NULL
};

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
  int i = 0;

  if (!dia_plugin_info_init(info, "OGDF Layout",
                            _("Layout Algorithms"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  while (cb_layout[i].action) {
#ifdef HAVE_OGDF
    // no point in adding these without OGDF yet 
    filter_register_callback (&cb_layout[i]);
#endif
    ++i;
  }

  return DIA_PLUGIN_INIT_OK;
}
