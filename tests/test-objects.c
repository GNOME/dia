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

#if GLIB_CHECK_VERSION(2,16,0)
#include <glib/gtestutils.h>
#endif

#include "object.h"
#include "plug-ins.h"
#include "libdia.h"

/* allows to select specific objects for testing */
static gchar *_type_name = NULL;
const real EPSILON = 1e-6;

int num_objects = 0;

static void
_test_creation (const DiaObjectType *type)
{
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
  g_assert (o->ops->get_object_menu == NULL || o->ops->get_object_menu(o, &point) != NULL);
  
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
}

static void
_test_copy (const DiaObjectType *type)
{
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
  oc->ops->destroy (oc);
}

static void
_test_modification (const DiaObjectType *type)
{
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  Rectangle bbox1, bbox2;
  Point to = {10, 10};
  
  /* does the object move ... ? */
  from = o->position;
  bbox1 = o->bounding_box;
  /* ... not (= hack used to force an update call) */
  o->ops->move (o, &o->position);
  bbox2 = o->bounding_box;
  g_assert (   fabs((bbox2.right - bbox2.left) - (bbox1.right - bbox1.left)) < EPSILON
            && fabs((bbox2.bottom - bbox2.top) - (bbox1.bottom - bbox1.top)) < EPSILON);
  /* .... really: without changing size ? */
  bbox1 = o->bounding_box;
  o->ops->move (o, &to);
  bbox2 = o->bounding_box;
  g_assert (   fabs((bbox2.right - bbox2.left) - (bbox1.right - bbox1.left)) < EPSILON
            && fabs((bbox2.bottom - bbox2.top) - (bbox1.bottom - bbox1.top)) < EPSILON);


  /* finally */
  o->ops->destroy (o);
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

#if GLIB_CHECK_VERSION(2,16,0)
  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Creation");
  g_test_add_data_func (testpath, type, _test_creation);
  g_free (testpath);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Copy");
  g_test_add_data_func (testpath, type, _test_copy);
  g_free (testpath);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Modification");
  g_test_add_data_func (testpath, type, _test_modification);
  g_free (testpath);
#endif

  ++num_objects;
}

int
main (int argc, char** argv)
{
  GList *plugins = NULL;
  int ret = 0;
  
#if GLIB_CHECK_VERSION(2,16,0)
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
      dia_register_plugins ();
    }
  plugins = dia_list_plugins ();
  g_assert (g_list_length (plugins) > 0);

  object_registry_foreach (_ot_item, "/Dia/Objects");

  ret = g_test_run ();
  g_print ("Tested %d objects.\n", num_objects);
#else
  g_print ("GLib version does not support g_test_*()");
#endif
  return ret;
}
