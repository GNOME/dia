/* test-objects.c -- Unit test for Dia object implmentations
 * Copyright (C) 2008 Hans Breuer
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
#include <stdio.h>
#include <string.h>
#include <math.h>

#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Dia"

#include <glib.h>
#include <glib-object.h>

#include "object.h"
#include "plug-ins.h"
#include "dialib.h"
#include "create.h"
#include "properties.h"
#include "diapathrenderer.h"

/* allows to select specific objects for testing */
static gchar *_type_name = NULL;
const real EPSILON = 1e-6;

int num_objects = 0;

static void
_test_creation (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  int i;
  Handle *h1 = NULL, *h2 = NULL;
  Point point = {0, 0};
  DiaObject *o = type->ops->create (&point, type->default_user_data, &h1, &h2);

  g_assert (o != NULL && o->type != NULL);
  /* NOT always: o->type == type, e.g. compatibility names for creation */

  /* check mandatory object ops */
  g_assert (   o->ops != NULL
            && o->ops->destroy != NULL
            && o->ops->draw != NULL
            && o->ops->distance_from != NULL
            && o->ops->selectf != NULL
            && o->ops->copy != NULL
            && o->ops->move != NULL
            && o->ops->move_handle != NULL
	    && o->ops->apply_properties_from_dialog != NULL
	    );
  
  /* can we really assume everthing complies with standard props nowadays? */
  g_assert (   o->ops->describe_props
            && o->ops->get_props
	    && o->ops->set_props);
  {
    const PropDescription *pdesc = o->ops->describe_props (o);
    /* get all properties */
    GPtrArray *plist = prop_list_from_descs (pdesc, pdtpp_true);
    
    g_assert (plist != NULL);
    prop_list_free(plist);
  }
  /* not implmeneted anywhere */
  g_assert (o->ops->edit_text == NULL);
  /* I think this is mandatory */
  g_assert (o->ops->apply_properties_list != NULL);

  /* bounding box must be initialized */
  g_assert (o->bounding_box.left <= o->bounding_box.right);
  g_assert (o->bounding_box.top <= o->bounding_box.bottom);
  /* object position must (should?) be in bounding box */
  g_assert (o->bounding_box.left <= o->position.x && o->position.x <= o->bounding_box.right);
  g_assert (o->bounding_box.top <= o->position.y && o->position.y <= o->bounding_box.bottom);

  /* both handles can be NULL, but if not hey must belong to the object  */
  for (i = 0; i < o->num_handles; ++i)
    {
      if (h1 != NULL && h1 == o->handles[i])
        h1 = NULL;
      if (h2 != NULL && h2 == o->handles[i])
        h2 = NULL;
      /* handles properly set up? */
      g_assert (o->handles[i] != NULL);
      g_assert (o->handles[i]->connected_to == NULL); /* starts not connected */
      g_assert (   o->handles[i]->type != HANDLE_NON_MOVABLE 
	        || (   o->handles[i]->type == HANDLE_NON_MOVABLE /* always together? */
		    && o->handles[i]->connect_type == HANDLE_NONCONNECTABLE));
    }
  /* handles now destroyed */
  g_assert (NULL == h1 && NULL == h2);

  for (i = 0; i < o->num_connections; ++i)
    {
      g_assert (o->connections[i] != NULL);
    }

  /* finally */
  o->ops->destroy (o);
  g_free (o);
}

