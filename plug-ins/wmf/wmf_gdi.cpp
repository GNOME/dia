#include "wmf_gdi.h"

namespace W32
{

/*
 * Private Helpers
 */
#define fwrite_le(a,b,c,d) fwrite(a,b,c,d)

static void
WriteRecHead(HDC hdc, int iFn, int iNumPara)
{
  DWORD dwSize = 3 + iNumPara; /* in WORDs ! */
  guint16 fn = (guint16)iFn;

  fwrite_le(&dwSize, sizeof(DWORD), 1, hdc->file);
  fwrite_le(&fn, sizeof(guint16), 1, hdc->file);
}

/*
 * "Exported" functions
 */
HGDIOBJ
SelectObject(HDC hdc, HGDIOBJ hobj)
{
  HGDIOBJ hRet;

  g_return_val_if_fail(hdc != NULL, NULL);
  g_return_val_if_fail(hobj != NULL, NULL);

  switch (hobj->Type)
  {
  case GDI_PEN :
    hRet = hdc->hPen;
    hdc->hPen = hobj;
    break;
  case GDI_BRUSH :
    hRet = hdc->hBrush;
    hdc->hBrush = hobj;
    break;
  case GDI_FONT :
    hRet = hdc->hFont;
    hdc->hFont = hobj;
    break;
  default :
    hRet = NULL;
    g_assert_not_reached();
  }

  return hRet;
}

BOOL
DeleteObject(HGDIOBJ hobj)
{
  if (GDI_FONT == hobj->Type)
    g_free(hobj->Font.sFaceName);

  g_free(hobj);

  return TRUE;
}

HGDIOBJ
GetStockObject(int iObj)
{
  return NULL;
}

HBRUSH
CreateSolidBrush(COLORREF color)
{
  HGDIOBJ hobj;

  hobj = g_new0(struct _GdiObject, 1);
  hobj->Type = GDI_BRUSH;
  hobj->Brush.Color = color;

  return hobj;
}

HPEN
CreatePen(wmfint iStyle, wmfint iWidth, COLORREF color)
{
  HGDIOBJ hobj;

  hobj = g_new0(struct _GdiObject, 1);
  hobj->Type = GDI_PEN;
  hobj->Pen.Color = color;
  hobj->Pen.Width = iWidth;
  hobj->Pen.Style = iStyle;

  return hobj;
}

HFONT
CreateFont(int iWidth, int iHeight, int iEscapement, int iOrientation, int iWeight,
           DWORD dwItalic, DWORD dwUnderline, DWORD dwStrikeOut, 
           DWORD dwCharSet, DWORD dwOutPrecision, DWORD dwClipPrecision, 
           DWORD dwQuality, DWORD dwPitchAndFamily,
           const char* sFaceName)
{
  HGDIOBJ hobj;

  hobj = g_new0(struct _GdiObject, 1);
  hobj->Type = GDI_FONT;

  hobj->Font.Width = iWidth;
  hobj->Font.Height = iHeight;
  hobj->Font.Escapement = iEscapement;
  hobj->Font.Orientation = iOrientation;
  hobj->Font.Weight = iWeight;

  hobj->Font.dwItalic = dwItalic;
  hobj->Font.dwUnderline = dwUnderline;
  hobj->Font.dwStrikeOut = dwStrikeOut;
  hobj->Font.dwCharSet = dwCharSet;
  hobj->Font.dwOutPrecision = dwOutPrecision;
  hobj->Font.dwClipPrecision = dwClipPrecision;
  hobj->Font.dwQuality = dwQuality;
  hobj->Font.dwPitchAndFamily = dwPitchAndFamily;

  hobj->Font.sFaceName = g_strdup(sFaceName);
                       
  /* ... */

  return hobj;
}

HDC
CreateEnhMetaFile(HDC hdcRef, const char* sName, RECT* pbox, const char* sDesc)
{
  HDC hdc;

  g_return_val_if_fail(NULL != sName, NULL);

  hdc = g_new0(struct _MetaFileDeviceContext, 1);

  hdc->file = fopen(sName, "wb"); /* write binary */

  return hdc;
}

HENHMETAFILE
CloseEnhMetaFile(HDC hdc)
{
  g_return_val_if_fail(NULL != hdc, NULL);

  WriteRecHead(hdc, 0x0, 0);

  fclose(hdc->file);
  g_free(hdc);

  return NULL;
}

BOOL
DeleteEnhMetaFile(HENHMETAFILE hemf)
{
  return TRUE;
}

BOOL
SetTextAlign(HDC hdc, wmfint iMode)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x012E, 1);
  fwrite_le(&iMode, sizeof(wmfint), 1, hdc->file);

  return TRUE;
}

