#ifndef _ZIGZAGLINE_H_
#define _ZIGZAGLINE_H_

#include "intl.h"
#include "object.h"
#include "orth_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "properties.h"
#include "autoroute.h"

typedef struct _Zigzagline {
  OrthConn orth;

  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  real corner_radius;
  Arrow start_arrow, end_arrow;
} Zigzagline;

extern ObjectTypeOps zigzagline_type_ops;

DiaObject *zigzagline_create(Point *startpoint,
                             void *user_data,
                             Handle **handle1,
                             Handle **handle2);

#endif /* _ZIGZAGLINE_H_ */
