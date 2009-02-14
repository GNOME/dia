#ifndef _WMF_GDI_H_
#define _WMF_GDI_H_

#include <stdio.h>
#include <glib.h>

typedef enum
{
  HOLLOW_BRUSH = 5
} eGdiBrush;

typedef enum
{
  PS_SOLID = 0,
  PS_DASH,
  PS_DOT,
  PS_DASHDOT,
  PS_DASHDOTDOT,
  PS_NULL,
  PS_INSIDEFRAME,
  PS_USERSTYLE,
  /* ... */
  PS_STYLE_MASK = 0x000F,

  PS_ENDCAP_ROUND  = (0<<8),
  PS_ENDCAP_SQUARE = (1<<8),
  PS_ENDCAP_FLAT   = (2<<8),
  PS_ENDCAP_MASK   = (0xF<<8),

  PS_JOIN_ROUND = (0<<12),
  PS_JOIN_BEVEL = (1<<12),
  PS_JOIN_MITER = (2<<12),
  PS_JOIN_MASK  = (0xF<<12),

  PS_COSMETIC = (0<<16),
  PS_GEOMETRIC = (1<<16),
  PS_TYPE_MASK = (0xF<<16) 
} ePenStyle;

typedef enum
{
  BS_SOLID = 0,
  BS_NULL
  /* ... */
} eBrushStyle;

typedef enum
{
  FW_DONTCARE   = 0,
  FW_THIN       = 100,
  FW_EXTRALIGHT = 200,
  FW_ULTRALIGHT = FW_EXTRALIGHT,
  FW_LIGHT      = 300,
  FW_NORMAL     = 400,
  FW_MEDIUM     = 500,
  FW_SEMIBOLD   = 600,
  FW_DEMIBOLD   = FW_SEMIBOLD,
  FW_BOLD       = 700,
  FW_EXTRABOLD  = 800,
  FW_ULTRABOLD  = FW_EXTRABOLD,
  FW_HEAVY      = 900
} eFontWeight;

typedef enum
{
  TA_LEFT   = 0,
  TA_TOP    = 0,
  TA_RIGHT  = 2,
  TA_CENTER = 6,
  TA_BOTTOM = 8,
  TA_BASELINE = 24
} eTextAlign;

typedef enum
{
  ANSI_CHARSET    = 0,
  DEFAULT_CHARSET = 1
} eCharSet;

typedef enum
{
  OUT_DEFAULT_PRECIS = 0,
  OUT_STRING_PRECIS,
  OUT_CHARACTER_PRECIS,
  OUT_STROKE_PRECIS,
  OUT_TT_PRECIS,
  OUT_DEVICE_PRECIS,
  OUT_RASTER_PRECIS,
  /* ... */
} eOutPrecision;

typedef enum
{
  CLIP_DEFAULT_PRECIS = 0
  /* ... */
} eClipPrecision;

typedef enum
{
  DEFAULT_QUALITY = 0,
  DRAFT_QUALITY,
  PROOF_QUALITY
} eQuality;

typedef enum
{
  DEFAULT_PITCH = 0,
  FIXED_PITCH,
  VARIABLE_PITCH
} ePitch;

typedef enum
{
  NULL_PEN = 8
} eStockObject;

typedef enum
{
  TRANSPARENT = 1,
  OPAQUE      = 2
} eBkMode;

typedef enum
{
  MM_TEXT = 1
} eMapMode;

typedef enum
{
  HORZSIZE = 4,
  VERTSIZE = 6,
  HORZRES  = 8,
  VERTRES  = 10,
  // some more constants not useful without a GDI printer (driver)
  PHYSICALWIDTH = 110,
  PHYSICALHEIGHT = 111,
  PHYSICALOFFSETX = 112,
  PHYSICALOFFSETY = 113
} eDeviceCaps;

