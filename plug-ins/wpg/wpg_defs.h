/*
 * wpg_defs.h - WordPerfect Graphics Metafile Definitions
 *
 * Based on: "Encyclopedia of Graphics File Formats"
 *       by James D. Murray, William vanRyper, Deborah Russel
 *
 * Translated to "C" by Hans Breuer <Hans@Breuer.Org>
 */

typedef struct
{
  guint8  Type;
  guint8  Color;
  guint16  Width;
}
WPGLineAttr;

typedef struct
{
  guint8  Type;
  guint8  Color;
}
WPGFillAttr;

typedef struct
{
  guint16  Width;
  guint16  Height;
  guint8  Reserved[10];

  guint16 Font;
  guint8  Reserved2;

  guint8  XAlign;
  guint8  YAlign;

  guint8  Color;
  gint16  Angle;
}
WPGTextStyle;

typedef struct
{
  guint8  Version;
  guint8  Flag;
  guint16  Width; 
  guint16  Height;
}
WPGStartData;

typedef struct
{
  guint16  x, y;   /* center */
  guint16  rx, ry; /* radius */

  guint16  RotAngle; /* rotation */
  guint16  StartAngle, EndAngle; /* 0 - 360° */
  guint16 Flags;
}
WPGEllipse;

typedef struct
{
  guint16 Width;
  guint16 Height;
  guint16 BitsPerPixel;
  guint16 Xdpi;
  guint16 Ydpi;
}
WPGBitmap1;

typedef struct
{
  guint16 Angle;
  guint16 Left;
  guint16 Bottom;
  guint16 Right;
  guint16 Top;
  guint16 Width;
  guint16 Height;
  guint16 Depth;
  guint16 Xdpi;
  guint16 Ydpi;
}
WPGBitmap2;

typedef struct
{
  guint8  Type;
  guint8  Size;
}
WPGHead8;

typedef struct
{
  guint8  Type;
  guint8  Dummy;
  guint16 Size;
}
WPGHead16;

typedef struct
{
  guint8  Type;
  guint8  Dummy;
  guint32 Size;
}
WPGHead32;

typedef struct
{
  guint16 x;
  guint16 y;
}
WPGPoint;

typedef enum 
{
  WPG_FILLATTR = 1,
  WPG_LINEATTR = 2,
  WPG_MARKERATTR = 3,
  WPG_POLYMARKER = 4,
  WPG_LINE = 5,
  WPG_POLYLINE = 6,
  WPG_RECTANGLE = 7,
  WPG_POLYGON = 8,
  WPG_ELLIPSE = 9,
  WPG_BITMAP1 = 11,
  WPG_TEXT = 12,
  WPG_TEXTSTYLE = 13,
  WPG_COLORMAP = 14,
  WPG_START = 15,
  WPG_END = 16,
  WPG_POSTSCRIPT1 = 17,
  WPG_OUTPUTATTR = 18,
  WPG_POLYCURVE = 19,
  WPG_BITMAP2 = 20,
  WPG_STARTFIGURE = 21,
  WPG_STARTCHART = 22
  /* ... */
} WPG_Type;

typedef enum
{
  WPG_FA_HOLLOW = 0,
  WPG_FA_SOLID = 1
  /* ... */
} WPG_FillAttr;

typedef enum
{
  WPG_LA_NONE = 0,
  WPG_LA_SOLID,
  WPG_LA_LONGDASH,
  WPG_LA_DOTS,
  WPG_LA_DASHDOT,
  WPG_LA_MEDIUMDASH,
  WPG_LA_DASHDOTDOT,
  WPG_LA_SHORTDASH
} WPG_LineAttr;
