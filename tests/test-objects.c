/* test-objects.c -- Unit test for Dia object implementations
 * Copyright (C) 2008-2014 Hans Breuer
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

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <glib.h>
#include <glib-object.h>

#include "object.h"
#include "plug-ins.h"
#include "dialib.h"
#include "create.h"
#include "properties.h"
#include "diapathrenderer.h"

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

  /* can we really assume everything complies with standard props nowadays? */
  g_assert (   o->ops->describe_props
            && o->ops->get_props
	    && o->ops->set_props);
  {
    const PropDescription *pdesc = o->ops->describe_props (o);
    /* get all properties */
    GPtrArray *props = prop_list_from_descs (pdesc, pdtpp_true);
    int num_described = props->len;
    int num_used = 0;

    /* Indirect check of object's private PropDescription and PropOffset array.
     * Both arrays should be same length (reference the same properties).
     * But the latter is only visible as parameter to object_get_props_from_offsets(),
     * at least for objects not intialzing DiaObjectType::prop_offsets
     */
    o->ops->get_props (o, props);
    for (i = 0; i < num_described; ++i) {
      Property *prop = (Property*)g_ptr_array_index(props,i);
      if ((prop->experience & PXP_NOTSET) == 0)
        ++num_used;
      else if ((prop->descr->flags & PROP_FLAG_WIDGET_ONLY) != 0)
	++num_used; /* ... but not expected to be set */
      else if (strcmp (prop->descr->type, PROP_TYPE_STATIC) == 0)
	++num_used; /* also not to be set */
      else {
        g_printerr ("Not set '%s'\n", prop->descr->name);
      }
    }
    g_assert_cmpint (num_used, ==, num_described);

    g_assert (props != NULL);
    prop_list_free(props);
  }
  /* not implemented anywhere */
  g_assert (o->ops->edit_text == NULL);
  /* I think this is mandatory */
  g_assert (o->ops->apply_properties_list != NULL);

  /* bounding box must be initialized */
  g_assert (o->bounding_box.left <= o->bounding_box.right);
  g_assert (o->bounding_box.top <= o->bounding_box.bottom);
  /* object position must (should?) be in bounding box */
  g_assert (o->bounding_box.left <= o->position.x && o->position.x <= o->bounding_box.right);
  g_assert (o->bounding_box.top <= o->position.y && o->position.y <= o->bounding_box.bottom);

  g_assert (o->num_handles > 0);
  /* both handles can be NULL, but if not they must belong to the object  */
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
  /* handles now found */
  g_assert (NULL == h1 && NULL == h2);

  for (i = 0; i < o->num_connections; ++i)
    {
      g_assert (o->connections[i] != NULL);
      g_assert (o->connections[i]->object == o); /* owner set? */
      g_assert ((o->connections[i]->directions & ~DIR_ALL) == 0); /* known directions */
      g_assert ((o->connections[i]->flags & ~CP_FLAGS_MAIN) == 0); /* known flags */
    }

  /* finally */
  o->ops->destroy (o);
  g_clear_pointer (&o, g_free);
}

static void
_test_copy (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *oc, *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  DiaRectangle bbox1, bbox2;
  Point to;
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

  /* check some further properties which must be copied ? */

  /* finally */
  o->ops->destroy (o);
  g_clear_pointer (&o, g_free);
  oc->ops->destroy (oc);
  g_clear_pointer (&oc, g_free);
}


