/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * wmf.cpp -- Windows Metafile export plugin for dia
 * Copyright (C) 2000, Hans Breuer, <Hans@Breuer.Org>
 *   based on dummy plug-in. 
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "config.h"
#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "render.h"
#include "filter.h"
#include "plug-ins.h"

#ifdef __cplusplus
}
#endif

namespace W32 {
// at least Rectangle conflicts ...
#include <windows.h>
}

/* force linking with gdi32 */
#pragma comment( lib, "gdi32" )
 

// #define SAVE_EMF

/* --- the renderer --- */
#define MY_RENDERER_NAME "WMF"

typedef struct _MyRenderer MyRenderer;
struct _MyRenderer {
    Renderer renderer;

    W32::HDC  hFileDC;
    gchar*    sFileName;

    /* if applicable everything is scaled to 0.01 mm */
    int nLineWidth;  /* need to cache these, because ... */
    int fnPenStyle;  /* ... both are needed at the same time */
    W32::HPEN  hPen; /* ugliness by concept, see DonePen() */

    W32::HFONT hFont;
};

/* include function declares and render object "vtable" */
#include "../renderer.inc"

/*
 * helper macros
 */
#define W32COLOR(c) \
	 0xff * c->red + \
	((unsigned char)(0xff * c->green)) * 256 + \
	((unsigned char)(0xff * c->blue)) * 65536;

#define WMF_SCALE 100.0
#define SC(a) ((a)*WMF_SCALE)

/*
 * helper functions
 */
static W32::HGDIOBJ
UsePen(MyRenderer* renderer, Color* colour)
{
    W32::HPEN hOldPen;
    W32::COLORREF rgb = W32COLOR(colour);

    renderer->hPen = W32::CreatePen( renderer->fnPenStyle, 
						 renderer->nLineWidth,
						 rgb);
    hOldPen = W32::SelectObject(renderer->hFileDC, renderer->hPen);
    return hOldPen;
}

static void
DonePen(MyRenderer* renderer, W32::HPEN hPen)
{
    /* restore the OLD one ... */
    if (hPen) W32::SelectObject(renderer->hFileDC, hPen);
    /* ... before deleting the last active */
    if (renderer->hPen)
    {
	W32::DeleteObject(renderer->hPen);
	renderer->hPen = NULL;
    }
}

#define DIAG_NOTE my_log
void
my_log(MyRenderer* renderer, char* format, ...)
{
    gchar *string;
    va_list args;
  
    g_return_if_fail (format != NULL);
  
    va_start (args, format);
    string = g_strdup_vprintf (format, args);
    va_end (args);

    //fprintf(renderer->file, string);
    g_print(string);

    g_free(string);
}
 
/*
 * renderer interface implementation
 */
static void
begin_render(MyRenderer *renderer, DiagramData *data)
{
    DIAG_NOTE(renderer, "begin_render\n");

    /* make unfilled the default */
    W32::SelectObject(renderer->hFileDC, 
	W32::GetStockObject (HOLLOW_BRUSH) );
}

static void
end_render(MyRenderer *renderer)
{
    W32::HENHMETAFILE hEmf;
    W32::UINT nSize;
    W32::BYTE* pData = NULL;
    FILE* f;

    DIAG_NOTE(renderer, "end_render\n");
    hEmf = W32::CloseEnhMetaFile(renderer->hFileDC);

#ifndef SAVE_EMF
    /* get size */
    nSize = W32::GetWinMetaFileBits(hEmf, 0, NULL, MM_ANISOTROPIC, W32::GetDC(NULL));
    pData = g_new(W32::BYTE, nSize);
    /* get data */
    nSize = W32::GetWinMetaFileBits(hEmf, nSize, pData, MM_ANISOTROPIC, W32::GetDC(NULL));

    /* write file */
    f = fopen(renderer->sFileName, "wb");
    fwrite(pData,1,nSize,f);
    fclose(f);
    
    g_free(pData);
#endif
    g_free(renderer->sFileName);

    if (hEmf)
	W32::DeleteEnhMetaFile(hEmf);
    if (renderer->hFont)
	W32::DeleteObject(renderer->hFont);
}

static void
set_linewidth(MyRenderer *renderer, real linewidth)
{  
    DIAG_NOTE(renderer, "set_linewidth %f\n", linewidth);

    renderer->nLineWidth = SC(linewidth);
}

