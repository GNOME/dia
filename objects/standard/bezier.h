#ifndef _BEZIERLINE_H_
#define _BEZIERLINE_H_

#include "intl.h"
#include "object.h"
#include "bezier_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "diamenu.h"
#include "message.h"
#include "properties.h"
#include "create.h"

typedef struct _Bezierline Bezierline;

struct _Bezierline {
  BezierConn bez;

  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  Arrow start_arrow, end_arrow;
  real absolute_start_gap, absolute_end_gap;
};

extern ObjectTypeOps bezierline_type_ops;

DiaObject *bezierline_create(Point *startpoint,
                             void *user_data,
                             Handle **handle1,
                             Handle **handle2);

#endif /* _BEZIERLINE_H_ */
