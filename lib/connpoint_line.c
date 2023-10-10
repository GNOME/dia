/*
 * SADT diagram support for dia
 * Copyright(C) 2000 Cyrille Chepelov
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

#include <string.h>
#include "connpoint_line.h"
#include "connectionpoint.h"
#include "dia_xml.h"


#define DEBUG_PARENT 0
#define DEBUG_ORDER 0


/**
 * SECTION:connpoint_line
 * @Title: ConnPointLine
 *
 * Connection point line is a helper struct, to hold a few connection points
 * on a line segment. There can be a variable number of these connection
 * points. The user should be made able to add or remove some connection
 * points.
 *
 * #ConnPointLine can be used to implement dynamic #ConnectionPoints for
 * a #DiaObject. It supports undo/redo, load/save to standard props
 * and obviously connections to it.
 */

static void cpl_reorder_connections(ConnPointLine *cpl);


inline static ConnectionPoint *
new_connpoint (DiaObject *obj)
{
  ConnectionPoint *cp = g_new0 (ConnectionPoint,1);
  cp->object = obj;
  return cp;
}


inline static void
del_connpoint (ConnectionPoint *cp)
{
  g_clear_pointer (&cp, g_free);
}


static ConnectionPoint *
cpl_remove_connpoint(ConnPointLine *cpl,int pos)
{
  ConnectionPoint *cp;

  g_assert (cpl->num_connections > 0);

  if (pos >= cpl->num_connections) {
    pos = cpl->num_connections - 1;
  } else {
    while (pos < 0) pos += cpl->num_connections;
  }

  cp = (ConnectionPoint *)(g_slist_nth(cpl->connections,pos)->data);
  g_assert(cp);

  cpl->connections = g_slist_remove(cpl->connections,(gpointer)cp);
  object_remove_connectionpoint(cpl->parent,cp);

  cpl->num_connections--;
  /* removing a point doesn't change the order of the remaining ones, so we
     don't need to call cpl_reorder_connections. */
  /* The caller is responsible for freeing the removed connection point */
  return cp;
}

static void
cpl_add_connectionpoint_at(ConnPointLine *cpl, int pos,ConnectionPoint *cp)
{
  if (pos == 0) {
    /* special case handling so that the order of CPL groups in
       the parent's CP list is preserved. */
    int fpos,i;
    ConnectionPoint *fcp;
    g_assert(cpl->connections);
    fpos = -1;
    fcp = (ConnectionPoint *)(cpl->connections->data);
    g_assert(fcp);
    for (i=0; i<cpl->parent->num_connections; i++) {
      if (cpl->parent->connections[i] == fcp) {
	fpos = i;
	break;
      }
    }
    g_assert(fpos >= 0);
    object_add_connectionpoint_at(cpl->parent,cp,fpos);
  }else {
    /* XXX : make this a little better ; try to insert at the correct
       position right away to eliminate cpl_reorder_connection */
    object_add_connectionpoint(cpl->parent,cp);
  }
  if (pos < 0) {
    cpl->connections = g_slist_append(cpl->connections,(gpointer)cp);
  }
  else {
    cpl->connections = g_slist_insert(cpl->connections,(gpointer)cp,pos);
  }
  cpl->num_connections++;

  /* we should call
     cpl_reorder_connections(cpl);
     before we leave the object !! However, this is delayed, for the case
     several CP's are added at once (initialisation). */
}

inline static void
cpl_add_connectionpoint(ConnPointLine *cpl,ConnectionPoint *cp)
{
  cpl_add_connectionpoint_at(cpl,-1,cp);
}

ConnPointLine *
connpointline_create(DiaObject *parent, int num_connections)
{
  ConnPointLine *cpl;
  int i;

  cpl = g_new0(ConnPointLine,1);
  cpl->parent = parent;

  cpl->connections = NULL;
  for (i=0; i<num_connections; i++) {
    cpl_add_connectionpoint(cpl,new_connpoint(cpl->parent));
  }
  cpl_reorder_connections(cpl);
  return cpl;
}


void
connpointline_destroy (ConnPointLine *cpl)
{
  while (cpl->num_connections > 0) {
    del_connpoint (cpl_remove_connpoint (cpl, 0));
  }

  g_clear_pointer (&cpl, g_free);
}