static void
set_linecaps(MyRenderer *renderer, LineCaps mode)
{
    DIAG_NOTE(renderer, "set_linecaps %d\n", mode);

    switch(mode) {
    case LINECAPS_BUTT:
	break;
    case LINECAPS_ROUND:
	break;
    case LINECAPS_PROJECTING:
	break;
    default:
	message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
    }
}

static void
set_linejoin(MyRenderer *renderer, LineJoin mode)
{
    DIAG_NOTE(renderer, "set_join %d\n", mode);

    switch(mode) {
    case LINEJOIN_MITER:
	break;
    case LINEJOIN_ROUND:
	break;
    case LINEJOIN_BEVEL:
	break;
    default:
	message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
    }
}

static void
set_linestyle(MyRenderer *renderer, LineStyle mode)
{
    DIAG_NOTE(renderer, "set_linestyle %d\n", mode);

    /* line type */
    switch (mode) {
    case LINESTYLE_SOLID:
      renderer->fnPenStyle = PS_SOLID;
      break;
    case LINESTYLE_DASHED:
      renderer->fnPenStyle = PS_DASH;
      break;
    case LINESTYLE_DASH_DOT:
      renderer->fnPenStyle = PS_DASHDOT;
      break;
    case LINESTYLE_DASH_DOT_DOT:
      renderer->fnPenStyle = PS_DASHDOTDOT;
      break;
    case LINESTYLE_DOTTED:
      renderer->fnPenStyle = PS_DOT;
      break;
    default:
	message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
    }
}

static void
set_dashlength(MyRenderer *renderer, real length)
{  
    DIAG_NOTE(renderer, "set_dashlength %f\n", length);

    /* dot = 20% of len */
}

static void
set_fillstyle(MyRenderer *renderer, FillStyle mode)
{
    DIAG_NOTE(renderer, "set_fillstyle %d\n", mode);

    switch(mode) {
    case FILLSTYLE_SOLID:
	break;
    default:
	message_error(MY_RENDERER_NAME ": Unsupported fill mode specified!\n");
    }
}

static void
set_font(MyRenderer *renderer, Font *font, real height)
{
    W32::LPCTSTR sFace;
    W32::DWORD dwItalic = 0;
    W32::DWORD dwWeight = FW_DONTCARE;

    DIAG_NOTE(renderer, "set_font %s %f\n", font->name, height);
    if (renderer->hFont)
	W32::DeleteObject(renderer->hFont);

    /* An ugly hack to map font names. It has to be
     * changed if Dia's fonts ever get changed.
     * See: lib/font.c
     *
     * Wouldn't it be better to provide this info in
     * a Font field ?
     */
    if (strstr(font->name, "Courier"))
	sFace = "Courier New";
    else if (strstr(font->name, "Times"))
	sFace = "Times New Roman";
    else
	sFace = "Arial";

    dwItalic = !!(    strstr(font->name, "Italic") 
		   || strstr(font->name, "Oblique")); //?
    if (strstr(font->name, "Bold"))
	dwWeight = FW_BOLD;
    else if (strstr(font->name, "Demi"))
	dwWeight = FW_DEMIBOLD;
    else if (strstr(font->name, "Light"))
	dwWeight = FW_LIGHT;

    renderer->hFont = (W32::HFONT)W32::CreateFont( 
	-(height * WMF_SCALE),  // logical height of font 
	0,			// logical average character width 
	0,			// angle of escapement
	0,			// base-line orientation angle 
	dwWeight,		// font weight
	dwItalic,		// italic attribute flag
	0,			// underline attribute flag
	0,			// strikeout attribute flag
	ANSI_CHARSET,		// character set identifier 
	OUT_TT_PRECIS, 		// output precision 
	CLIP_DEFAULT_PRECIS,	// clipping precision
	PROOF_QUALITY,		// output quality 
	DEFAULT_PITCH,		// pitch and family
	sFace);		// pointer to typeface name string 
}

static void
draw_line(MyRenderer *renderer, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
    W32::HGDIOBJ hPen;

    DIAG_NOTE(renderer, "draw_line %f,%f -> %f, %f\n", 
              start->x, start->y, end->x, end->y);

    hPen = UsePen(renderer, line_colour);

    W32::MoveToEx(renderer->hFileDC, SC(start->x), SC(start->y), NULL);
    W32::LineTo(renderer->hFileDC, SC(end->x), SC(end->y));

    DonePen(renderer, hPen);
}

