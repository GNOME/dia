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
  PS_DASHDOTDOT
} ePenStyle;

typedef enum
{
  FW_DONTCARE   = 0,
  FW_THIN       = 100,
  FW_EXTRALIGHT = 200,
  FW_LIGHT      = 300,
  FW_NORMAL     = 400,
  FW_MEDIUM     = 500,
  FW_SEMIBOLD   = 600,
  FW_DEMIBOLD = FW_SEMIBOLD,
  FW_BOLD       = 700,
  FW_EXTRABOLD  = 800,
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

namespace W32
{

typedef gulong COLORREF;
typedef guint16 WORD;
typedef guint32 DWORD;
typedef gboolean BOOL;
typedef guint UINT;
typedef guint8 BYTE;

typedef char* LPCTSTR;

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
  GDI_FONT
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
typedef struct _GdiObject
{
  eGdiType Type;
  union
  {
    GDI_Pen   Pen;
    GDI_Brush Brush;
    GDI_DiaFont  Font;
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

HGDIOBJ
SelectObject(HDC hdc, HGDIOBJ hobj);

BOOL
DeleteObject(HGDIOBJ hobj);

HGDIOBJ
GetStockObject(int iObj);

HBRUSH
CreateSolidBrush(COLORREF color);

HPEN
CreatePen(wmfint iStyle, wmfint iWidth, COLORREF color);

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
Arc(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom,
    wmfint iStartX, wmfint iStartY, wmfint iEndX, wmfint iEndY);

BOOL
Ellipse(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom);

BOOL
PolyBezier(HDC hdc, LPPOINT ppts, int iNum);

BOOL
TextOut(HDC hdc, wmfint iX, wmfint iY, const char* s, wmfint iNumChars);

} // namespace W32

#endif // _WMF_GDI_H_