static ConnPointLine *
cpl_inplacecreate(DiaObject *obj, int nc, int *realconncount)
{
  int i;
  ConnPointLine *newcpl;
  ConnectionPoint *cp;

  /* This thing creates a connection point line without actually adding
     connection points to the parent object. */
  newcpl = g_new0(ConnPointLine,1);
  newcpl->parent = obj;

  for (i=0; i < nc; i++,(*realconncount)++) {
    cp = g_new0(ConnectionPoint,1);
    cp->object = newcpl->parent;
    obj->connections[*realconncount] = cp;
    newcpl->connections = g_slist_append(newcpl->connections,cp);
  }
  newcpl->num_connections = nc;
  return newcpl;
}

ConnPointLine *
connpointline_load(DiaObject *obj,ObjectNode obj_node,
		   const gchar *name, int default_nc,int *realconncount,
		   DiaContext *ctx)
{
  ConnPointLine *cpl;
  int nc = default_nc;
  AttributeNode attr;

  attr = object_find_attribute(obj_node, name);
  if (attr != NULL)
    nc = data_int(attribute_first_data(attr), ctx);
  cpl = connpointline_create(obj,nc);

  if (realconncount) (*realconncount) += cpl->num_connections;
  return cpl;
  /* NOT this !
  return cpl_inplacecreate(obj,
			   load_int(obj_node,name,default_nc),
			   realconncount);
  */
}

void
connpointline_save(ConnPointLine *cpl,ObjectNode obj_node,
		   const gchar *name, DiaContext *ctx)
{
  data_add_int(new_attribute(obj_node, name),cpl->num_connections, ctx);
}

ConnPointLine *
connpointline_copy(DiaObject *newobj,ConnPointLine *cpl, int *realconncount)
{
  g_assert(realconncount);
  return cpl_inplacecreate(newobj,cpl->num_connections,realconncount);
}

void connpointline_update(ConnPointLine *cpl)
{

}

void
connpointline_putonaline(ConnPointLine *cpl,Point *start,Point *end, gint dirs)
{
  Point se_vector;
  real se_len,pseudopoints;
  int i;
  GSList *elem;

  point_copy(&se_vector, end);
  point_sub(&se_vector, start);

  se_len = point_len(&se_vector);

  if (se_len > 0)
    point_normalize(&se_vector);

  cpl->start = *start;
  cpl->end = *end;

  if (dirs != DIR_NONE)
    /* use the one given by the caller */;
  else if (fabs(se_vector.x) > fabs(se_vector.y))
    dirs = DIR_NORTH|DIR_SOUTH;
  else
    dirs = DIR_EAST|DIR_WEST;

  pseudopoints = cpl->num_connections + 1; /* here, we count the start and end
					    points as eating real positions. */
  for (i=0, elem=cpl->connections;
       i<cpl->num_connections;
       i++,elem=g_slist_next(elem)) {
    ConnectionPoint *cp = (ConnectionPoint *)(elem->data);
    cp->pos = se_vector;
    cp->directions = dirs;
    point_scale(&cp->pos,se_len * (i+1.0)/pseudopoints);
    point_add(&cp->pos,start);
  }
}


/* These object_* functions are useful to me, because of what they do, I think
   they belong to lib/object.c ; should I move them ? */
static void
object_move_connection(DiaObject *obj,int sourcepos,int destpos)
{
  ConnectionPoint *cp;
  g_assert(destpos < sourcepos);
  cp = obj->connections[sourcepos];

  memmove(&(obj->connections[destpos+1]),&(obj->connections[destpos]),
	  sizeof(ConnectionPoint *)*(sourcepos-destpos));
  obj->connections[destpos] = cp;
}

static int
object_find_connection(DiaObject *obj, ConnectionPoint *cp, int startpos)
{
  int i;
  for (i = startpos; i < obj->num_connections; i++) {
    if (obj->connections[i] == cp) return i;
  }
  return -1; /* should not happen */
}