static void
_test_movement (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {5, 5};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  DiaRectangle bbox1, bbox2;
  Point to = {10, 10};
  DiaObjectChange *change;
  Point pos;
  real epsilon;
  Point *handle_positions;
  Point *cp_positions;
  guint i;
  /* With the use here the number of connections can not change.
   * Declaring and using it const avoids misinterpretation of scan-build
   * about the first and second loop over it being different.
   */
  const int num_connections = o->num_connections;
  /* same for the number of handles */
  const int num_handles = o->num_handles;

  /* does the object move ... ? */
  from = o->position;
  bbox1 = o->bounding_box;
  /* ... not (= hack used to force an update call) */
  change = dia_object_move (o, &o->position);
  g_clear_pointer (&change, dia_object_change_unref);
  g_assert (num_connections == o->num_connections);
  bbox2 = o->bounding_box;
  if (   strcmp (type->name, "Cybernetics - b-sens") == 0
      || strcmp (type->name, "Cybernetics - l-sens") == 0
      || strcmp (type->name, "Cybernetics - r-sens") == 0
      || strcmp (type->name, "Cybernetics - t-sens") == 0
      ) /* not nice, but also not a reason to fail */
    epsilon = 0.5 + EPSILON;
  else
    epsilon = EPSILON;

  g_assert_cmpfloat (fabs(fabs(bbox2.right - bbox2.left) - fabs(bbox1.right - bbox1.left)), <, epsilon);
  g_assert_cmpfloat (fabs(fabs(bbox2.bottom - bbox2.top) - fabs(bbox1.bottom - bbox1.top)), <, epsilon);
  /* .... really: without changing size ? */
  pos = o->position;
  bbox1 = o->bounding_box;
  /* remember handle and connection point positions ... */
  handle_positions = g_alloca (sizeof(Point) * num_handles);
  /* at least one handle is mandatory */
  g_assert (num_handles > 0);
  for (i = 0; i < num_handles; ++i)
    handle_positions[i] = o->handles[i]->pos;
  cp_positions = g_alloca (sizeof(Point) * o->num_connections);
  for (i = 0; i < num_connections; ++i)
    cp_positions[i] = o->connections[i]->pos;

  change = dia_object_move (o, &to);
  g_clear_pointer (&change, dia_object_change_unref);
  g_assert (num_connections == o->num_connections);
  /* does the position reflect the move? */
  g_assert_cmpfloat (fabs(fabs(pos.x - o->position.x) - fabs(from.x - to.x)), <, EPSILON);
  g_assert_cmpfloat (fabs(fabs(pos.y - o->position.y) - fabs(from.y - to.y)), <, EPSILON);
  /* ... also for handles and connection points? */
  for (i = 0; i < num_handles; ++i)
    {
      g_assert_cmpfloat (fabs(fabs(handle_positions[i].x - o->handles[i]->pos.x) - fabs(from.x - to.x)), <, EPSILON);
      g_assert_cmpfloat (fabs(fabs(handle_positions[i].y - o->handles[i]->pos.y) - fabs(from.y - to.y)), <, EPSILON);
    }
  for (i = 0; i < num_connections; ++i)
    {
      g_assert_cmpfloat (fabs(fabs(cp_positions[i].x - o->connections[i]->pos.x) - fabs(from.x - to.x)), <, EPSILON);
      g_assert_cmpfloat (fabs(fabs(cp_positions[i].y - o->connections[i]->pos.y) - fabs(from.y - to.y)), <, EPSILON);
    }

  bbox2 = o->bounding_box;
  /* test fails e.g. for 'Cisco - Web cluster' probably due to bezier-bbox-issues: bug 568115 */
  if (/* FIXME: this shape should be simple enough to actually fix the bug */
         strcmp (type->name, "Assorted - Heart") == 0 /* height off 0.05 */
      || strstr (type->name, "Bugs -") == type->name)
    g_test_message ("SKIPPED %s! ", type->name);
  else
    {
      g_assert_cmpfloat (fabs((bbox2.right - bbox2.left) - fabs(bbox1.right - bbox1.left)), <, EPSILON);
      g_assert_cmpfloat (fabs((bbox2.bottom - bbox2.top) - fabs(bbox1.bottom - bbox1.top)), <, EPSILON);
    }
  /* finally */
  o->ops->destroy (o);
  g_clear_pointer (&o, g_free);
}

