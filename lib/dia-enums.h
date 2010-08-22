#ifndef DIA_ENUMS_H
#define DIA_ENUMS_H

typedef enum {
  LINECAPS_BUTT,
  LINECAPS_ROUND,
  LINECAPS_PROJECTING
} LineCaps;

typedef enum {
  LINEJOIN_MITER,
  LINEJOIN_ROUND,
  LINEJOIN_BEVEL
} LineJoin;

typedef enum {
  LINESTYLE_SOLID,
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
#endif