#if DEBUG_ORDER
static int obj_find_connection(DiaObject *obj,ConnectionPoint *cp)
{
  int i;
  for (i=0;i<obj->num_connections;i++)
    if (cp == obj->connections[i]) return i;
  return -1;
}


static void cpl_dump_connections(ConnPointLine *cpl)
{
  DiaObject *obj = cpl->parent;
  int i;
  GSList *elem;
  ConnectionPoint *cp;

  g_message("CPL order dump");
  for (i=0,elem = cpl->connections;
       i<cpl->num_connections;
       i++,elem = g_slist_next(elem)) {
    cp = (ConnectionPoint *)(elem->data);
    g_message("connection %p %d@CPL %d@OBJ",
	      cp,i,obj_find_connection(obj,cp));
  }
}
#endif

static void
cpl_reorder_connections(ConnPointLine *cpl)
{
  /* This is needed, so that we don't mess up the loaded connections if
     we save after the user has removed and added some connection points.
     Normally, if an object owns several CPL, the order of the groups of
     connectionpoints in its connectionpoint list should not change, as long
     as we call this function whenever we do something.

     The CPL has two big responsibilities here : first, it messes with
     the parent object's structures (ugh), second, it must ensure that its
     first CP is inserted so that it is found first in the parent's CP list,
     and that the order of CP groups in the parent's CP list is respected (so
     that the parent could have several different CPL and rely on the order).
  */

  int i,j,first;
  ConnectionPoint *cp;
  GSList *elem;
  DiaObject *obj;

  if (!cpl->connections) return;
  #if DEBUG_ORDER
  g_message("before cpl_reorder");
  cpl_dump_connections(cpl);
  #endif

  first = -1;
  cp = (ConnectionPoint *)(cpl->connections->data);
  obj = cpl->parent;
  for (i=0; i<obj->num_connections; i++){
    if (obj->connections[i] == cp) {
      first = i;
      break;
    }
  }
  g_assert(first >= 0); /* otherwise things went loose badly. */
  for (i=0,j=first,elem=cpl->connections;
       i<cpl->num_connections;
       elem=g_slist_next(elem),i++,j++) {
    cp = (ConnectionPoint *)(elem->data); /* = cpl->connections[i] */
    if ( cp != obj->connections[j]) { /* first time will always be false.
					 Is GCC that smart ? Probably not. */
      object_move_connection(obj,object_find_connection(obj,cp,j),j);
    }
  }
#if DEBUG_ORDER
  g_message("after cpl_reorder");
  cpl_dump_connections(cpl);
#endif
#if DEBUG_PARENT
  j = 0;
  for (i=0; i<cpl->parent->num_connections;i++)
    if (!cpl->parent->connections[i]) j++;
  /* We should never make such holes !*/
  if (j) g_warning("in cpl_reorder_connections there are %d holes in the parent's ConnectionPoint list !",j);
#endif
}


int
connpointline_can_add_point(ConnPointLine *cpl, Point *clicked)
{
  return 1;
}

int
connpointline_can_remove_point(ConnPointLine *cpl, Point *clicked)
{
  if (cpl->num_connections <= 1)
    return 0;
  else
    return 1;
}

static int
cpl_get_pointbefore(ConnPointLine *cpl, Point *clickedpoint)
{
  int i, pos = -1;
  GSList *elem;
  ConnectionPoint *cp;
  real dist = 65536.0;
  real tmpdist;

  if (!clickedpoint) return 0;

  for (i=0,elem=cpl->connections;
       i<cpl->num_connections;
       i++,elem=g_slist_next(elem)) {
    cp = (ConnectionPoint *)(elem->data);

    tmpdist = distance_point_point(&cp->pos,clickedpoint);
    if (tmpdist < dist) {
      dist = tmpdist;
      pos = i;
    }
  }
  tmpdist = distance_point_point(&cpl->end,clickedpoint);
  if (tmpdist < dist) {
    /*dist = tmpdist; */
    pos = -1;
  }
  return pos;
}


struct _DiaConnPointLineObjectChange {
  DiaObjectChange obj_change;

  int add; /* How much to add or remove */
  int applied; /* 1 if the event has been applied. */

  ConnPointLine *cpl;
  int pos; /* Position where the change happened. */
  ConnectionPoint **cp; /* The removed connection point. */
};