COLORREF
SetTextColor(HDC hdc, COLORREF color)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x0209, 1);
  fwrite_le(&color, sizeof(COLORREF), 1, hdc->file);

  return TRUE;
}

/*
 * Drawing Functions
 */
BOOL
MoveToEx(HDC hdc, wmfint x, wmfint y, LPPOINT ppt)
{

  g_return_val_if_fail(hdc != NULL, FALSE);

  if (ppt)
    *ppt = hdc->actPos;

  hdc->actPos.x = x;
  hdc->actPos.y = y;

  WriteRecHead(hdc, 0x0214, 2);
  fwrite_le(&y, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&x, sizeof(wmfint), 1, hdc->file);

  return TRUE;
}

BOOL
LineTo(HDC hdc, wmfint x, wmfint y)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  hdc->actPos.x = x;
  hdc->actPos.y = y;

  WriteRecHead(hdc, 0x0213, 2);
  fwrite_le(&y, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&x, sizeof(wmfint), 1, hdc->file);

  return TRUE;
}

BOOL
Polyline(HDC hdc, LPPOINT ppts, wmfint iNum)
{
  int i;

  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x0325, 2*iNum);
  fwrite_le(&iNum, sizeof(wmfint), 1, hdc->file);
  for (i = iNum - 1; i > -1; i--)
  {
    fwrite_le(&(ppts[i].y), sizeof(wmfint), 1, hdc->file);
    fwrite_le(&(ppts[i].x), sizeof(wmfint), 1, hdc->file);
  }

  return TRUE;
}

BOOL
Polygon(HDC hdc, LPPOINT ppts, wmfint iNum)
{
  int i;

  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x0324, 2*iNum+1);
  fwrite_le(&iNum, sizeof(wmfint), 1, hdc->file);
  for (i = iNum - 1; i > -1; i--)
  {
    fwrite_le(&(ppts[i].y), sizeof(wmfint), 1, hdc->file);
    fwrite_le(&(ppts[i].x), sizeof(wmfint), 1, hdc->file);
  }

  return TRUE;
}

BOOL
Rectangle(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x041B, 4);
  fwrite_le(&iBottom, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iRight, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iTop, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iLeft, sizeof(wmfint), 1, hdc->file);

  return TRUE;
}

BOOL
Arc(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom,
    wmfint iStartX, wmfint iStartY, wmfint iEndX, wmfint iEndY)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x0817, 8);

  fwrite_le(&iEndY, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iEndX, sizeof(wmfint), 1, hdc->file);

  fwrite_le(&iStartY, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iStartX, sizeof(wmfint), 1, hdc->file);

  fwrite_le(&iBottom, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iRight, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iTop, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iLeft, sizeof(wmfint), 1, hdc->file);

  return TRUE;
}

BOOL
Ellipse(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x0418, 4);
  fwrite_le(&iBottom, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iRight, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iTop, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iLeft, sizeof(wmfint), 1, hdc->file);

  return TRUE;
}


BOOL
PolyBezier(HDC hdc, LPPOINT ppts, int iNum)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  /* ... */

  return TRUE;
}


BOOL
TextOut(HDC hdc, wmfint iX, wmfint iY, const char* s, wmfint iNumChars)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x0521, 3 + (iNumChars % 2) ? iNumChars / 2 : iNumChars / 2 + 1);

  fwrite_le(&iNumChars, sizeof(wmfint), 1, hdc->file);

  fwrite_le(s, 1, iNumChars, hdc->file);
  if (iNumChars % 2) // fill to WORD
    fwrite(s, 1, 1, hdc->file);

  fwrite_le(&iY, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iX, sizeof(wmfint), 1, hdc->file);

  return TRUE;
}

HDC GetDC(void* hwnd)
{
  return NULL; 
}

} // eof namespace