static void
_test_copy (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *oc, *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  Rectangle bbox1, bbox2;
  Point to = {0, 0};
  int i;

  g_assert (o != NULL && o->type != NULL);

  /* does the object move ... ? */
  from = o->position;
  bbox1 = o->bounding_box;
  oc = o->ops->copy (o);

  g_assert (oc != NULL && oc->type == o->type && oc->ops == o->ops);

  to = o->position;
  bbox2 = o->bounding_box;

  /* ... it should not */
  g_assert (fabs(from.x - to.x) < EPSILON && fabs(from.y - to.y) < EPSILON);
  g_assert (   fabs((bbox2.right - bbox2.left) - (bbox1.right - bbox1.left)) < EPSILON
            && fabs((bbox2.bottom - bbox2.top) - (bbox1.bottom - bbox1.top)) < EPSILON);

  /* is copying producing dangling pointers ? */
  g_assert (o->num_handles == oc->num_handles);
  for (i = 0; i < o->num_handles; ++i)
    {
      g_assert (oc->handles[i] != NULL && oc->handles[i] != o->handles[i]);
    }

  g_assert (o->num_connections == oc->num_connections);
  for (i = 0; i < o->num_connections; ++i)
    {
      g_assert (oc->connections[i]->object == oc); /* owner set? */
      g_assert (oc->connections[i] != NULL && oc->connections[i] != o->connections[i]);
    }

  /* check some further properties which must be copied ?*/

  /* finally */
  o->ops->destroy (o);
  g_free (o);
  oc->ops->destroy (oc);
  g_free (oc);
}

/* small helper to just throw it away */
static void
_object_change_free(ObjectChange *change)
{
  if (change) { /* usually this is NULL for move */
    if (change->free) {
      change->free(change);
      g_free(change);
    }
  }
}

static void
_test_movement (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {5, 5};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  Rectangle bbox1, bbox2;
  Point to = {10, 10};
  ObjectChange *change;
  Point pos;
  real epsilon;
  Point *handle_positions;
  Point *cp_positions;
  guint i;
  
  /* does the object move ... ? */
  from = o->position;
  bbox1 = o->bounding_box;
  /* ... not (= hack used to force an update call) */
  change = o->ops->move (o, &o->position);
  if (change) /* usually this is NULL for move */
    _object_change_free(change);
  bbox2 = o->bounding_box;
  if (   strcmp (type->name, "Cybernetics - b-sens") == 0
      || strcmp (type->name, "Cybernetics - l-sens") == 0
      || strcmp (type->name, "Cybernetics - r-sens") == 0
      || strcmp (type->name, "Cybernetics - t-sens") == 0
      ) /* not nice, but also not a reason to fail */
    epsilon = 0.5 + EPSILON;
  else
    epsilon = EPSILON;

  g_assert (   fabs(fabs(bbox2.right - bbox2.left) - fabs(bbox1.right - bbox1.left)) < epsilon
            && fabs(fabs(bbox2.bottom - bbox2.top) - fabs(bbox1.bottom - bbox1.top)) < epsilon);
  /* .... really: without changing size ? */
  pos = o->position;
  bbox1 = o->bounding_box;
  /* remember handle and connection point positions ... */
  handle_positions = g_alloca (sizeof(Point) * o->num_handles);
  for (i = 0; i < o->num_handles; ++i)
    handle_positions[i] = o->handles[i]->pos;
  cp_positions = g_alloca (sizeof(Point) * o->num_connections);
  for (i = 0; i < o->num_connections; ++i)
    cp_positions[i] = o->connections[i]->pos;

  change = o->ops->move (o, &to);
  if (change) /* usually this is NULL for move */
    _object_change_free(change);
  /* does the position reflect the move? */
  g_assert (   fabs(fabs(pos.x - o->position.x) - fabs(from.x - to.x)) < EPSILON
            && fabs(fabs(pos.y - o->position.y) - fabs(from.y - to.y)) < EPSILON );
  /* ... also for handles and connection points? */
  for (i = 0; i < o->num_handles; ++i)
    g_assert (   fabs(fabs(handle_positions[i].x - o->handles[i]->pos.x) - fabs(from.x - to.x)) < EPSILON
              && fabs(fabs(handle_positions[i].y - o->handles[i]->pos.y) - fabs(from.y - to.y)) < EPSILON);
  for (i = 0; i < o->num_connections; ++i)
    g_assert (   fabs(fabs(cp_positions[i].x - o->connections[i]->pos.x) - fabs(from.x - to.x)) < EPSILON
              && fabs(fabs(cp_positions[i].y - o->connections[i]->pos.y) - fabs(from.y - to.y)) < EPSILON);

  bbox2 = o->bounding_box;
  /* test fails e.g. for 'Cisco - Web cluster' probably due to bezier-bbox-issues: bug 568115 */
  if (   strcmp (type->name, "Cisco - IP Softphone") == 0 /* ok on win32 */
      || strcmp (type->name, "Cisco - Router in building") == 0 /* height off 0.5 */
      || strcmp (type->name, "Cisco - MCU") == 0 /* height off 0.8 */
      || strcmp (type->name, "Cisco - Mac Woman") == 0 /* ok on win32 */
      /* more failing recently? */
      || strcmp (type->name, "Cisco - SVX (interchangeable with End office)") == 0 /* height off 0.32 */
      /* ... changed move condition (starting point)  */
      || strcmp (type->name, "Cisco - ATM Tag Switch Router") == 0 /* height off 0.7 */
      /* FIXME: this shape should be simple enough to actually fix the bug */
      || strcmp (type->name, "Assorted - Heart") == 0 /* height off 0.05 */
      || strstr (type->name, "Bugs -") == type->name
     )
    g_test_message ("SKIPPED! ");
  else
    g_assert (   fabs((bbox2.right - bbox2.left) - (bbox1.right - bbox1.left)) < EPSILON
              && fabs((bbox2.bottom - bbox2.top) - (bbox1.bottom - bbox1.top)) < EPSILON);

  /* finally */
  o->ops->destroy (o);
  g_free (o);
}

