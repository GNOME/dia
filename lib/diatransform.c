
#include "diatransform.h"

typedef struct _DiaTransformClass DiaTransformClass;

struct _DiaTransform
{
  GObject parent_instance;
  /*< private >*/
  Rectangle *visible; /* pointer to original rectangle for transform_coords */
  real      *factor;  /* pointer to original factor for transform_length */
};

struct _DiaTransformClass
{
  GObjectClass parent_class;
};

static void dia_transform_class_init (DiaTransformClass *klass);

static gpointer parent_class = NULL;

GType
dia_transform_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaTransformClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dia_transform_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaTransform),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "DiaTransform",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
dia_transform_finalize (GObject *object)
{
  DiaTransform *dia_transform = DIA_TRANSFORM (object);

  /* don't free the fields, we don't own them */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dia_transform_class_init (DiaTransformClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaTransformClass *dia_transform_class = DIA_TRANSFORM_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = dia_transform_finalize;

}

DiaTransform *
dia_transform_new (Rectangle *rect, real* zoom)
{
  DiaTransform *t = g_object_new (DIA_TYPE_TRANSFORM, NULL);
  t->visible = rect;
  t->factor = zoom;

  return t;
}

real 
dia_transform_length (DiaTransform *t, real len)
{
  g_return_val_if_fail (DIA_IS_TRANSFORM (t), len);
  g_return_val_if_fail (t != NULL && *t->factor != 0.0, len);

  return (len * *(t->factor));
}

void 
dia_transform_coords (DiaTransform *t, 
                      coord x, coord y, 
                      int *xi, int *yi)
{
  g_return_if_fail (DIA_IS_TRANSFORM (t));
  g_return_if_fail (t != NULL && t->factor != NULL);

  *xi = ROUND ( (x - t->visible->left) * *(t->factor));
  *yi = ROUND ( (y - t->visible->top) * *(t->factor));
}