static void
draw_polyline(MyRenderer *renderer, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
    W32::HGDIOBJ hPen;
    W32::POINT*  pts;
    int	    i;

    DIAG_NOTE(renderer, "draw_polyline n:%d %f,%f ...\n", 
              num_points, points->x, points->y);

    if (num_points < 2)
	return;
    pts = g_new (W32::POINT, num_points+1);
    for (i = 0; i < num_points; i++)
    {
	pts[i].x = SC(points[i].x);
	pts[i].y = SC(points[i].y);
    }

    hPen = UsePen(renderer, line_colour);
    W32::Polyline(renderer->hFileDC, pts, num_points);
    DonePen(renderer, hPen);

    g_free(pts);
}

static void
draw_polygon(MyRenderer *renderer, 
	     Point *points, int num_points, 
	     Color *line_colour)
{
    W32::HGDIOBJ hPen;
    W32::POINT*  pts;
    int	    i;

    DIAG_NOTE(renderer, "draw_polygon n:%d %f,%f ...\n", 
              num_points, points->x, points->y);

    if (num_points < 2)
	return;
    pts = g_new (W32::POINT, num_points+1);
    for (i = 0; i < num_points; i++)
    {
	pts[i].x = SC(points[i].x);
	pts[i].y = SC(points[i].y);
    }

    hPen = UsePen(renderer, line_colour);

    W32::Polygon(renderer->hFileDC, pts, num_points);

    DonePen(renderer, hPen);

    g_free(pts);
}

static void
fill_polygon(MyRenderer *renderer, 
	     Point *points, int num_points, 
	     Color *colour)
{
    W32::HBRUSH  hBrush, hBrOld;
    W32::COLORREF rgb = W32COLOR(colour);

    DIAG_NOTE(renderer, "fill_polygon n:%d %f,%f ...\n", 
              num_points, points->x, points->y);

    hBrush = W32::CreateSolidBrush(rgb);
    hBrOld = W32::SelectObject(renderer->hFileDC, hBrush);

    draw_polygon(renderer, points, num_points, colour);

    W32::SelectObject(renderer->hFileDC, 
    W32::GetStockObject (HOLLOW_BRUSH) );
    W32::DeleteObject(hBrush);
}

static void
draw_rect(MyRenderer *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  W32::HGDIOBJ hPen;

  DIAG_NOTE(renderer, "draw_rect %f,%f -> %f,%f\n", 
            ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y);

  hPen = UsePen(renderer, colour);

  W32::Rectangle(renderer->hFileDC,
  SC(ul_corner->x), SC(ul_corner->y),
  SC(lr_corner->x), SC(lr_corner->y));

  DonePen(renderer, hPen);
}

static void
fill_rect(MyRenderer *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
  W32::HGDIOBJ hBrush, hBrOld;
  W32::COLORREF rgb = W32COLOR(colour);

  DIAG_NOTE(renderer, "fill_rect %f,%f -> %f,%f\n", 
            ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y);

  hBrush = W32::CreateSolidBrush(rgb);
  hBrOld = W32::SelectObject(renderer->hFileDC, hBrush);

  draw_rect(renderer, ul_corner, lr_corner, colour);

  W32::SelectObject(renderer->hFileDC, 
  W32::GetStockObject (HOLLOW_BRUSH) );
  W32::DeleteObject(hBrush);
}

static void
draw_arc(MyRenderer *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  W32::HGDIOBJ hPen;
  W32::POINT ptStart, ptEnd;

  DIAG_NOTE(renderer, "draw_arc %fx%f <%f,<%f @%f,%f\n", 
            width, height, angle1, angle2, center->x, center->y);

  hPen = UsePen(renderer, colour);

  /* calculate start and end points of arc */
  ptStart.x = SC(center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle1));
  ptStart.y = SC(center->y - (height / 2.0) * sin((M_PI / 180.0) * angle1));
  ptEnd.x = SC(center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle2));
  ptEnd.y = SC(center->y - (height / 2.0) * sin((M_PI / 180.0) * angle2));

  W32::MoveToEx(renderer->hFileDC, ptStart.x, ptStart.y, NULL);
  W32::Arc(renderer->hFileDC,
  SC(center->x - width / 2), /* bbox corners */
  SC(center->y - height / 2),
  SC(center->x + width / 2), 
  SC(center->y + height / 2),
  ptStart.x, ptStart.y, ptEnd.x, ptEnd.y); 

  DonePen(renderer, hPen);
}

