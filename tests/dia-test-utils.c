/*
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 * SPDX-FileCopyrightText: Copyright 2025 Zander Brown
 *
 * Originally from KGX (kgx-test-utils.c)
 */

#include "config.h"

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "dia-test-utils.h"


GQuark dia_test_notify_data_quark (void);


G_DEFINE_QUARK (dia-test-notify-data, dia_test_notify_data)


typedef struct _NotifyData NotifyData;
struct _NotifyData {
  GPtrArray *expected_properties;
  GPtrArray *got_properties;
  GObject *object;
  gulong handler;
  gboolean no_notify;
};


static void
notify_data_free (gpointer data)
{
  NotifyData *self = data;

  g_clear_pointer (&self->expected_properties, g_ptr_array_unref);
  g_clear_pointer (&self->got_properties, g_ptr_array_unref);
  g_clear_signal_handler (&self->handler, self->object);
  g_clear_weak_pointer (&self->object);

  g_free (self);
}


G_DEFINE_AUTOPTR_CLEANUP_FUNC (NotifyData, notify_data_free)


static void
handle_notify (GObject                *object,
               GParamSpec             *pspec,
               G_GNUC_UNUSED gpointer  user_data)
{
  NotifyData *data =
    g_object_get_qdata (object, dia_test_notify_data_quark ());
  guint where;

  g_ptr_array_add (data->got_properties, g_strdup (pspec->name));

  if (g_ptr_array_find_with_equal_func (data->expected_properties,
                                        pspec->name,
                                        g_str_equal,
                                        &where)) {
    g_ptr_array_remove_index (data->expected_properties, where);
  }
}


static NotifyData *
get_notify_data (gpointer object)
{
  NotifyData *data =
    g_object_get_qdata (object, dia_test_notify_data_quark ());
  NotifyData *new_data;

  if (data) {
    return data;
  }

  new_data = g_new0 (NotifyData, 1);
  new_data->expected_properties =
    g_ptr_array_new_null_terminated (5, g_free, TRUE);
  new_data->got_properties =
    g_ptr_array_new_null_terminated (5, g_free, TRUE);
  g_set_weak_pointer (&new_data->object, object);
  new_data->handler =
    g_signal_connect (object, "notify", G_CALLBACK (handle_notify), NULL);

  g_object_set_qdata_full (object,
                           dia_test_notify_data_quark (),
                           new_data,
                           notify_data_free);

  return new_data;
}


/**
 * dia_expect_property_notify:
 * @object: (type GObject.Object): the object to watch
 * @property: the property to expect
 *
 * Adds @property to the list of expected property notifications on @object,
 * this can be called multiple times with the same @property to require
 * the property be notified more than once.
 *
 * This cannot be used at the same time as [func@Dia.expect_no_notify].
 */
void
dia_expect_property_notify (gpointer object, const char *property)
{
  NotifyData *data = get_notify_data (object);

  g_assert_false (data->no_notify);

  g_ptr_array_add (data->expected_properties, g_strdup (property));
}


/**
 * dia_expect_no_notify:
 * @object: (type GObject.Object): the object to watch
 *
 * Watches @object with the expectation that *no* notifications are received
 * before the corresponding [func@Dia.assert_expected_notifies] call.
 *
 * This cannot be used at the same time as [func@Dia.expect_property_notify].
 */
void
dia_expect_no_notify (gpointer object)
{
  NotifyData *data = get_notify_data (object);

  g_assert_cmpint (data->expected_properties->len, ==, 0);

  data->no_notify = TRUE;
}


/**
 * dia_assert_expected_notifies:
 * @object: (type GObject.Object): the watched object
 *
 * If notifications were expected, assert that each of them were received.
 * Note that order isn't enforced, and we ignore an ‘extra’ notifications, we
 * simple assert that the ‘expected’ notifications were fulfilled.
 *
 * Conversely, if we expected *no* notifications, we assert that there were
 * indeed none *whatsoever* on @object.
 */
void
dia_assert_expected_notifies (gpointer object)
{
  NotifyData *data =
    g_object_steal_qdata (object, dia_test_notify_data_quark ());

  g_assert_nonnull (data);

  if (data->no_notify) {
    char *notified =
      g_strjoinv (", ", (GStrv) data->got_properties->pdata);

    g_assert_cmpstr (notified, ==, "");

    g_clear_pointer (&notified, g_free);
  } else {
    char *unnotified =
      g_strjoinv (", ", (GStrv) data->expected_properties->pdata);

    g_assert_cmpstr (unnotified, ==, "");

    g_clear_pointer (&unnotified, g_free);
  }

  g_clear_pointer (&data, notify_data_free);
}


void
dia_test_property_notify (gpointer object, const char *property, ...)
{
  GValue value_a = G_VALUE_INIT;
  GValue value_b = G_VALUE_INIT;
  char *error = NULL;
  GParamSpec *spec;
  va_list args;

  va_start (args, property);

  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                       property);
  g_assert_nonnull (spec);

  G_VALUE_COLLECT_INIT (&value_a, spec->value_type, args, 0, &error);
  g_assert_cmpstr (error, ==, NULL);

  G_VALUE_COLLECT_INIT (&value_b, spec->value_type, args, 0, &error);
  g_assert_cmpstr (error, ==, NULL);

  dia_expect_property_notify (object, property);
  g_object_set_property (object, property, &value_a);
  dia_assert_expected_notifies (object);

  dia_expect_no_notify (object);
  g_object_set_property (object, property, &value_a);
  dia_assert_expected_notifies (object);

  dia_expect_property_notify (object, property);
  g_object_set_property (object, property, &value_b);
  dia_assert_expected_notifies (object);

  va_end (args);

  g_value_unset (&value_a);
  g_value_unset (&value_b);
  g_clear_pointer (&error, g_free);
}
