/*
 * WMF_GDI is an implementation of Windoze GDI functions required
 * for saving of Windows Meta Files, if the Win32 API isn't 
 * available. It isn't finished yet but shoud be easily extendable
 * for someone interested in Dia WMF support on non Windoze 
 * platforms.
 *
 * (c) 2000 Hans Breuer <Hans@Breuer.Org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

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
  case GDI_STOCK :
    return SelectObject(hdc, hobj->Stock.hobj);
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
  else if (GDI_STOCK == hobj->Type)
    DeleteObject(hobj->Stock.hobj);

  g_free(hobj);

  return TRUE;
}

HGDIOBJ
GetStockObject(int iObj)
{
  HGDIOBJ hobj;

  hobj = g_new0(struct _GdiObject, 1);
  hobj->Type = GDI_STOCK;
  hobj->Stock.Nr = iObj;
  
  switch (iObj)
    {
    case NULL_PEN :
      hobj->Stock.hobj = CreatePen (0,0,0);
      break;
    case HOLLOW_BRUSH :
      hobj->Stock.hobj = CreateSolidBrush(0);
      break;
    default :
      g_assert_not_reached ();
    }
  return hobj;
}

int
GetDeviceCaps(HDC hdc, int cap)
{
  //dummy implementation should be good enough?
  switch (cap)
  {
  case HORZSIZE:
    return 1024;
  case VERTSIZE:
    return 768;
  case HORZRES:
    return 96;
  case VERTRES:
    return 96;
  }
  return 0;
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

HPEN
ExtCreatePen(wmfint iStyle, wmfint iWidth, LOGBRUSH* lb, DWORD nDashes, DWORD* pDashes)
{
  //TODO : support the advanced style, e.g. PS_USERSTYLE 
  return CreatePen (iStyle, iWidth, lb->lbColor);
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
RoundRect(HDC hdc, wmfint iLeft, wmfint iTop, wmfint iRight, wmfint iBottom, wmfint iWidth, wmfint iHeight)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x061C, 6);
  fwrite_le(&iHeight, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&iWidth, sizeof(wmfint), 1, hdc->file);
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
Pie(HDC hdc, wmfint h, wmfint g, wmfint f, wmfint e, wmfint d, wmfint c, wmfint b, wmfint a)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x081A, 8);

  // dump params from right to left
  fwrite_le(&a, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&b, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&c, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&d, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&e, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&f, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&g, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&h, sizeof(wmfint), 1, hdc->file);

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

BOOL
TextOutW(HDC hdc, wmfint iX, wmfint iY, const gunichar2* s, long iNumChars)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  g_warning ("TextOutW not implemented");

  return TRUE;
}


HDC GetDC(void* hwnd)
{
  return NULL; 
}

BOOL ReleaseDC (void* hwnd, HDC hdc)
{
  return FALSE;
}

UINT
GetACP() 
{ 
  //FIXME: always  Latin1 (but what to doe elese ??)
  return 1252; 
}

wmfint
SetBkMode(HDC hdc, wmfint m)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x0102, 1);
  fwrite_le(&m, sizeof(wmfint), 1, hdc->file);
  return 0;
}

wmfint
SetMapMode(HDC hdc, wmfint m)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x0103, 1);
  fwrite_le(&m, sizeof(wmfint), 1, hdc->file);
  return 0;
}

wmfint
IntersectClipRect(HDC hdc, wmfint d, wmfint c, wmfint b, wmfint a)
{
  g_return_val_if_fail(hdc != NULL, FALSE);

  WriteRecHead(hdc, 0x0416, 4);
  fwrite_le(&a, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&b, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&c, sizeof(wmfint), 1, hdc->file);
  fwrite_le(&d, sizeof(wmfint), 1, hdc->file);
  return 0;
}

DWORD
GetVersion()
{
  //FIXME : this claims kind of win95
  return 0x80000000;
}

} // eof namespace