static void
fill_arc(MyRenderer *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
  W32::HGDIOBJ hPen;

  DIAG_NOTE(renderer, "fill_arc %fx%f <%f,<%f @%f,%f\n", 
            width, height, angle1, angle2, center->x, center->y);

  hPen = UsePen(renderer, colour);

  DonePen(renderer, hPen);
}

static void
draw_ellipse(MyRenderer *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  W32::HGDIOBJ hPen;

  DIAG_NOTE(renderer, "draw_ellipse %fx%f @ %f,%f\n", 
            width, height, center->x, center->y);

  hPen = UsePen(renderer, colour);

  W32::Ellipse(renderer->hFileDC,
  SC(center->x - width / 2), /* bbox corners */
  SC(center->y - height / 2),
  SC(center->x + width / 2), 
  SC(center->y + height / 2));

  DonePen(renderer, hPen);
}

static void
fill_ellipse(MyRenderer *renderer, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
  W32::HGDIOBJ hPen;
  W32::HGDIOBJ hBrush, hBrOld;
  W32::COLORREF rgb = W32COLOR(colour);

  DIAG_NOTE(renderer, "fill_ellipse %fx%f @ %f,%f\n", 
            width, height, center->x, center->y);

  hBrush = W32::CreateSolidBrush(rgb);
  hBrOld = W32::SelectObject(renderer->hFileDC, hBrush);

  draw_ellipse(renderer, center, width, height, colour);

  W32::SelectObject(renderer->hFileDC, 
  W32::GetStockObject (HOLLOW_BRUSH) );
  W32::DeleteObject(hBrush);
}

static void
draw_bezier(MyRenderer *renderer, 
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
  W32::HGDIOBJ hPen;
  W32::POINT * pts;
  int i;

  DIAG_NOTE(renderer, "draw_bezier n:%d %fx%f ...\n", 
            numpoints, points->p1.x, points->p1.y);

  pts = g_new(W32::POINT, (numpoints-1) * 3 + 1);

  pts[0].x = SC(points[0].p1.x);
  pts[0].y = SC(points[0].p1.y);

  for (i = 1; i < numpoints; i++)
  {
    switch(points[i].type)
    {
	case _BezPoint::BEZ_MOVE_TO:
        g_warning("only first BezPoint can be a BEZ_MOVE_TO");
	  break;
      case _BezPoint::BEZ_LINE_TO:
        /* everyhing the same ?*/
        pts[i*3-2].x = pts[i*3-1].x = 
        pts[i*3  ].x = SC(points[i].p1.x);
        pts[i*3-2].y = pts[i*3-1].y = 
        pts[i*3  ].y = SC(points[i].p1.y);
        break;
	case _BezPoint::BEZ_CURVE_TO:
        /* control points */
        pts[i*3-2].x = SC(points[i].p1.x);
        pts[i*3-2].y = SC(points[i].p1.y);
        pts[i*3-1].x = SC(points[i].p2.x);
        pts[i*3-1].y = SC(points[i].p2.y);
        /* end point */
        pts[i*3  ].x = SC(points[i].p3.x);
        pts[i*3  ].y = SC(points[i].p3.y);
	  break;
	default:
	  break;
	}
    }

    hPen = UsePen(renderer, colour);

    W32::PolyBezier(renderer->hFileDC,
	pts, (numpoints-1)*3+1);

    DonePen(renderer, hPen);

    g_free(pts);
}

static void
fill_bezier(MyRenderer *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *colour)
{
    W32::HGDIOBJ hPen;

    DIAG_NOTE(renderer, "fill_bezier n:%d %fx%f ...\n", 
              numpoints, points->p1.x, points->p1.y);
}

