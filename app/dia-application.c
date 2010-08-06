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

static guint _dia_application_signals[LAST_SIGNAL] = { 0, };

typedef struct _DiaApplicationClass DiaApplicationClass;
struct _DiaApplicationClass
{
  GObjectClass parent_class;
  
  /* signalsing global state changes */
  void (*diagram_add)    (DiaApplication *app, Diagram *diagram);
  void (*diagram_change) (DiaApplication *app, Diagram *diagram, guint flags);
  void (*diagram_remove) (DiaApplication *app, Diagram *diagram);
  
  void (*display_add)    (DiaApplication *app, DDisplay *display);
  void (*display_change) (DiaApplication *app, DDisplay *display, guint flags);
  void (*display_remove) (DiaApplication *app, DDisplay *display);
};

/**
 * The central place managing global state changes like (dis-)appearance of diagrams
 */
struct _DiaApplication
{
  GObject parent;
};

GType dia_application_get_type (void);
#define DIA_TYPE_APPLICATION (dia_application_get_type ())

G_DEFINE_TYPE (DiaApplication, dia_application, G_TYPE_OBJECT);

static void
dia_application_class_init (DiaApplicationClass *klass)
{
  G_GNUC_UNUSED GObjectClass *object_class = G_OBJECT_CLASS (klass);
  
  _dia_application_signals[DIAGRAM_ADD] =
    g_signal_new ("diagram_add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (DiaApplicationClass, diagram_add),
                  NULL, NULL,
                  dia_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 
                  1,
		  DIA_TYPE_DIAGRAM);
  _dia_application_signals[DIAGRAM_CHANGE] =
    g_signal_new ("diagram_change",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (DiaApplicationClass, diagram_change),
                  NULL, NULL,
                  dia_marshal_VOID__OBJECT_UINT_POINTER,
                  G_TYPE_NONE, 
                  3,
		  DIA_TYPE_DIAGRAM, G_TYPE_UINT, G_TYPE_POINTER);
  _dia_application_signals[DIAGRAM_REMOVE] =
    g_signal_new ("diagram_remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (DiaApplicationClass, diagram_remove),
                  NULL, NULL,
                  dia_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 
                  1,
		  DIA_TYPE_DIAGRAM);
}

static void 
dia_application_init (DiaApplication *app)
{
}

static DiaApplication *_app = NULL;

/* ensure the singleton is available */
DiaApplication *
dia_application_get (void)
{
  
  if (!_app)
    _app = g_object_new (DIA_TYPE_APPLICATION, NULL);
  
  return _app;
}

void
dia_diagram_add (Diagram *dia)
{
  if (_app)
    g_signal_emit (_app, _dia_application_signals[DIAGRAM_ADD], 0, dia);
}
void
dia_diagram_remove (Diagram *dia)
{
  if (_app)
    g_signal_emit (_app, _dia_application_signals[DIAGRAM_REMOVE], 0, dia);
}

void 
dia_diagram_change (Diagram *dia, guint flags, gpointer object)
{
  if (_app)
    g_signal_emit (_app, _dia_application_signals[DIAGRAM_CHANGE], 0, dia, flags, object);
}