static void
_test_change (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  ObjectChange *change;
  const PropDescription *descs;
  GPtrArray *props;

  if (o->ops->apply_properties_list) {
    /* get description */
    descs = o->ops->describe_props (o);
    /* get unset value vector */ 
    props = prop_list_from_descs (descs, pdtpp_is_visible);
    /* fill it with this objects values */
    o->ops->get_props (o, props);
    /* apply it back to the object - maybe we should do some change first? */
    change = o->ops->apply_properties_list (o, props);
    prop_list_free (props);
    if (change) {
      /* maybe we should do something interesting first? */
      _object_change_free(change);
    } else {
      g_print ("'%s' - no undo?\n", o->type->name);
    }
  }
  /* finally */
  o->ops->destroy (o);
  g_free (o);
}
static void
_test_move_handle (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  DiaObject *o2 = NULL;
  ObjectChange *change;
  ConnectionPoint *cp = NULL;
  gint i;
  
  if (h2) /* not mandatory to return one */
    {
      Point to = h2->pos;
      to.x += 1.0; to.y += 1.0;
      change = o->ops->move_handle(o, h2, &to, NULL, HANDLE_MOVE_CREATE_FINAL, 0);
      /* the API would allow, but it gave at least a leak at app/create_object.c */
      if (change)
        {
          g_test_message ("CHANGE ");
          _object_change_free(change);
        }
      h2 = NULL;
    }
  /* find a good handle to move */
  for (i = 0; i < o->num_handles; ++i)
    {
      
      if (o->handles[i]->type == HANDLE_MAJOR_CONTROL)
        {
          h2 = o->handles[i];
	  if (h2->connect_type == HANDLE_CONNECTABLE)
	    {
	      o2 = create_standard_box (5.0, 5.0, 2.0, 2.0);
	      if (!o2) {
		/* this may happen if Standard objects are not included */
		g_test_message ("SKIPPED connect test.");
		break;
	      }
	      g_assert(o2->num_connections > 0);
	      cp = o2->connections[0];
	      object_connect(o, h2, cp);
	    }
	  break;
	}
    }
  /* second move */
  if (h2)
    {
      Point to = h2->pos;
      to.x += 1.0; to.y += 1.0;
      if (cp)
        {
          change = o->ops->move_handle(o, h2, &to, cp, HANDLE_MOVE_CONNECTED, 0);
          /* again the API would allow, but it gave at least a leak at app/connectionpoint_ops.c */
          g_assert (change == NULL);
	}
      else
        {
          change = o->ops->move_handle(o, h2, &to, NULL, HANDLE_MOVE_USER_FINAL, 0);
	  if (change) /* still not mandatory */
	    {
	      to.x -= 1.0; to.y -= 1.0;
	      change->revert(change, NULL);
	      _object_change_free(change);
	      if (TRUE) /* move_handle undo is handled on the application level ;( */
                 /* NOP */;
	      else
	        g_assert(   fabs(to.x - o->handles[i]->pos.x) < EPSILON
                         && fabs(to.y - o->handles[i]->pos.y) < EPSILON);
	    }
	}
      h2 = NULL;
    }
  /* finally */
  o->ops->destroy (o);
  g_free (o);
  if (o2) {
    o2->ops->destroy (o2);
    g_free (o2);
  }
}
static void
_test_connectionpoint_consistency (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point pos = {0, 0};
  Point center;
  DiaObject *o = type->ops->create (&pos, type->default_user_data, &h1, &h2);
  ObjectChange *change;
  int i;
  gboolean any_dir_set = FALSE;

  change = dia_object_set_string (o, NULL, "Test me!");
  _object_change_free (change);

  for (i = 0; i < o->num_connections; ++i) {
    ConnectionPoint *cp = o->connections[i];
    if (cp->directions != DIR_NONE)
      any_dir_set = TRUE;
  }
  if (!any_dir_set) {
    /* should be connection */
    int start_end = 0; /* counter */
    for (i = 0; i < o->num_handles; ++i) {
      Handle *h = o->handles[i];
      if (h->id == HANDLE_MOVE_STARTPOINT || h->id == HANDLE_MOVE_ENDPOINT)
        ++start_end;
    }
    if (start_end < 2 && o->num_connections > 0)
      g_print ("'%s' with no directions\n", type->name);
    return;
  }

  pos = o->position;
  center.x = (o->bounding_box.right + o->bounding_box.left) / 2;
  center.y = (o->bounding_box.bottom + o->bounding_box.top) / 2;
  for (i = 0; i < o->num_connections; ++i) {
    ConnectionPoint *cp = o->connections[i];
    if (cp->directions == DIR_ALL) {
      /* Use this as misplaced mainpoint check ...
       * - some shapes should never have got a main point, but removing these
       *   now would produce complaints if they were used in diagrams
       * - the main point should be within the object, which can't work for
       *   an object consisting of only lines ...
       * - some main points where just placed automatically and got fixed
       *   after idntified with this check
       */
      if (   strcmp (type->name, "chemeng - coil") == 0
          || strcmp (type->name, "chemeng - coilv") == 0
          || strcmp (type->name, "chemeng - doublepipe") == 0
          || strcmp (type->name, "chemeng - pneum") == 0
          || strcmp (type->name, "chemeng - pneumv") == 0
          || strcmp (type->name, "chemeng - pnuemv") == 0
          || strcmp (type->name, "chemeng - spray") == 0
          || strcmp (type->name, "Circuit - Horizontal Powersource (European)") == 0
          || strcmp (type->name, "Circuit - NMOS Transistor (European)") == 0
          || strcmp (type->name, "Circuit - NPN Transistor") == 0
          || strcmp (type->name, "Circuit - PMOS Transistor (European)") == 0
          || strcmp (type->name, "Circuit - PNP Transistor") == 0
          || strcmp (type->name, "Circuit - Vertical Capacitor") == 0
          || strcmp (type->name, "Circuit - Vertical Powersource (European)") == 0
          || strcmp (type->name, "Civil - Preliminary Clarification Tank") == 0
          || strcmp (type->name, "Civil - Reference Line") == 0
          || strcmp (type->name, "Civil - Aerator") == 0
          || strcmp (type->name, "Civil - Basin") == 0
          || strcmp (type->name, "Civil - Final-Settling Basin") == 0
          || strcmp (type->name, "Small Extension Node") == 0 /* MSE */
         )
        g_print ("main-cp misplaced!");
      else
        g_assert (o->ops->distance_from (o, &cp->pos) == 0 && "within");
      continue;
    }
    if (   strcmp (type->name, "chronogram - reference") == 0
        || strcmp (type->name, "BPMN - Data-Object") == 0
        || strcmp (type->name, "Optics - Scope") == 0
        || strcmp (type->name, "Optics - Spectrum") == 0
        || strcmp (type->name, "GRAFCET - Transition") == 0
        || strcmp (type->name, "Standard - Polygon") == 0
        || strcmp (type->name, "GRAFCET - Action") == 0)
      continue; /* undecided */
    /* Some things which should not be set */
    if (cp->pos.x > center.x)
      g_assert ((cp->directions & DIR_WEST) == 0);
    else if (cp->pos.x < center.x)
      g_assert ((cp->directions & DIR_EAST) == 0);
    if (cp->pos.y > center.y)
      g_assert ((cp->directions & DIR_NORTH) == 0);
    else if (cp->pos.y < center.y)
      g_assert ((cp->directions & DIR_SOUTH) == 0);
  }
  /* every connection point should be in bounds of the object */
  for (i = 0; i < o->num_connections; ++i) {
    ConnectionPoint *cp = o->connections[i];
#if 1
    if (   strcmp (type->name, "Racks - Label Anchors 42U") == 0
        || strcmp (type->name, "Civil - Horizontal Rest") == 0
        || strcmp (type->name, "Cisco - Data Center Switch") == 0
        || strcmp (type->name, "Civil - Bivalent Vertical Rest") == 0
        || strcmp (type->name, "Cisco - VIP") == 0
        || strcmp (type->name, "Building Site - Proportioning Batcher") == 0
        || strcmp (type->name, "Civil - Gas Bottle") == 0
        || strcmp (type->name, "Civil - Water Level") == 0
        || strcmp (type->name, "UML - Classicon") == 0
        || strcmp (type->name, "scene graph - field") == 0
        || strcmp (type->name, "Cisco - Telecommuter") == 0
        || strcmp (type->name, "Civil - Vertical Rest") == 0
        || strcmp (type->name, "Cisco - PC Adapter Card") == 0
        || strcmp (type->name, "Civil - Reference Line") == 0
        || strcmp (type->name, "Cisco - Dot-Dot") == 0
        || strcmp (type->name, "Cisco - WLAN controller") == 0
        || strstr (type->name, "Bugs -") == type->name)
      break; /* kind of wasteful to check for every connection */
    g_assert (   o->ops->distance_from (o, &cp->pos) < 0.01
              || distance_rectangle_point (&o->bounding_box, &cp->pos) < 0.01);
#else
    /* generate exception list - after all it is legal to have CPs out of bounds */
    if (   o->ops->distance_from (o, &cp->pos) >= 0.01
        && distance_rectangle_point (&o->bounding_box, &cp->pos) >= 0.01) {
      g_print ("        || strcmp (type->name, \"%s\") == 0\n", type->name);
      break;
    }
#endif
  }
  /* finally */
  o->ops->destroy (o);  
  g_free (o);
}
static void
_test_object_menu (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  int i;

  /* the method itself is optional */
  if (!o->ops->get_object_menu) {
    g_test_message ("SKIPPED (n.i.)!");
  } else {
    DiaMenu *menu = (o->ops->get_object_menu)(o, &from); /* clicked_pos should not matter much */
    /* strangley enough I found a crash with menu==NULL today ;) */
    int num_items = menu ? menu->num_items : 0;

    for (i = 0; i < num_items; ++i) {
      DiaMenuItem *item;

      /* Every call below might update availability of the active items */
      menu = (o->ops->get_object_menu)(o, &from);
      item = &menu->items[i];

      if (item->text) /* the second form is a submenu, see: "FS - Function" */
        g_assert (item->callback != NULL || ((DiaMenu*)item->callback_data)->num_items > 0);
      else
        g_assert (item->callback == NULL && "Separator with callback?");

      g_assert ((item->active & ~(DIAMENU_ACTIVE|DIAMENU_TOGGLE|DIAMENU_TOGGLE_ON)) == 0);

      /* if we have a callback active, call it */
      if (item->callback && (item->active & DIAMENU_ACTIVE))
      {
	ObjectChange *change;

	/* g_test_message() does not show normally */
	g_test_message ("\n\tCalling '%s'...", item->text);
	change = (item->callback)(o, &from, item->callback_data);
	if (!change) {
	  g_test_message ("Undo/redo missing: %s\n", item->text);
	} else {
	  /* Don't just call _object_change_free(change);
	   * For 'Convert to *' this will screw up (destroy) the object 
	   * at hand'. So revert first, afterwards destroy the change.
	   * The object parameter is deprecated, but still necessary!
	   */
	  (change->revert)(change, o);
	  _object_change_free(change);
	}
      }
    }
  }
  /* finally */
  o->ops->destroy (o);  
  g_free (o);
}