namespace W32
{

typedef gulong COLORREF;
typedef guint16 WORD;
typedef guint32 DWORD;
typedef gboolean BOOL;
typedef guint UINT;
typedef guint8 BYTE;

typedef const char* LPCTSTR;
typedef const gunichar2* LPCWSTR;

typedef guint16 wmfint;

typedef struct
{
  wmfint x, y;
} POINT, * LPPOINT;

typedef struct
{
  wmfint top, left, right, bottom;
} RECT;

typedef struct _WindowsMetaHeader
{
  WORD  FileType;       /* Type of metafile (0=memory, 1=disk) */
  WORD  HeaderSize;     /* Size of header in WORDS (always 9) */
  WORD  Version;        /* Version of Microsoft Windows used */
  DWORD FileSize;       /* Total size of the metafile in WORDs */
  WORD  NumOfObjects;   /* Number of objects in the file */
  DWORD MaxRecordSize;  /* The size of largest record in WORDs */
  WORD  NumOfParams;    /* Not Used (always 0) */
} WMFHEAD;

typedef enum
{
  GDI_PEN = 1,
  GDI_BRUSH,
  GDI_FONT,
  GDI_STOCK
} eGdiType;

typedef struct
{
  wmfint Style, Width;
  COLORREF Color;
} GDI_Pen;

typedef struct
{
  wmfint Style;
  COLORREF Color;
} GDI_Brush;

typedef struct
{
  wmfint lbStyle;
  COLORREF lbColor;
  DWORD lbHatch;
} LOGBRUSH;

typedef struct
{
  wmfint Width, Height;
  wmfint Escapement, Orientation;
  wmfint Weight;

  DWORD dwItalic;
  DWORD dwUnderline;
  DWORD dwStrikeOut;
  DWORD dwCharSet;
  DWORD dwOutPrecision;
  DWORD dwClipPrecision;
  DWORD dwQuality;
  DWORD dwPitchAndFamily;

  char* sFaceName;
} GDI_Font;

typedef struct _GdiObject* HGDIOBJ;

typedef struct
{
  wmfint  Nr;
  HGDIOBJ hobj;
} GDI_Stock;

typedef struct _GdiObject
{
  eGdiType Type;
  union
  {
    GDI_Pen   Pen;
    GDI_Brush Brush;
    GDI_Font  Font;
    GDI_Stock Stock;
  };
} * HBRUSH, * HPEN, * HFONT;

typedef struct _MetaFileDeviceContext
{
  FILE* file;

  POINT actPos;

  HGDIOBJ hPen;
  HGDIOBJ hBrush;
  HGDIOBJ hFont;
} * HDC;

HDC GetDC(void* hwnd);

BOOL
ReleaseDC(void* hwnd, HDC);

HGDIOBJ
SelectObject(HDC hdc, HGDIOBJ hobj);

BOOL
DeleteObject(HGDIOBJ hobj);

HGDIOBJ
GetStockObject(int iObj);

int
GetDeviceCaps(HDC, int);

HBRUSH
CreateSolidBrush(COLORREF color);

HPEN
CreatePen(wmfint iStyle, wmfint iWidth, COLORREF color);

HPEN
ExtCreatePen(wmfint iStyle, wmfint iWidth, LOGBRUSH*, DWORD, DWORD*);

HFONT
CreateFont(int iWidth, int iHeight, int iEscapement, int iOrientation, int iWeight,
           DWORD dwItalic, DWORD dwUnderline, DWORD dwStrikeOut, 
           DWORD dwCharSet, DWORD dwOutPrecision, DWORD dwClipPrecision, 
           DWORD dwQuality, DWORD dwPitchAndFamily,
           const char* sFaceName);

HDC
CreateEnhMetaFile(HDC hdcRef, const char* sName, RECT* pbox, const char* sDesc);

typedef void* HENHMETAFILE;

HENHMETAFILE
CloseEnhMetaFile(HDC hdc);

BOOL
DeleteEnhMetaFile(HENHMETAFILE hemf);

BOOL
SetTextAlign(HDC hdc, wmfint iMode);

COLORREF
SetTextColor(HDC hdc, COLORREF color);

BOOL
MoveToEx(HDC hdc, wmfint x, wmfint y, LPPOINT ppt);

BOOL
LineTo(HDC hdc, wmfint x, wmfint y);

BOOL
Polyline(HDC hdc, LPPOINT ppts, wmfint iNum);

BOOL
Polygon(HDC hdc, LPPOINT ppts, wmfint iNum);

BOOL
Rectangle(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom);

BOOL
RoundRect(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom, wmfint iWidth, wmfint iHeight);

BOOL
Arc(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom,
    wmfint iStartX, wmfint iStartY, wmfint iEndX, wmfint iEndY);

BOOL
Pie(HDC, wmfint, wmfint, wmfint, wmfint, wmfint, wmfint, wmfint, wmfint);

BOOL
Ellipse(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom);

BOOL
PolyBezier(HDC hdc, LPPOINT ppts, int iNum);

BOOL
TextOut(HDC hdc, wmfint iX, wmfint iY, const char* s, wmfint iNumChars);

BOOL
TextOutW(HDC hdc, wmfint iX, wmfint iY, const gunichar2* s, long iNumChars);

UINT
GetACP();

wmfint
SetBkMode(HDC hdc, wmfint m);

wmfint
SetMapMode(HDC hdc, wmfint m);

wmfint
IntersectClipRect(HDC hdc, wmfint d, wmfint c, wmfint b, wmfint a);

/* This is not really part of GDI, but advanced GDI programming must make
 * use of the windoze versions, cause win9x GDI is 16 bit only and highly limited ...
 */
DWORD GetVersion ();

} /* namespace W32 */

#endif /* _WMF_GDI_H_ */