static void
_test_change (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);

  if (o->ops->apply_properties_list) {
    DiaObjectChange *change;
    /* get description */
    const PropDescription *descs = o->ops->describe_props (o);
    /* get unset value vector */
    GPtrArray *props = prop_list_from_descs (descs, pdtpp_is_visible);
    /* fill it with this objects values */
    dia_object_get_properties (o, props);
    /* apply it back to the object - maybe we should do some change first? */
    change = dia_object_apply_properties (o, props);
    prop_list_free (props);
    if (change) {
      /* maybe we should do something interesting first? */
      g_clear_pointer (&change, dia_object_change_unref);
    } else {
      g_printerr ("'%s' - no undo?\n", o->type->name);
    }
  }
  /* finally */
  o->ops->destroy (o);
  g_clear_pointer (&o, g_free);
}
static void
_test_move_handle (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  DiaObject *o2 = NULL;
  DiaObjectChange *change;
  ConnectionPoint *cp = NULL;
  gint i;

  if (h1)
    {
      /* The first handle is used to connect - if possible.
       * Only very few object return in unconnectable handle, namely:
       *   "Standard - Beziergon", "Standard - Path", "Standard - Polygon",
       *   "Database - Compound",
       *   "GRAFCET - Condition",
       *   "Misc - Ngon",
       *   "Network - Radio Cell"
       * This is not a bug in these object implementations, because other
       * uses are possible ...
       */
      if (h1->connect_type != HANDLE_CONNECTABLE)
        g_test_message ("Handle 1 not connectable");

    }

  if (h2) /* not mandatory to return one */
    {
      Point to = h2->pos;
      to.x += 1.0; to.y += 1.0;
      change = dia_object_move_handle (o, h2, &to, NULL, HANDLE_MOVE_CREATE_FINAL, 0);
      /* the API would allow, but it gave at least a leak at app/create_object.c */
      if (change) {
        g_test_message ("CHANGE ");
        g_clear_pointer (&change, dia_object_change_unref);
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
  if (h2) {
    Point to = h2->pos;
    DiaRectangle bb_org = o->bounding_box;
    from = to;
    to.x += 1.0; to.y += 1.0;
    if (cp) {
      change = dia_object_move_handle (o, h2, &to, cp, HANDLE_MOVE_CONNECTED, 0);
      /* again the API would allow, but it gave at least a leak at app/connectionpoint_ops.c */
      g_assert (change == NULL);
    } else {
      change = dia_object_move_handle (o, h2, &to, NULL, HANDLE_MOVE_USER_FINAL, 0);
      if (change) /* still not mandatory */ {
        to.x -= 1.0; to.y -= 1.0;
        dia_object_change_revert(change, NULL);
        g_clear_pointer (&change, dia_object_change_unref);
        if (TRUE) {
          /* move_handle undo is handled on the application level ;( */
          /* NOP */;
        } else {
          g_assert(   fabs (to.x - o->handles[i]->pos.x) < EPSILON
                   && fabs (to.y - o->handles[i]->pos.y) < EPSILON);
        }
      }
    }
    /* moving back to the original position must restore the original object */
    change = dia_object_move_handle (o, h2, &from, NULL, HANDLE_MOVE_USER_FINAL, 0);
    /* custom_move_handle() might return a change */
    g_clear_pointer (&change, dia_object_change_unref);
    if (   strcmp (type->name, "UML - Lifeline") == 0
        || strcmp (type->name, "Standard - Outline") == 0) {
      g_test_message ("No restore by '%s'::move_handle", type->name);
    } else {
      g_assert_cmpfloat (fabs(o->bounding_box.top - bb_org.top), <, EPSILON);
      g_assert_cmpfloat (fabs(o->bounding_box.left - bb_org.left), <, EPSILON);
      g_assert_cmpfloat (fabs(o->bounding_box.right - bb_org.right), <, EPSILON);
      g_assert_cmpfloat (fabs(o->bounding_box.bottom - bb_org.bottom), <, EPSILON);
    }
    h2 = NULL;
  }
  /* finally */
  o->ops->destroy (o);
  g_clear_pointer (&o, g_free);
  if (o2) {
    o2->ops->destroy (o2);
    g_clear_pointer (&o2, g_free);
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
  DiaObjectChange *change;
  int i;
  gboolean any_dir_set = FALSE;
  ConnectionPoint *cp_prev = NULL;

  change = dia_object_set_string (o, NULL, "Test me!");
  g_clear_pointer (&change, dia_object_change_unref);

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
    if (start_end < 2 && o->num_connections > 0) {
      g_printerr ("'%s' with no directions\n", type->name);
    }
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
       *   after identified with this check
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
        g_printerr ("'%s' main-cp misplaced!\n", type->name);
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

  if (o->num_connections > 1)
    cp_prev = o->connections[o->num_connections-1];
  for (i = 0; i < o->num_connections; ++i) {
    ConnectionPoint *cp = o->connections[i];
    if (cp_prev) {
      /* if the previous cp had the same coordinate x or y it should have the same direction */
      if (   strcmp (type->name, "GRAFCET - Vergent") == 0
	  || strcmp (type->name, "Standard - Polygon") == 0)
	continue; /* not a hard requirement */
      /* not with main point which usually has DIR_ALL */
      if (cp_prev->directions != DIR_ALL && cp->directions != DIR_ALL) {
	if (cp_prev->pos.x == cp->pos.x)
	  g_assert_cmpint ((cp_prev->directions & (DIR_WEST|DIR_EAST)), ==, (cp->directions & (DIR_WEST|DIR_EAST)));
	if (cp_prev->pos.y == cp->pos.y)
	  g_assert_cmpint ((cp_prev->directions & (DIR_NORTH|DIR_SOUTH)), ==, (cp->directions & (DIR_NORTH|DIR_SOUTH)));
      }
      cp_prev = cp;
    }
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
  g_clear_pointer (&o, g_free);
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
    /* strangely enough I found a crash with menu==NULL today ;) */
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
      if (item->callback && (item->active & DIAMENU_ACTIVE)) {
        DiaObjectChange *change;

        /* g_test_message() does not show normally */
        g_test_message ("\n\tCalling '%s'...", item->text);
        change = (item->callback) (o, &from, item->callback_data);
        if (!change) {
          g_test_message ("Undo/redo missing: %s\n", item->text);
        } else {
          /* Don't just call _object_change_free(change);
          * For 'Convert to *' this will screw up (destroy) the object
          * at hand'. So revert first, afterwards destroy the change.
          * The object parameter is deprecated, but still necessary!
          */
          dia_object_change_revert (change, o);
#if 0
	  /* XXX: Even more needs to be done to keep sane objects, see object_change_revert()
	   * in app/undo.c. AFAICT this is only needed to compensate for orthconn_set_points()
	   * while that does not update the object itself (which it does nowadays).
	   */
	  {
	    /* Make sure object updates its data: */
	    Point p = o->position;
	    (o->ops->move)(o,&p);
	  }
#endif
          g_clear_pointer (&change, dia_object_change_unref);
        }
      }
    }
  }
  /* finally */
  o->ops->destroy (o);
  g_clear_pointer (&o, g_free);
}


static void
_test_draw (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  /* using DiaPathRender for drawing internally */
  DiaObject *p;

  p = create_standard_path_from_object (o);
  if (p) /* play safe, maybe it can not be converted? */
    {
      const DiaRectangle *obb = dia_object_get_bounding_box (o);
      const DiaRectangle *pbb = dia_object_get_bounding_box (p);
      real epsilon = 0.2; /* XXX: smaller value needs longer exception list */

      /* Bounding boxes of these objects should be close, if not
       * this could be either some miscalculation within the object
       * implementation (update_data?) or some more generic problem
       * with DiaPathRenderer
       */
      if (!rectangle_in_rectangle (obb, pbb))
	{
#if 0
	  /* Generate exceptions list below with ./test-objects -q */
	  real ov = MAX(fabs (obb->top - pbb->top), fabs (obb->left - pbb->left));
	  ov = MAX(ov, fabs (obb->right - pbb->right));
	  ov = MAX(ov, fabs (obb->bottom - pbb->bottom));
	  if (ov >= epsilon)
	    g_print ("\t  else if (strcmp (type->name, \"%s\") == 0)\n"
		     "\t    epsilon = %.2g;\n", type->name, ov + 0.01);
#else
	  /* drawing _outside_ of objects's bounding box */
	  if (strcmp (type->name, "T-Junction") == 0)
	    epsilon = 0.2;
	  else if (strcmp (type->name, "Cisco - Edge Label Switch Router with NetFlow") == 0)
	    epsilon = 0.22;
	  else if (strcmp (type->name, "Cisco - SSL Terminator") == 0)
	    epsilon = 0.23;
	  else if (strcmp (type->name, "Cisco - VPN concentrator") == 0)
	    epsilon = 0.24;
	  else if (strcmp (type->name, "Cisco - End Office") == 0)
	    epsilon = 0.24;
	  else if (strcmp (type->name, "Cisco - Printer") == 0)
	    epsilon = 0.26;
	  else if (strcmp (type->name, "Circuit - Vertical Resistor") == 0)
	    epsilon = 0.26; /* win32: pass */
	  else if (strcmp (type->name, "Circuit - Horizontal Resistor") == 0)
	    epsilon = 0.26; /* win32: pass */
	  else if (strcmp (type->name, "Cisco - Location server") == 0)
	    epsilon = 0.27; /* win32: pass */
	  else if (strcmp (type->name, "Cisco - System controller") == 0)
	    epsilon = 0.27; /* win32: pass */
	  else if (strcmp (type->name, "Cisco - Pager") == 0)
	    epsilon = 0.28;
	  else if (strcmp (type->name, "Cisco - IAD router") == 0)
	    epsilon = 0.28;
	  else if (strcmp (type->name, "Cisco - Newton") == 0)
	    epsilon = 0.28;
	  else if (strcmp (type->name, "Cisco - Truck") == 0)
	    epsilon = 0.29;
	  else if (strcmp (type->name, "ER - Relationship") == 0)
	    epsilon = 0.29;
	  else if (strcmp (type->name, "Network - Base Station") == 0)
	    epsilon = 0.31; /* win32: pass */
	  else if (strcmp (type->name, "Cisco - Protocol Translator") == 0)
	    epsilon = 0.66; /* win32: 0.30 */
	  else if (strcmp (type->name, "Cisco - Cellular phone") == 0)
	    epsilon = 0.32; /* win32: pass */
	  else if (strcmp (type->name, "UML - Constraint") == 0)
	    epsilon = 0.33; /* win32: pass */
	  else if (strcmp (type->name, "UML - Message") == 0)
	    epsilon = 0.50; /* win32: 0.39 */
	  else if (strcmp (type->name, "Cisco - ICM") == 0)
	    epsilon = 0.34;
	  else if (strcmp (type->name, "Cisco - Access Gateway") == 0)
	    epsilon = 0.39; /* win32: pass */
	  else if (strcmp (type->name, "chemeng - SaT-floatinghead") == 0)
	    epsilon = 0.40;
	  else if (strcmp (type->name, "chemeng - kettle") == 0)
	    epsilon = 0.42;
	  else if (strcmp (type->name, "Cisco - NetRanger") == 0)
	    epsilon = 0.42; /* win32: pass */
	  else if (strcmp (type->name, "Cisco - Mac Woman") == 0)
	    epsilon = 0.47; /* win32: 0.43 */
	  else if (strcmp (type->name, "Pneum - press") == 0)
	    epsilon = 0.45; /* win32: 0.44 */
	  else if (strcmp (type->name, "Pneum - presspn") == 0)
	    epsilon = 0.44;
	  else if (strcmp (type->name, "Pneum - presshy") == 0)
	    epsilon = 0.44;
	  else if (strcmp (type->name, "Cisco - Optical Transport") == 0)
	    epsilon = 0.48; /* win32: 0.47 */
	  else if (strcmp (type->name, "Jackson - phenomenon") == 0)
	    epsilon = 0.51; /* osx: 0.39 */
	  else if (strcmp (type->name, "Cisco - Speaker") == 0)
	    epsilon = 0.58; /* win32: 0.57 */
	  else if (strcmp (type->name, "Cisco - 6705") == 0)
	    epsilon = 0.64;
	  else if (strcmp (type->name, "KAOS - mbr") == 0)
	    epsilon = 0.70; /* win32: 0.69 */
	  else if (strcmp (type->name, "FS - Flow") == 0)
	    epsilon = 0.73; /* osx: 0.58 */
	  else if (strcmp (type->name, "SADT - arrow") == 0)
	    epsilon = 0.75; /* win32: 0.74 */
	  else if (strcmp (type->name, "Network - WAN Connection") == 0)
	    epsilon = 0.86;
	  else if (strcmp (type->name, "Network - General Printer") == 0)
	    epsilon = 0.92;
	  else if (strcmp (type->name, "UML - Constraint") == 0)
	    epsilon = 1.1;
	  else if (strcmp (type->name, "Pneum - SEIJack") == 0)
	    epsilon = 1.1;
	  else if (strcmp (type->name, "Pneum - SEOJack") == 0)
	    epsilon = 1.1;
	  else if (strcmp (type->name, "Pneum - DEJack") == 0)
	    epsilon = 1.1; /* osx: 0.9 */
	  else if (strcmp (type->name, "SDL - Comment") == 0)
	    epsilon = 3.5; /* osx: 3.1 */

	  g_assert_cmpfloat (fabs (obb->top - pbb->top), <, epsilon);
	  g_assert_cmpfloat (fabs (obb->left - pbb->left), <, epsilon);
	  g_assert_cmpfloat (fabs (obb->right - pbb->right), <, epsilon);
	  g_assert_cmpfloat (fabs (obb->bottom - pbb->bottom), <, epsilon);
#endif
	}
      /* destroy path object */
      p->ops->destroy (p);
      g_clear_pointer (&p, g_free);
    }
  else
    {
      g_test_message ("SKIPPED (no path from %s)! ", type->name);
    }
  /* finally */
  o->ops->destroy (o);
  g_clear_pointer (&o, g_free);
}

static void
_test_distance_from (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  const DiaRectangle *ebox;
  Point center;
  real width, height;
  Point test;
  real outside = 0.01; /* tolerance value for being outside */

  /* This shall work for all objects so it does not check for full correctness,
   * but is currently tolerant enough for element and connection objects.
   * If it fails either the bounding calculation is bogus or distance_from method.
   */
  /* Outside of the enclosing (bounding) box can not be inside the object */
  ebox = dia_object_get_enclosing_box (o);
  center.x = (ebox->left + ebox->right) / 2;
  center.y = (ebox->top + ebox->bottom) / 2;
  width = ebox->right - ebox->left;
  height = ebox->bottom - ebox->top;

  /* Some custom objects still fail this check otherwise */
  if (   strcmp (type->name, "Civil - Gas Bottle") == 0
      || strcmp (type->name, "Cybernetics - l-sens") == 0)
    outside += 0.1;

  test.y = center.y;
  test.x = center.x - width/2 - outside;
  g_assert (o->ops->distance_from (o, &test) > 0 && "left");
  test.x = center.x + width/2 + outside;
  g_assert (o->ops->distance_from (o, &test) > 0 && "right");
  test.x = center.x;
  test.y = center.y - height/2 - outside;
  g_assert (o->ops->distance_from (o, &test) > 0 && "top");
  test.y = center.y + height/2 + outside;
  g_assert (o->ops->distance_from (o, &test) > 0 && "bottom");

  /* finally */
  o->ops->destroy (o);
  g_clear_pointer (&o, g_free);
}

#include "lib/prop_geomtypes.h" /* BezPointarrayProperty */

/* there is no v-table entry for Add/Delete (Corner|Segment)
 * so we have to search the object menu
 */
static DiaObjectChange *
_change_point (DiaObject *o, const gchar *verb, Point *pt)
{
  int i;
  ObjectMenuFunc omf = o->ops->get_object_menu;
  DiaMenu *menu = omf ? (omf)(o, pt) : NULL;
  /* strangely enough I found a crash with menu==NULL today ;) */
  int num_items = menu ? menu->num_items : 0;

  for (i = 0; i < num_items; ++i) {
    DiaMenuItem *item = &menu->items[i];

    if (item->text && strncmp (item->text, verb, strlen(verb)) == 0) {
      if (item->callback && (item->active & DIAMENU_ACTIVE))
	return (item->callback)(o, pt, item->callback_data);
    }
  }
  return NULL;
}


static Point
_bez_between (const BezPoint *a, const BezPoint *b)
{
  Point r = {
    ((a->type == BEZ_CURVE_TO ? a->p3.x : a->p1.x) + (b->type == BEZ_CURVE_TO ? b->p3.x : b->p1.x))/2,
    ((a->type == BEZ_CURVE_TO ? a->p3.y : a->p1.y) + (b->type == BEZ_CURVE_TO ? b->p3.y : b->p1.y))/2,
  };
  return r;
}

static void
_test_segments (gconstpointer user_data)
{
  const DiaObjectType *type = (const DiaObjectType *)user_data;
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  Property *prop;

  /* only few object support to add/remove objects, filter to these objects first */
  if ((prop = object_prop_by_name_type (o, "bez_points", PROP_TYPE_BEZPOINTARRAY)) != NULL) {
    /* get the points array to compare against */
    GArray *d1 = ((BezPointarrayProperty *)prop)->bezpointarray_data;
    /* add and delete some points */
    int n;
    for (n = 0; n < d1->len - 1; ++n) {
      DiaObjectChange *ch1, *ch2;
      from = _bez_between (&g_array_index(d1, BezPoint, n), &g_array_index(d1, BezPoint, n+1));
      if ((ch1 = _change_point (o, "Add ", &from)) != NULL) {
        int i;
        Property *prop2 = object_prop_by_name_type (o, "bez_points", PROP_TYPE_BEZPOINTARRAY);
        GArray *d2 = ((BezPointarrayProperty *)prop2)->bezpointarray_data;
        g_assert (d1->len == d2->len - 1);
        ch2 = _change_point (o, "Delete ", &from);
        prop2->ops->free (prop2);
        /* adding and deleting the same point shall lead to the initial state */
        prop2 = object_prop_by_name_type (o, "bez_points", PROP_TYPE_BEZPOINTARRAY);
        d2 = ((BezPointarrayProperty *)prop2)->bezpointarray_data;
        for (i = 0; i < d1->len; ++i) {
          BezPoint *bp1 = &g_array_index(d1, BezPoint, i);
          BezPoint *bp2 = &g_array_index(d2, BezPoint, i);
          g_assert_cmpfloat (fabs (bp1->p1.x - bp2->p1.x), <, EPSILON); /* for all types of BezPoint */
          g_assert_cmpfloat (fabs (bp1->p1.y - bp2->p1.y), <, EPSILON); /* - " - */
          g_assert (bp1->type == bp2->type);
          if (bp1->type != BEZ_CURVE_TO)
            continue;
          g_assert_cmpfloat (fabs (bp1->p2.x - bp2->p2.x), <, EPSILON);
          g_assert_cmpfloat (fabs (bp1->p2.y - bp2->p2.y), <, EPSILON);
          g_assert_cmpfloat (fabs (bp1->p3.x - bp2->p3.x), <, EPSILON);
          g_assert_cmpfloat (fabs (bp1->p3.y - bp2->p3.y), <, EPSILON);
        }
        prop2->ops->free (prop2);
        /* Check if undo is reconstructing the object, too */
        dia_object_change_revert (ch2, o);
        g_clear_pointer (&ch2, dia_object_change_unref);
        dia_object_change_revert (ch1, o);
        g_clear_pointer (&ch1, dia_object_change_unref);
        prop2 = object_prop_by_name_type (o, "bez_points", PROP_TYPE_BEZPOINTARRAY);
        d2 = ((BezPointarrayProperty *) prop2)->bezpointarray_data;
        for (i = 0; i < d1->len; ++i) {
          BezPoint *bp1 = &g_array_index(d1, BezPoint, i);
          BezPoint *bp2 = &g_array_index(d2, BezPoint, i);
          g_assert_cmpfloat (fabs (bp1->p1.x - bp2->p1.x), <, EPSILON); /* for all types of BezPoint */
          g_assert_cmpfloat (fabs (bp1->p1.y - bp2->p1.y), <, EPSILON); /* - " - */
          g_assert (bp1->type == bp2->type);
          if (bp1->type != BEZ_CURVE_TO)
            continue;
          g_assert_cmpfloat (fabs (bp1->p2.x - bp2->p2.x), <, EPSILON);
          g_assert_cmpfloat (fabs (bp1->p2.y - bp2->p2.y), <, EPSILON);
          g_assert_cmpfloat (fabs (bp1->p3.x - bp2->p3.x), <, EPSILON);
          g_assert_cmpfloat (fabs (bp1->p3.y - bp2->p3.y), <, EPSILON);
        }
        prop2->ops->free (prop2);
      }
    }
  } else if ((prop = object_prop_by_name_type (o, "poly_points", PROP_TYPE_POINTARRAY)) != NULL) {
    /* ToDo: the assumption does not hold for "orth_points", check if it should.  */
    /* get the points array to compare against */
    GArray *d1 = ((PointarrayProperty *)prop)->pointarray_data;
    /* add and delete some points */
    int n;
    for (n = 0; n < d1->len - 1; ++n) {
      DiaObjectChange *ch1, *ch2;
      from.x = (g_array_index(d1, Point, n).x + g_array_index(d1, Point, n+1).x) / 2;
      from.y = (g_array_index(d1, Point, n).y + g_array_index(d1, Point, n+1).y) / 2;
      if ((ch1 = _change_point (o, "Add ", &from)) != NULL) {
        int i;
        GArray *d2;
        Property *prop2 = object_prop_by_name_type (o, "poly_points", PROP_TYPE_POINTARRAY);
        d2 = ((PointarrayProperty *)prop2)->pointarray_data;
        g_assert (d1->len == d2->len - 1);
        ch2 = _change_point (o, "Delete ", &from);
        prop2->ops->free (prop2);
        /* adding and deleting the same point shall lead to the initial state */
        prop2 = object_prop_by_name_type (o, "poly_points", PROP_TYPE_POINTARRAY);
        d2 = ((PointarrayProperty *)prop2)->pointarray_data;
        for (i = 0; i < d1->len; ++i) {
          Point p1 = g_array_index(d1, Point, i);
          Point p2 = g_array_index(d2, Point, i);
          g_assert_cmpfloat (fabs (p1.x - p2.x), <, EPSILON);
          g_assert_cmpfloat (fabs (p1.y - p2.y), <, EPSILON);
        }
        prop2->ops->free (prop2);
        /* Check if undo is reconstructing the object, too */
        dia_object_change_revert (ch2, o);
        g_clear_pointer (&ch2, dia_object_change_unref);
        dia_object_change_revert (ch1, o);
        g_clear_pointer (&ch1, dia_object_change_unref);
        prop2 = object_prop_by_name_type (o, "poly_points", PROP_TYPE_POINTARRAY);
        d2 = ((PointarrayProperty *) prop2)->pointarray_data;
        for (i = 0; i < d1->len; ++i) {
          Point p1 = g_array_index (d1, Point, i);
          Point p2 = g_array_index (d2, Point, i);
          g_assert_cmpfloat (fabs (p1.x - p2.x), <, EPSILON);
          g_assert_cmpfloat (fabs (p1.y - p2.y), <, EPSILON);
        }
        prop2->ops->free (prop2);
      }
    }
  } else {
    g_test_message ("n.a. ");
  }
  if (prop)
    prop->ops->free (prop);
  /* finally */
  o->ops->destroy (o);
  g_clear_pointer (&o, g_free);
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
  g_clear_pointer (&testpath, g_free);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Copy");
  g_test_add_data_func (testpath, type, _test_copy);
  g_clear_pointer (&testpath, g_free);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Movement");
  g_test_add_data_func (testpath, type, _test_movement);
  g_clear_pointer (&testpath, g_free);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Change");
  g_test_add_data_func (testpath, type, _test_change);
  g_clear_pointer (&testpath, g_free);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "MoveHandle");
  g_test_add_data_func (testpath, type, _test_move_handle);
  g_clear_pointer (&testpath, g_free);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "ConnectionPoints");
  g_test_add_data_func (testpath, type, _test_connectionpoint_consistency);
  g_clear_pointer (&testpath, g_free);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "ObjectMenu");
  g_test_add_data_func (testpath, type, _test_object_menu);
  g_clear_pointer (&testpath, g_free);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Draw");
  g_test_add_data_func (testpath, type, _test_draw);
  g_clear_pointer (&testpath, g_free);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "DistanceFrom");
  g_test_add_data_func (testpath, type, _test_distance_from);
  g_clear_pointer (&testpath, g_free);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Segments");
  g_test_add_data_func (testpath, type, _test_segments);
  g_clear_pointer (&testpath, g_free);

  ++num_objects;
}

#ifdef G_OS_WIN32
#include <windows.h>
#endif

int
main (int argc, char** argv)
{
  GList *plugins = NULL;
  int ret = 0;

#ifdef G_OS_WIN32
  /* No dialog if it fails, please. */
  SetErrorMode(SetErrorMode(0) | SEM_NOGPFAULTERRORBOX);
#endif

  /* not using gtk_test_init() means we can only test non-gtk facilities of objects */
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
  g_printerr ("%d objects.\n", num_objects);

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