static void
_test_draw (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);

  DiaRenderer *renderer = g_object_new (DIA_TYPE_PATH_RENDERER, 0);

  o->ops->draw (o, renderer);
  /* finally */
  o->ops->destroy (o);  
  g_free (o);
  g_object_unref (renderer);
}

/*
 * A dictionary interface to all registered object(-types)
 */
static void
_ot_item (gpointer key,
          gpointer value,
          gpointer user_data)
{
  gchar *name = (gchar *)key;
  DiaObjectType *type = (DiaObjectType *)value;
  const gchar *base = (const gchar *)user_data;
  gchar *testpath;

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Creation");
  g_test_add_data_func (testpath, type, _test_creation);
  g_free (testpath);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Copy");
  g_test_add_data_func (testpath, type, _test_copy);
  g_free (testpath);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Movement");
  g_test_add_data_func (testpath, type, _test_movement);
  g_free (testpath);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Change");
  g_test_add_data_func (testpath, type, _test_change);
  g_free (testpath);
  
  testpath = g_strdup_printf ("%s/%s/%s", base, name, "MoveHandle");
  g_test_add_data_func (testpath, type, _test_move_handle);
  g_free (testpath);

  
  testpath = g_strdup_printf ("%s/%s/%s", base, name, "ConnectionPoints");
  g_test_add_data_func (testpath, type, _test_connectionpoint_consistency);
  g_free (testpath);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "ObjectMenu");
  g_test_add_data_func (testpath, type, _test_object_menu);
  g_free (testpath);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Draw");
  g_test_add_data_func (testpath, type, _test_draw);
  g_free (testpath);

  ++num_objects;
}