DIA_DEFINE_OBJECT_CHANGE (DiaConnPointLineObjectChange,
                          dia_conn_point_line_object_change)


static void
dia_conn_point_line_object_change_addremove (DiaConnPointLineObjectChange *change,
                                             ConnPointLine                *cpl,
                                             int                           action,
                                             int                           resultingapplied)
{
  if (action != 0) {
    if (action > 0) { /* We should add */
      while (action--) {
        cpl_add_connectionpoint_at (cpl, change->pos, change->cp[action]);
        change->cp[action] = NULL;
      }
      cpl_reorder_connections(cpl);
    } else { /* We should remove. Warning, action is negative. */
      while (action++) {
        change->cp[-action] = cpl_remove_connpoint (cpl, change->pos);
      }
    }
  } else {
    g_warning ("cpl_change_addremove(): null action !");
  }
  change->applied = resultingapplied;
}


static void
dia_conn_point_line_object_change_apply (DiaObjectChange *self, DiaObject *probablynotcpl)
{
  DiaConnPointLineObjectChange *change = DIA_CONN_POINT_LINE_OBJECT_CHANGE (self);

  dia_conn_point_line_object_change_addremove (change, change->cpl, change->add, 1);
}


static void
dia_conn_point_line_object_change_revert (DiaObjectChange *self, DiaObject *probablynotcpl)
{
  DiaConnPointLineObjectChange *change = DIA_CONN_POINT_LINE_OBJECT_CHANGE (self);

  dia_conn_point_line_object_change_addremove (change, change->cpl, -(change->add), 0);
}


static void
dia_conn_point_line_object_change_free (DiaObjectChange *self)
{
  DiaConnPointLineObjectChange *change = DIA_CONN_POINT_LINE_OBJECT_CHANGE (self);
  int i = ABS (change->add);

  while (i--) {
    if (change->cp[i]) {
      del_connpoint (change->cp[i]);
    }
  }
  g_clear_pointer (&change->cp, g_free);

  change->cp = (ConnectionPoint **)(0xDEADBEEF);
}


static DiaObjectChange *
dia_conn_point_line_object_change_new (ConnPointLine *cpl, int pos, int add)
{
  DiaConnPointLineObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_CONN_POINT_LINE_OBJECT_CHANGE);

  change->cpl = cpl;
  change->applied = 0;
  change->add = add;
  change->pos = pos;

  change->cp = g_new0 (ConnectionPoint *, ABS (add));
  while (add-- > 0) {
    change->cp[add] = new_connpoint (cpl->parent);
  }

  return DIA_OBJECT_CHANGE (change);
}


DiaObjectChange *
connpointline_add_points (ConnPointLine *cpl,
                          Point         *clickedpoint,
                          int            count)
{
  int pos;
  DiaObjectChange *change;

  pos = cpl_get_pointbefore (cpl, clickedpoint);
  change = dia_conn_point_line_object_change_new (cpl, pos, count);

  dia_object_change_apply (change, DIA_OBJECT (cpl));

  return change;
}


DiaObjectChange *
connpointline_remove_points (ConnPointLine *cpl,
                             Point         *clickedpoint,
                             int            count)
{
  int pos;
  DiaObjectChange *change;

  pos = cpl_get_pointbefore (cpl, clickedpoint);
  change = dia_conn_point_line_object_change_new (cpl, pos, -count);

  dia_object_change_apply (change, DIA_OBJECT (cpl));

  return change;
}


int
connpointline_adjust_count (ConnPointLine *cpl,
                            int            newcount,
                            Point         *where)
{
  int oldcount,delta;

  oldcount = cpl->num_connections;

  if (newcount < 0) newcount = 0;

  delta = newcount - oldcount;
  if (delta != 0) {
    DiaObjectChange *change;
    /*g_message("going to adjust %d (to be %d)",delta,shouldbe);*/

    if (delta > 0) {
      change = connpointline_add_points (cpl, where, delta);
    } else {
      change = connpointline_remove_points (cpl, where, -delta);
    }

    g_clear_pointer (&change, dia_object_change_unref);
    /* we don't really need this change object. */
  }

  return oldcount;
}
