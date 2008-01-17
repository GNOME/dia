#ifndef _POLYLINE_H_
#define _POLYLINE_H_

#include "intl.h"
#include "object.h"
#include "poly_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "diamenu.h"
#include "message.h"
#include "properties.h"


typedef struct _Polyline {
  PolyConn poly;

  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  real corner_radius;
  Arrow start_arrow, end_arrow;
  real absolute_start_gap, absolute_end_gap;
} Polyline;

extern ObjectTypeOps polyline_type_ops;

DiaObject *polyline_create(Point *startpoint,
                           void *user_data,
                           Handle **handle1,
                           Handle **handle2);
                           
#endif /* _POLYLINE_H_ */
