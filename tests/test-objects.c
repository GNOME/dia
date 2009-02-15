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
#include "dialib.h"

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

/* samll helper to just throw it away */
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
_test_movement (const DiaObjectType *type)
{
  Handle *h1 = NULL, *h2 = NULL;
  Point from = {0, 0};
  DiaObject *o = type->ops->create (&from, type->default_user_data, &h1, &h2);
  Rectangle bbox1, bbox2;
  Point to = {10, 10};
  ObjectChange *change;
  
  /* does the object move ... ? */
  from = o->position;
  bbox1 = o->bounding_box;
  /* ... not (= hack used to force an update call) */
  change = o->ops->move (o, &o->position);
  if (change) /* usually this is NULL for move */
    _object_change_free(change);
  bbox2 = o->bounding_box;
  g_assert (   fabs((bbox2.right - bbox2.left) - (bbox1.right - bbox1.left)) < EPSILON
            && fabs((bbox2.bottom - bbox2.top) - (bbox1.bottom - bbox1.top)) < EPSILON);
  /* .... really: without changing size ? */
  bbox1 = o->bounding_box;
  change = o->ops->move (o, &to);
  if (change) /* usually this is NULL for move */
    _object_change_free(change);
  bbox2 = o->bounding_box;
  /* test fails e.g. for 'Cisco - Web cluster' probably due to bezier-bbox-issues: bug 568115 */
  if (   strcmp (type->name, "Cisco - Web cluster") == 0
      || strcmp (type->name, "Cisco - University") == 0
      || strcmp (type->name, "Cisco - Web browser") == 0
      || strcmp (type->name, "Cisco - Firewall Service Module") == 0
      || strcmp (type->name, "Cisco - Data Center Switch") == 0
      || strcmp (type->name, "Cisco - Content Engine (Cache Director)") == 0
      || strcmp (type->name, "Cisco - Storage Solution Engine") == 0
      || strcmp (type->name, "Cisco - IP Softphone") == 0
      || strcmp (type->name, "Cisco - Data Center Switch") == 0
      || strcmp (type->name, "Cisco - Video Camera right") == 0
      || strcmp (type->name, "Cisco - Video camera") == 0
      || strcmp (type->name, "Cisco - Data Center Switch") == 0
      || strcmp (type->name, "Cisco - File cabinet") == 0
      || strcmp (type->name, "Cisco - Universal Gateway") == 0
      || strcmp (type->name, "Cisco - CDDI-FDDI") == 0
      || strcmp (type->name, "Cisco - Generic processor") == 0
      || strcmp (type->name, "Cisco - CiscoWorks workstation") == 0
      || strcmp (type->name, "Cisco - Laptop") == 0
      || strcmp (type->name, "Cisco - Breakout box") == 0
      || strcmp (type->name, "Cisco - IBM mainframe") == 0
      || strcmp (type->name, "Cisco - Multilayer switch") == 0
      || strcmp (type->name, "Cisco - Edge Label Switch Router with NetFlow") == 0
      || strcmp (type->name, "Cisco - Route Switch Processor with Si") == 0
      || strcmp (type->name, "Cisco - Router in building") == 0
      || strcmp (type->name, "Cisco - Cisco CA") == 0
      || strcmp (type->name, "Cisco - Unity server") == 0
      || strcmp (type->name, "Cisco - SONET MUX") == 0
      || strcmp (type->name, "Cisco - PC Video") == 0
      || strcmp (type->name, "Cisco - IP Telephony Router") == 0
      || strcmp (type->name, "Cisco - Content Service Switch 1100") == 0
      || strcmp (type->name, "Cisco - Speaker") == 0
      || strcmp (type->name, "Cisco - Generic softswitch") == 0
      || strcmp (type->name, "Network - A Telephone") == 0
      || strcmp (type->name, "Cisco - Multilayer Switch with Silicon") == 0
      || strcmp (type->name, "Cisco - Androgynous Person") == 0
      || strcmp (type->name, "Cisco - MGX 8000 Series Voice Gateway") == 0
      || strcmp (type->name, "Cisco - PC Adapter Card") == 0
      || strcmp (type->name, "Cisco - Wireless Transport") == 0
      || strcmp (type->name, "Cisco - Workgroup director") == 0
      || strcmp (type->name, "Cisco - IntelliSwitch Stack") == 0
      || strcmp (type->name, "Cisco - ATA") == 0
      || strcmp (type->name, "Cisco - BBS") == 0
      || strcmp (type->name, "Cisco - Automatic Protection Switching") == 0
      || strcmp (type->name, "Cisco - Mobile Access Router") == 0
      || strcmp (type->name, "Cisco - MCU") == 0
      || strcmp (type->name, "Cisco - Layer 2 Remote Switch") == 0
      || strcmp (type->name, "Cisco - CDM Content Distribution Manager") == 0
      || strcmp (type->name, "Cisco - CiscoWorks Man") == 0
      || strcmp (type->name, "Cisco - Access Gateway") == 0
      || strcmp (type->name, "Cisco - Government Building") == 0
      || strcmp (type->name, "Cisco - ITP") == 0
      || strcmp (type->name, "Cisco - CiscoSecurity") == 0
      || strcmp (type->name, "Cisco - VIP") == 0
      || strcmp (type->name, "Cisco - 10700") == 0
      || strcmp (type->name, "Cisco - IBM mainframe with FEP") == 0
      || strcmp (type->name, "Cisco - Phone Ethernet") == 0
      || strcmp (type->name, "Cisco - Cisco Hub") == 0
      || strcmp (type->name, "Cisco - Mac Woman") == 0
      || strcmp (type->name, "Cisco - Small Business") == 0
      || strcmp (type->name, "Cisco - Content Switch") == 0
      || strcmp (type->name, "Cisco - Satellite dish") == 0
      || strcmp (type->name, "Cisco - Storage Router") == 0
      || strcmp (type->name, "Cisco - NetFlow router") == 0
      || strcmp (type->name, "Cisco - STB") == 0
      || strcmp (type->name, "Cisco - STB (set top box)") == 0
      || strcmp (type->name, "Cisco - Key") == 0
      || strcmp (type->name, "Cisco - Router with Firewall") == 0
      || strcmp (type->name, "Cisco - Router with Silicon Switch") == 0
      || strcmp (type->name, "Cisco - Data Center Switch Reversed") == 0
      || strcmp (type->name, "Cisco - 7500ARS (7513) Router") == 0
      || strcmp (type->name, "Cisco - 7507 Router") == 0
      || strcmp (type->name, "Cisco - ATM Router") == 0
      || strcmp (type->name, "Cisco - Distributed Director") == 0
      || strcmp (type->name, "Cisco - Centri Firewall") == 0
      || strcmp (type->name, "Cisco - IAD router") == 0
      || strcmp (type->name, "Cisco - Optical Cross-Connect") == 0
      || strcmp (type->name, "Cisco - Cloud Dark") == 0
      || strcmp (type->name, "Cisco - PC Man left") == 0
      || strcmp (type->name, "Cisco - Cloud Gold") == 0
      || strcmp (type->name, "Cisco - ME 1100") == 0
      || strcmp (type->name, "Cisco - Satellite") == 0
      || strcmp (type->name, "Cisco - SUN workstation") == 0
      || strcmp (type->name, "Cisco - Multilayer Switch with Silicon subdued") == 0
      || strcmp (type->name, "Cisco - Lock") == 0
      || strcmp (type->name, "Cisco - SIP Proxy server") == 0
      || strcmp (type->name, "Cisco - WWW server") == 0
      || strcmp (type->name, "Cisco - Channelized Pipe") == 0
      || strcmp (type->name, "Cisco - PC Man") == 0
      || strcmp (type->name, "Cisco - Headphones") == 0
      || strcmp (type->name, "Cisco - PC with Router-Based Software") == 0
      || strcmp (type->name, "Cisco - Content Service Router") == 0
      || strcmp (type->name, "Cisco - Microphone") == 0
      || strcmp (type->name, "Cisco - Cloud White") == 0
      || strcmp (type->name, "Cisco - Directory Server") == 0
      || strcmp (type->name, "Cisco - Modem") == 0
      || strcmp (type->name, "Cisco - Cloud") == 0
      || strcmp (type->name, "Cisco - Octel") == 0
      || strcmp (type->name, "Cisco - Lock and Key") == 0
      || strcmp (type->name, "Cisco - Route Switch Processor") == 0

      || strcmp (type->name, "Assorted - Heart") == 0
      /* FIXME: this shape should be simple enough to actually fix the bug */
      || strcmp (type->name, "Flowchart - Document") == 0
     )
    g_print ("SKIPPED! ");
  else
    g_assert (   fabs((bbox2.right - bbox2.left) - (bbox1.right - bbox1.left)) < EPSILON
              && fabs((bbox2.bottom - bbox2.top) - (bbox1.bottom - bbox1.top)) < EPSILON);

  /* finally */
  o->ops->destroy (o);
}

static void
_test_change (const DiaObjectType *type)
{
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

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Movement");
  g_test_add_data_func (testpath, type, _test_movement);
  g_free (testpath);

  testpath = g_strdup_printf ("%s/%s/%s", base, name, "Change");
  g_test_add_data_func (testpath, type, _test_change);
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
  g_print ("%d objects.\n", num_objects);
#else
  g_print ("GLib version does not support g_test_*()");
#endif
  return ret;
}