int
main (int argc, char** argv)
{
  GList *plugins = NULL;
  int ret = 0;
  
  /* not using gtk_test_init() means we can only test non-gtk facilities of objects */
  g_type_init ();
  g_test_init (&argc, &argv, NULL);
  
  libdia_init (DIA_MESSAGE_STDERR);
  
  /* todo: improve command line parsing */
  if (argc > 1)
    {
      const gchar* path = argv[1];
      
      if (g_file_test (path, G_FILE_TEST_IS_DIR))
        dia_register_plugins_in_dir (path);
      else
        dia_register_plugin (path);
    }
  else
    {
      /* avoid loading objects/plug-ins form the users home directory */
      g_setenv ("HOME", "/tmp", TRUE);
      dia_register_plugins ();
    }
  plugins = dia_list_plugins ();
  g_assert (g_list_length (plugins) > 0);

  object_registry_foreach (_ot_item, "/Dia/Objects");

  ret = g_test_run ();
  g_print ("%d objects.\n", num_objects);

  return ret;
}

#ifdef _MSC_VER
int _matherr( struct _exception *except )
{
  const char *type;

#define CASE(x) case _ ##x : type=#x; break
  switch (except->type) {
  CASE(DOMAIN); 
  CASE(SING);
  CASE(UNDERFLOW);
  CASE(OVERFLOW);
  CASE(TLOSS);
  CASE(PLOSS);
  }
#undef CASE
  g_warning ("%s %s(%f, %f)", type, except->name, except->arg1, except->arg2);
  return 0;
}
#endif