static void
draw_string(MyRenderer *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
    int len;
    W32::HGDIOBJ hOld;

    DIAG_NOTE(renderer, "draw_string %f,%f %s\n", 
              pos->x, pos->y, text);

    switch (alignment) {
    case ALIGN_LEFT:
	W32::SetTextAlign(renderer->hFileDC, TA_LEFT+TA_BASELINE);
	break;
    case ALIGN_CENTER:
	W32::SetTextAlign(renderer->hFileDC, TA_CENTER+TA_BASELINE);
	break;
    case ALIGN_RIGHT:
	W32::SetTextAlign(renderer->hFileDC, TA_RIGHT+TA_BASELINE);
	break;
    }
    /* work out size of first chunk of text */
    len = strlen(text);

    hOld = W32::SelectObject(renderer->hFileDC, renderer->hFont);
    W32::TextOut(renderer->hFileDC,
	SC(pos->x), SC(pos->y),
	text, len);
    W32::SelectObject(renderer->hFileDC, hOld);
}

static void
draw_image(MyRenderer *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
    W32::HBITMAP hBmp;
    int iWidth, iHeight;
    unsigned char* pData = NULL;

    DIAG_NOTE(renderer, "draw_image %fx%f @%f,%f\n", 
              width, height, point->x, point->y);

    iWidth  = dia_image_width(image);
    iHeight = dia_image_height(image);

    if ((dia_image_width(image)*3) % 4)
    {
	/* transform data to be DWORD aligned */
	int x, y;
	const unsigned char* pIn = NULL;
	unsigned char* pOut = NULL;

	pOut = pData = g_new(unsigned char, ((((iWidth*3-1)/4)+1)*4)*iHeight);

	pIn = dia_image_rgb_data(image);
	for (y = 0; y < iHeight; y++)
	{
	    for (x = 0; x < iWidth; x++)
	    {
		*pOut++ = *pIn++;
		*pOut++ = *pIn++;
		*pOut++ = *pIn++;
	    }
	    pOut += (4 - (iWidth*3)%4);
	}

	hBmp = W32::CreateBitmap ( iWidth, iHeight, 1, 24, pData);
    }
    else
    {
	hBmp = W32::CreateBitmap ( 
	    dia_image_width(image), dia_image_height(image),
	    1, 24, dia_image_rgb_data(image));
    }

    W32::SelectObject(renderer->hFileDC, hBmp);
    //Hack to get SRCCOPY out of namespace W32
#   ifndef DWORD
#     define DWORD unsigned long
#   endif
    W32::BitBlt(renderer->hFileDC, // destination
	SC(point->x), SC(point->y), SC(width), SC(height),
	renderer->hFileDC, // source
	0, 0, SRCCOPY);

    W32::DeleteObject(hBmp);
    if (pData)
	g_free(pData);
}

static void
export_data(DiagramData *data, const gchar *filename, const gchar *diafilename)
{
    MyRenderer *renderer;
    W32::HDC  file;
    Rectangle *extent;
    gint len;

    W32::RECT bbox;

    bbox.top = SC(data->extents.top);
    bbox.left = SC(data->extents.left);
    bbox.right = SC(data->extents.right);
    bbox.bottom = SC(data->extents.bottom);

    file = (W32::HDC)W32::CreateEnhMetaFile( 
	W32::GetDC(NULL), // handle to a reference device context
#ifdef SAVE_EMF
	filename,   // pointer to a filename string
#else
      NULL, // in memory
#endif
	&bbox,	    // pointer to a bounding rectangle
	"Dia\0Diagram\0"); // pointer to an optional description string 

    if (file == NULL) {
	message_error(_("Couldn't open: '%s' for writing.\n"), filename);
	return;
    }

    renderer = g_new(MyRenderer, 1);
    renderer->renderer.ops = &MyRenderOps;
    renderer->renderer.is_interactive = 0;
    renderer->renderer.interactive_ops = NULL;

    renderer->hFileDC = file;
    renderer->sFileName = g_strdup(filename);

    extent = &data->extents;

    /* write extents */
    DIAG_NOTE(renderer, "export_data extents %f,%f -> %f,%f\n", 
              extent->left, extent->top, extent->right, extent->bottom);


    data_render(data, (Renderer *)renderer, NULL, NULL, NULL);

    g_free(renderer);
}

static const gchar *extensions[] = { "wmf", NULL };
static DiaExportFilter my_export_filter = {
    N_("Windows Meta File"),
    extensions,
    export_data
};


/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    if (!dia_plugin_info_init(info, "WMF",
			      _("WMF export filter"),
			      NULL, NULL))
	return DIA_PLUGIN_INIT_ERROR;

    filter_register_export(&my_export_filter);

    return DIA_PLUGIN_INIT_OK;
}
