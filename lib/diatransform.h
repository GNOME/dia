#ifndef DIA_TRANSFORM_H
#define DIA_TRANSFORM_H

#include "diatypes.h"
#include <glib-object.h>
#include "geometry.h"

G_BEGIN_DECLS

#define DIA_TYPE_TRANSFORM           (dia_transform_get_type ())
#define DIA_TRANSFORM(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_TRANSFORM, DiaTransform))
#define DIA_TRANSFORM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_TRANSFORM, DiaTransformClass))
#define DIA_IS_TRANSFORM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_TRANSFORM))
#define DIA_TRANSFORM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_TRANSFORM, DiaTransformClass))

GType dia_transform_get_type (void) G_GNUC_CONST;

DiaTransform *dia_transform_new (Rectangle *rect, real* zoom);
real dia_transform_length (DiaTransform *transform, real len);
void dia_transform_coords (DiaTransform *transform, coord x, coord y, int *xi, int *yi);
void dia_transform_coords_double (DiaTransform *transform, coord x, coord y, double *xd, double *yd);
real dia_untransform_length(DiaTransform *t, real len);

G_END_DECLS

#endif /* DIA_TRANSFORM_H */
