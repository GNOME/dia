/*
create_diagram_tree_window
diagram_tree
diagtree_show_callback
 */

#include "config.h"

#include <glib-object.h>

#include "dia-application.h"
#include "display.h"

#include <lib/diamarshal.h>

enum {
  DIAGRAM_ADD,
  DIAGRAM_CHANGE,
  DIAGRAM_REMOVE,

  DISPLAY_ADD,
  DISPLAY_CHANGE,
  DISPLAY_REMOVE,

  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0, };


/**
 * DiaApplication:
 *
 * The central place managing global state changes like (dis-)appearance
 * of diagrams
 */
struct _DiaApplication
{
  GObject parent;

  GListStore *diagrams;
};

G_DEFINE_TYPE (DiaApplication, dia_application, G_TYPE_OBJECT);

static void
dia_application_class_init (DiaApplicationClass *klass)
{
  signals[DIAGRAM_ADD] =
    g_signal_new ("diagram-add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  dia_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  DIA_TYPE_DIAGRAM);

  signals[DIAGRAM_CHANGE] =
    g_signal_new ("diagram-change",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  dia_marshal_VOID__OBJECT_UINT_POINTER,
                  G_TYPE_NONE,
                  3,
                  DIA_TYPE_DIAGRAM,
                  G_TYPE_UINT,
                  G_TYPE_POINTER);

  signals[DIAGRAM_REMOVE] =
    g_signal_new ("diagram-remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  dia_marshal_VOID__OBJECT,
                  G_TYPE_NONE,
                  1,
                  DIA_TYPE_DIAGRAM);
}

static void
dia_application_init (DiaApplication *app)
{
  app->diagrams = g_list_store_new (DIA_TYPE_DIAGRAM);
}

/* ensure the singleton is available */
DiaApplication *
dia_application_get_default (void)
{
  static DiaApplication *instance;

  if (instance == NULL) {
    instance = g_object_new (DIA_TYPE_APPLICATION, NULL);
    g_object_add_weak_pointer (G_OBJECT (instance), (gpointer *) &instance);
  }

  return instance;
}

void
dia_application_diagram_add (DiaApplication *app,
                             Diagram        *dia)
{
  g_return_if_fail (DIA_IS_APPLICATION (app));
  g_return_if_fail (DIA_IS_DIAGRAM (dia));

  g_list_store_append (app->diagrams, dia);

  g_signal_emit (app, signals[DIAGRAM_ADD], 0, dia);
}


void
dia_application_diagram_remove (DiaApplication *app,
                                Diagram        *dia)
{
  int i = 0;

  g_return_if_fail (DIA_IS_APPLICATION (app));
  g_return_if_fail (DIA_IS_DIAGRAM (dia));

  i = dia_application_diagram_index (app, dia);
  g_list_store_remove (app->diagrams, i);

  g_signal_emit (app, signals[DIAGRAM_REMOVE], 0, dia);
}


int
dia_application_diagram_index (DiaApplication *self,
                               Diagram        *dia)
{
  Diagram *item = NULL;
  int i = 0;

  g_return_val_if_fail (DIA_IS_APPLICATION (self), -1);
  g_return_val_if_fail (DIA_IS_DIAGRAM (dia), -1);

  while ((item = g_list_model_get_item (G_LIST_MODEL (self->diagrams), i))) {
    if (item == dia) {
      g_clear_object (&item);

      return i;
    }

    g_clear_object (&item);

    i++;
  }

  return -1;
}


GListModel *
dia_application_get_diagrams (DiaApplication *app)
{
  g_return_val_if_fail (DIA_IS_APPLICATION (app), NULL);

  return G_LIST_MODEL (app->diagrams);
}

void
dia_application_diagram_change (DiaApplication *app,
                                Diagram        *dia,
                                guint           flags,
                                gpointer        object)
{
  g_signal_emit (app, signals[DIAGRAM_CHANGE], 0, dia, flags, object);
}
