#ifndef DIA_ENUMS_H
#define DIA_ENUMS_H

typedef enum {
  LINECAPS_DEFAULT = -1, /* default usually butt, this is unset */
  LINECAPS_BUTT = 0,
  LINECAPS_ROUND,
  LINECAPS_PROJECTING
} LineCaps;

typedef enum {
  LINEJOIN_DEFAULT = -1, /* default usually miter, this is unset */
  LINEJOIN_MITER = 0,
  LINEJOIN_ROUND,
  LINEJOIN_BEVEL
} LineJoin;

typedef enum {
  LINESTYLE_DEFAULT = -1, /* default usually solid, this is unset */
  LINESTYLE_SOLID = 0,
  LINESTYLE_DASHED,
  LINESTYLE_DASH_DOT,
  LINESTYLE_DASH_DOT_DOT,
  LINESTYLE_DOTTED
} LineStyle;

typedef enum {
  FILLSTYLE_SOLID
} FillStyle;

typedef enum {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
} Alignment;

typedef enum {
  TEXTFIT_NEVER,
  TEXTFIT_WHEN_NEEDED,
  TEXTFIT_ALWAYS
} TextFitting;

/* Used to be in widgets.h polluting a lot of object implementations */
#define DEFAULT_LINESTYLE LINESTYLE_SOLID
#define DEFAULT_LINESTYLE_DASHLEN 1.0

#endif
