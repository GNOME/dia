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

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "plug-ins.h"
#include "dia_image.h"

#ifdef __cplusplus
}
#endif

#ifdef HAVE_WINDOWS_H
namespace W32 {
// at least Rectangle conflicts ...
#include <windows.h>
}
#else
#include "wmf_gdi.h"
#define SAVE_EMF
#endif

/* force linking with gdi32 */
#pragma comment( lib, "gdi32" )


// #define SAVE_EMF

typedef struct _PlaceableMetaHeader
{
  guint32 Key;           /* Magic number (always 9AC6CDD7h) */
  guint16 Handle;        /* Metafile HANDLE number (always 0) */
  gint16  Left;          /* Left coordinate in metafile units */
  gint16  Top;           /* Top coordinate in metafile units */
  gint16  Right;         /* Right coordinate in metafile units */
  gint16  Bottom;        /* Bottom coordinate in metafile units */
  guint16 Inch;          /* Number of metafile units per inch */
  guint32 Reserved;      /* Reserved (always 0) */
  guint16 Checksum;      /* Checksum value for previous 10 WORDs */
} PLACEABLEMETAHEADER;

/* --- the renderer --- */
G_BEGIN_DECLS

#define WMF_TYPE_RENDERER           (wmf_renderer_get_type ())
#define WMF_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), WMF_TYPE_RENDERER, WmfRenderer))
#define WMF_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), WMF_TYPE_RENDERER, WmfRendererClass))
#define WMF_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WMF_TYPE_RENDERER))
#define WMF_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), WMF_TYPE_RENDERER, WmfRendererClass))

GType wmf_renderer_get_type (void) G_GNUC_CONST;

typedef struct _WmfRenderer WmfRenderer;
typedef struct _WmfRendererClass WmfRendererClass;

struct _WmfRenderer
{
    DiaRenderer parent_instance;

    W32::HDC  hFileDC;
    gchar*    sFileName;

    W32::HDC  hPrintDC;

    /* if applicable everything is scaled to 0.01 mm */
    int nLineWidth;  /* need to cache these, because ... */
    int fnPenStyle;  /* ... both are needed at the same time */
    W32::HPEN  hPen; /* ugliness by concept, see DonePen() */

    W32::HFONT hFont;
    PLACEABLEMETAHEADER pmh;
    double xoff, yoff;
    double scale;
};

struct _WmfRendererClass
{
  DiaRendererClass parent_class;
};

G_END_DECLS

/*
 * helper macros
 */
#define W32COLOR(c) \
	(W32::COLORREF)( 0xff * c->red + \
	((unsigned char)(0xff * c->green)) * 256 + \
	((unsigned char)(0xff * c->blue)) * 65536)

#define SC(a) ((int)((a)*renderer->scale))
#define SCX(a) ((int)(((a)+renderer->xoff)*renderer->scale))
#define SCY(a) ((int)(((a)+renderer->yoff)*renderer->scale))

/*
 * helper functions
 */
static W32::HPEN
UsePen(WmfRenderer* renderer, Color* colour)
{
    W32::HPEN hOldPen;
    if (colour) {
	W32::COLORREF rgb = W32COLOR(colour);
	renderer->hPen = W32::CreatePen(renderer->fnPenStyle, 
					renderer->nLineWidth,
					rgb);
    } else {
	renderer->hPen = (W32::HPEN)W32::GetStockObject(NULL_PEN);
    }
    hOldPen = (W32::HPEN)W32::SelectObject(renderer->hFileDC, renderer->hPen);
    return hOldPen;
}

static void
DonePen(WmfRenderer* renderer, W32::HPEN hPen)
{
    /* restore the OLD one ... */
    if (hPen) 
      W32::SelectObject(renderer->hFileDC, hPen);
    /* ... before deleting the last active */
    if (renderer->hPen)
    {
	W32::DeleteObject(renderer->hPen);
	renderer->hPen = NULL;
    }
}

#define DIAG_NOTE /* my_log */
void
my_log(WmfRenderer* renderer, char* format, ...)
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
begin_render(DiaRenderer *self)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    DIAG_NOTE(renderer, "begin_render\n");

    /* make unfilled the default */
    W32::SelectObject(renderer->hFileDC, 
	                W32::GetStockObject (HOLLOW_BRUSH) );
}

static void
end_render(DiaRenderer *self)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HENHMETAFILE hEmf;
    W32::UINT nSize;
    W32::BYTE* pData = NULL;
    FILE* f;

    DIAG_NOTE(renderer, "end_render\n");
    hEmf = W32::CloseEnhMetaFile(renderer->hFileDC);

#ifndef SAVE_EMF
    /* Don't do it when printing */
    if (renderer->sFileName && strlen (renderer->sFileName)) {

        W32::HDC hdc = W32::GetDC(NULL);

        f = fopen(renderer->sFileName, "wb");

	/* write the placeable header */
	fwrite(&renderer->pmh, 1, 22 /* NOT: sizeof(PLACEABLEMETAHEADER) */, f);

	/* get size */
	nSize = W32::GetWinMetaFileBits(hEmf, 0, NULL, MM_ANISOTROPIC, hdc);
	pData = g_new(W32::BYTE, nSize);
	/* get data */
	nSize = W32::GetWinMetaFileBits(hEmf, nSize, pData, MM_ANISOTROPIC, hdc);

	/* write file */
	fwrite(pData,1,nSize,f);
	fclose(f);
    
	g_free(pData);

        W32::DeleteDC(hdc);
    } else {
        W32::RECT r = {0, 0, 0, 0};
        r.right = W32::GetDeviceCaps (renderer->hPrintDC, PHYSICALWIDTH); 
        r.bottom = W32::GetDeviceCaps (renderer->hPrintDC, PHYSICALHEIGHT); 

        W32::PlayEnhMetaFile (renderer->hPrintDC, hEmf, &r);
    }
#endif
    g_free(renderer->sFileName);

    if (hEmf)
	W32::DeleteEnhMetaFile(hEmf);
    if (renderer->hFont)
	W32::DeleteObject(renderer->hFont);
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{  
    WmfRenderer *renderer = WMF_RENDERER (self);

    DIAG_NOTE(renderer, "set_linewidth %f\n", linewidth);

    renderer->nLineWidth = SC(linewidth);
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    DIAG_NOTE(renderer, "set_linecaps %d\n", mode);

    switch(mode) {
    case LINECAPS_BUTT:
	break;
    case LINECAPS_ROUND:
	break;
    case LINECAPS_PROJECTING:
	break;
    default:
	message_error("WmfRenderer : Unsupported fill mode specified!\n");
    }
}

static void
set_linejoin(DiaRenderer *self, LineJoin mode)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    DIAG_NOTE(renderer, "set_join %d\n", mode);

    switch(mode) {
    case LINEJOIN_MITER:
	break;
    case LINEJOIN_ROUND:
	break;
    case LINEJOIN_BEVEL:
	break;
    default:
	message_error("WmfRenderer : Unsupported fill mode specified!\n");
    }
}

static void
set_linestyle(DiaRenderer *self, LineStyle mode)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

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
	message_error("WmfRenderer : Unsupported fill mode specified!\n");
    }
    
    /* Non-solid linestyles are only displayed if width <= 1. 
     * Better implementation will require custom linestyles */
    switch (mode) {
    case LINESTYLE_DASHED:
    case LINESTYLE_DASH_DOT:
    case LINESTYLE_DASH_DOT_DOT:
    case LINESTYLE_DOTTED:
      renderer->nLineWidth = MIN(renderer->nLineWidth, 1);
      break;
    }
}

static void
set_dashlength(DiaRenderer *self, real length)
{  
    WmfRenderer *renderer = WMF_RENDERER (self);

    DIAG_NOTE(renderer, "set_dashlength %f\n", length);

    /* dot = 10% of len */
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    DIAG_NOTE(renderer, "set_fillstyle %d\n", mode);

    switch(mode) {
    case FILLSTYLE_SOLID:
	break;
    default:
	message_error("WmfRenderer : Unsupported fill mode specified!\n");
    }
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    W32::LPCTSTR sFace;
    W32::DWORD dwItalic = 0;
    W32::DWORD dwWeight = FW_DONTCARE;
    DiaFontStyle style;

    DIAG_NOTE(renderer, "set_font %s %f\n", 
              dia_font_get_family (font), height);
    if (renderer->hFont)
	W32::DeleteObject(renderer->hFont);

    sFace = dia_font_get_family (font);
    style = dia_font_get_style(font);
    dwItalic = DIA_FONT_STYLE_GET_SLANT(style) != DIA_FONT_NORMAL;

    /* although there is a known algorithm avoid it for cleanness */
    switch (DIA_FONT_STYLE_GET_WEIGHT(style)) {
    case DIA_FONT_ULTRALIGHT    : dwWeight = FW_ULTRALIGHT; break;
    case DIA_FONT_LIGHT         : dwWeight = FW_LIGHT; break;
    case DIA_FONT_MEDIUM        : dwWeight = FW_MEDIUM; break;
    case DIA_FONT_DEMIBOLD      : dwWeight = FW_DEMIBOLD; break;
    case DIA_FONT_BOLD          : dwWeight = FW_BOLD; break;
    case DIA_FONT_ULTRABOLD     : dwWeight = FW_ULTRABOLD; break;
    case DIA_FONT_HEAVY         : dwWeight = FW_HEAVY; break;
    default : dwWeight = FW_NORMAL; break;
    }

    renderer->hFont = (W32::HFONT)W32::CreateFont( 
	- SC (height),  // logical height of font 
	0,		// logical average character width 
	0,		// angle of escapement
	0,		// base-line orientation angle 
	dwWeight,	// font weight
	dwItalic,	// italic attribute flag
	0,		// underline attribute flag
	0,		// strikeout attribute flag
	DEFAULT_CHARSET,	// character set identifier 
	OUT_TT_PRECIS, 	// output precision 
	CLIP_DEFAULT_PRECIS,	// clipping precision
	PROOF_QUALITY,		// output quality 
	DEFAULT_PITCH,		// pitch and family
	sFace);		// pointer to typeface name string 
}

static void
draw_line(DiaRenderer *self, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    W32::HPEN hPen;

    DIAG_NOTE(renderer, "draw_line %f,%f -> %f, %f\n", 
              start->x, start->y, end->x, end->y);

    hPen = UsePen(renderer, line_colour);

    W32::MoveToEx(renderer->hFileDC, SCX(start->x), SCY(start->y), NULL);
    W32::LineTo(renderer->hFileDC, SCX(end->x), SCY(end->y));

    DonePen(renderer, hPen);
}

static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    W32::HPEN hPen;
    W32::POINT*  pts;
    int	    i;

    DIAG_NOTE(renderer, "draw_polyline n:%d %f,%f ...\n", 
              num_points, points->x, points->y);

    if (num_points < 2)
	return;
    pts = g_new (W32::POINT, num_points+1);
    for (i = 0; i < num_points; i++)
    {
	pts[i].x = SCX(points[i].x);
	pts[i].y = SCY(points[i].y);
    }

    hPen = UsePen(renderer, line_colour);
    W32::Polyline(renderer->hFileDC, pts, num_points);
    DonePen(renderer, hPen);

    g_free(pts);
}

static void
draw_polygon(DiaRenderer *self, 
	     Point *points, int num_points, 
	     Color *line_colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    W32::HPEN    hPen;
    W32::POINT*  pts;
    int          i;

    DIAG_NOTE(renderer, "draw_polygon n:%d %f,%f ...\n", 
              num_points, points->x, points->y);

    if (num_points < 2)
	  return;
    pts = g_new (W32::POINT, num_points+1);
    for (i = 0; i < num_points; i++)
    {
	pts[i].x = SCX(points[i].x);
	pts[i].y = SCY(points[i].y);
    }

    hPen = UsePen(renderer, line_colour);

    W32::Polygon(renderer->hFileDC, pts, num_points);

    DonePen(renderer, hPen);

    g_free(pts);
}

static void
fill_polygon(DiaRenderer *self, 
	     Point *points, int num_points, 
	     Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    W32::HBRUSH  hBrush, hBrOld;
    W32::COLORREF rgb = W32COLOR(colour);

    DIAG_NOTE(renderer, "fill_polygon n:%d %f,%f ...\n", 
              num_points, points->x, points->y);

    hBrush = W32::CreateSolidBrush(rgb);
    hBrOld = (W32::HBRUSH)W32::SelectObject(renderer->hFileDC, hBrush);

    draw_polygon(self, points, num_points, NULL);

    W32::SelectObject(renderer->hFileDC, 
                      W32::GetStockObject(HOLLOW_BRUSH) );
    W32::DeleteObject(hBrush);
}

static void
draw_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    W32::HPEN hPen;

    DIAG_NOTE(renderer, "draw_rect %f,%f -> %f,%f\n", 
              ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y);

    hPen = UsePen(renderer, colour);

    W32::Rectangle(renderer->hFileDC,
                   SCX(ul_corner->x), SCY(ul_corner->y),
                   SCX(lr_corner->x), SCY(lr_corner->y));

    DonePen(renderer, hPen);
}

static void
fill_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HGDIOBJ hBrush, hBrOld;
    W32::COLORREF rgb = W32COLOR(colour);

    DIAG_NOTE(renderer, "fill_rect %f,%f -> %f,%f\n", 
              ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y);

    hBrush = W32::CreateSolidBrush(rgb);
    hBrOld = W32::SelectObject(renderer->hFileDC, hBrush);

    draw_rect(self, ul_corner, lr_corner, NULL);

    W32::SelectObject(renderer->hFileDC, 
                    W32::GetStockObject (HOLLOW_BRUSH) );
    W32::DeleteObject(hBrush);
}

static void
draw_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HPEN  hPen;
    W32::POINT ptStart, ptEnd;

    DIAG_NOTE(renderer, "draw_arc %fx%f <%f,<%f @%f,%f\n", 
              width, height, angle1, angle2, center->x, center->y);

    hPen = UsePen(renderer, colour);

    /* calculate start and end points of arc */
    ptStart.x = SCX(center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle1));
    ptStart.y = SCY(center->y - (height / 2.0) * sin((M_PI / 180.0) * angle1));
    ptEnd.x = SCX(center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle2));
    ptEnd.y = SCY(center->y - (height / 2.0) * sin((M_PI / 180.0) * angle2));

    W32::MoveToEx(renderer->hFileDC, ptStart.x, ptStart.y, NULL);
    W32::Arc(renderer->hFileDC,
             SCX(center->x - width / 2), /* bbox corners */
             SCY(center->y - height / 2),
             SCX(center->x + width / 2), 
             SCY(center->y + height / 2),
             ptStart.x, ptStart.y, ptEnd.x, ptEnd.y); 

    DonePen(renderer, hPen);
}

static void
fill_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HPEN    hPen;
    W32::HGDIOBJ hBrush, hBrOld;
    W32::POINT ptStart, ptEnd;
    W32::COLORREF rgb = W32COLOR(colour);

    DIAG_NOTE(renderer, "fill_arc %fx%f <%f,<%f @%f,%f\n", 
              width, height, angle1, angle2, center->x, center->y);

    /* calculate start and end points of arc */
    ptStart.x = SCX(center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle1));
    ptStart.y = SCY(center->y - (height / 2.0) * sin((M_PI / 180.0) * angle1));
    ptEnd.x = SCX(center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle2));
    ptEnd.y = SCY(center->y - (height / 2.0) * sin((M_PI / 180.0) * angle2));

    hPen = UsePen(renderer, NULL); /* no border */
    hBrush = W32::CreateSolidBrush(rgb);
    hBrOld = W32::SelectObject(renderer->hFileDC, hBrush);

    W32::Pie(renderer->hFileDC,
             SCX(center->x - width / 2), /* bbox corners */
             SCY(center->y - height / 2),
             SCX(center->x + width / 2), 
             SCY(center->y + height / 2),
             ptStart.x, ptStart.y, ptEnd.x, ptEnd.y); 

    W32::SelectObject(renderer->hFileDC, 
                    W32::GetStockObject (HOLLOW_BRUSH) );
    W32::DeleteObject(hBrush);
    DonePen(renderer, hPen);
}

static void
draw_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HPEN hPen;

    DIAG_NOTE(renderer, "draw_ellipse %fx%f @ %f,%f\n", 
              width, height, center->x, center->y);

    hPen = UsePen(renderer, colour);

    W32::Ellipse(renderer->hFileDC,
                 SCX(center->x - width / 2), /* bbox corners */
                 SCY(center->y - height / 2),
                 SCX(center->x + width / 2), 
                 SCY(center->y + height / 2));

    DonePen(renderer, hPen);
}

static void
fill_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HPEN    hPen;
    W32::HGDIOBJ hBrush, hBrOld;
    W32::COLORREF rgb = W32COLOR(colour);

    DIAG_NOTE(renderer, "fill_ellipse %fx%f @ %f,%f\n", 
              width, height, center->x, center->y);

    hBrush = W32::CreateSolidBrush(rgb);
    hBrOld = W32::SelectObject(renderer->hFileDC, hBrush);

    draw_ellipse(self, center, width, height, NULL);

    W32::SelectObject(renderer->hFileDC, 
                      W32::GetStockObject (HOLLOW_BRUSH) );
    W32::DeleteObject(hBrush);
}

static void
draw_bezier(DiaRenderer *self, 
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HPEN    hPen;
    W32::POINT * pts;
    int i;

    DIAG_NOTE(renderer, "draw_bezier n:%d %fx%f ...\n", 
              numpoints, points->p1.x, points->p1.y);

    pts = g_new(W32::POINT, (numpoints-1) * 3 + 1);

    pts[0].x = SCX(points[0].p1.x);
    pts[0].y = SCY(points[0].p1.y);

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
            pts[i*3  ].x = SCX(points[i].p1.x);
            pts[i*3-2].y = pts[i*3-1].y = 
            pts[i*3  ].y = SCY(points[i].p1.y);
        break;
        case _BezPoint::BEZ_CURVE_TO:
            /* control points */
            pts[i*3-2].x = SCX(points[i].p1.x);
            pts[i*3-2].y = SCY(points[i].p1.y);
            pts[i*3-1].x = SCX(points[i].p2.x);
            pts[i*3-1].y = SCY(points[i].p2.y);
            /* end point */
            pts[i*3  ].x = SCX(points[i].p3.x);
            pts[i*3  ].y = SCY(points[i].p3.y);
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

#ifndef SAVE_EMF
/* not defined in compatibility layer */
static void
fill_bezier(DiaRenderer *self, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HGDIOBJ hBrush, hBrOld;
    W32::COLORREF rgb = W32COLOR(colour);

    DIAG_NOTE(renderer, "fill_bezier n:%d %fx%f ...\n", 
              numpoints, points->p1.x, points->p1.y);

    hBrush = W32::CreateSolidBrush(rgb);
    hBrOld = W32::SelectObject(renderer->hFileDC, hBrush);

    W32::BeginPath (renderer->hFileDC);
    draw_bezier(self, points, numpoints, NULL);
    W32::EndPath (renderer->hFileDC);
    W32::FillPath (renderer->hFileDC);

    W32::SelectObject(renderer->hFileDC, 
                      W32::GetStockObject (HOLLOW_BRUSH) );
    W32::DeleteObject(hBrush);
}
#endif

static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    int len;
    W32::HGDIOBJ hOld;
    W32::COLORREF rgb = W32COLOR(colour);

    DIAG_NOTE(renderer, "draw_string %f,%f %s\n", 
              pos->x, pos->y, text);

    W32::SetTextColor(renderer->hFileDC, rgb);

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
    {
# if 0 // one way to go, but see below ...
        gint wclen = 0;
        gunichar2* swc = g_utf8_to_utf16 (text, -1, NULL, &wclen, NULL);
        W32::TextOutW (renderer->hFileDC,
                       SCX(pos->x), SCY(pos->y),
                       swc, wclen);
        g_free (swc);
# else
        // works with newest cvs and tml's "official" 2000-12-26 release
        char* scp; 
        /* convert from utf8 to active codepage */
        static char codepage[10];
        sprintf (codepage, "CP%d", W32::GetACP ());

        scp = g_convert (text, strlen (text),
                         codepage, "UTF-8",
                         NULL, NULL, NULL);
        if (scp)
        {
            W32::TextOut(renderer->hFileDC,
                         SCX(pos->x), SCY(pos->y),
                         scp, strlen(scp));
            g_free (scp);
        }
        else // converson failed, write unconverted
            W32::TextOut(renderer->hFileDC,
                         SCX(pos->x), SCY(pos->y),
                         text, strlen (text));
# endif
    }

    W32::SelectObject(renderer->hFileDC, hOld);
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
#ifdef SAVE_EMF
    /* not yet supported in compatibility mode */
#else	
    W32::HBITMAP hBmp;
    int iWidth, iHeight;
    unsigned char* pData = NULL;
    unsigned char* pImg  = NULL;

    DIAG_NOTE(renderer, "draw_image %fx%f @%f,%f\n", 
              width, height, point->x, point->y);

    iWidth  = dia_image_width(image);
    iHeight = dia_image_height(image);
    pImg = dia_image_rgb_data(image);

#if 0 /* only working with 24 bit screen resolution */
    if ((dia_image_width(image)*3) % 4)
    {
        /* transform data to be DWORD aligned */
        int x, y;
        const unsigned char* pIn = NULL;
        unsigned char* pOut = NULL;

        pOut = pData = g_new(unsigned char, ((((iWidth*3-1)/4)+1)*4)*iHeight);

        pIn = pImg;
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
                        1, 24, pImg);
    }
    W32::HDC hMemDC = W32::CreateCompatibleDC (renderer->hFileDC);
#else
    W32::HDC hMemDC = W32::CreateCompatibleDC (renderer->hFileDC);
    W32::BITMAPINFO bmi;
    memset (&bmi, 0, sizeof (W32::BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof (W32::BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = iWidth;
    bmi.bmiHeader.biHeight = -iHeight; // invert it
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;

    hBmp = W32::CreateDIBSection (hMemDC, &bmi, DIB_RGB_COLORS,
                                  (void**)&pData, NULL, 0);
    /* copy data, always line by line */
    for (int y = 0; y < iHeight; y ++) 
    {
        int line_offset = dia_image_rowstride(image) * y;
        for (int x = 0; x < iWidth*3; x+=3)
        {
            // change from RGB to BGR order
            pData[x  ] = pImg[line_offset + x+2];
            pData[x+1] = pImg[line_offset + x+1];
            pData[x+2] = pImg[line_offset + x  ];
        }
        pData += (((3*iWidth-1)/4)+1)*4;
    }
    pData = NULL; // don't free it below
#endif
    W32::HBITMAP hOldBmp = (W32::HBITMAP)W32::SelectObject(hMemDC, hBmp);
    //Hack to get SRCCOPY out of namespace W32
#   ifndef DWORD
#     define DWORD unsigned long
#   endif
    W32::StretchBlt(renderer->hFileDC, // destination
                SCX(point->x), SCY(point->y), SC(width), SC(height),
                hMemDC, // source
                0, 0, iWidth, iHeight, SRCCOPY);

    W32::SelectObject (hMemDC, hOldBmp);
    W32::DeleteDC (hMemDC);
    W32::DeleteObject (hBmp);
    if (pData)
        g_free (pData);
    if (pImg)
        g_free (pImg);
#endif /* SAVE_EMF */
}

/* GObject boiler plate */
static void wmf_renderer_class_init (WmfRendererClass *klass);

static gpointer parent_class = NULL;

GType
wmf_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (WmfRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) wmf_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (WmfRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "WmfRenderer",
                                            &object_info, (GTypeFlags)0);
    }
  
  return object_type;
}

static void
wmf_renderer_finalize (GObject *object)
{
  WmfRenderer *wmf_renderer = WMF_RENDERER (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
wmf_renderer_class_init (WmfRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = wmf_renderer_finalize;

  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->set_font  = set_font;

  renderer_class->draw_line    = draw_line;
  renderer_class->fill_polygon = fill_polygon;
  renderer_class->draw_rect    = draw_rect;
  renderer_class->fill_rect    = fill_rect;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;
  renderer_class->draw_polygon   = draw_polygon;

  renderer_class->draw_bezier   = draw_bezier;
#ifndef SAVE_EMF
  renderer_class->fill_bezier   = fill_bezier;
#endif
}

/* plug-in export api */
static void
export_data(DiagramData *data, const gchar *filename, 
            const gchar *diafilename, void* user_data)
{
    WmfRenderer *renderer;
    W32::HDC  file;
    Rectangle *extent;
    gint len;
    double scale;

    W32::RECT bbox;

    /* Bounding Box in .01-millimeter units ??? */
    bbox.top = 0;
    bbox.left = 0;
    if (  (data->extents.right - data->extents.left)
        > (data->extents.bottom - data->extents.top)) {
        scale = floor (32000.0 / (data->extents.right - data->extents.left));
    }
    else {
        scale = floor (32000.0 / (data->extents.bottom - data->extents.top));
    }
    bbox.right = (int)((data->extents.right - data->extents.left) * scale);
    bbox.bottom = (int)((data->extents.bottom - data->extents.top) * scale);

    file = (W32::HDC)W32::CreateEnhMetaFile(
                    W32::GetDC(NULL), // handle to a reference device context
#ifdef SAVE_EMF
                    filename, // pointer to a filename string
#else
                    NULL, // in memory
#endif
                    &bbox, // pointer to a bounding rectangle
                    "Dia\0Diagram\0"); // pointer to an optional description string 

    if (file == NULL) {
        message_error(_("Couldn't open: '%s' for writing.\n"), filename);
        return;
    }

    renderer = (WmfRenderer*)g_object_new(WMF_TYPE_RENDERER, NULL);

    renderer->hFileDC = file;
    renderer->sFileName = g_strdup(filename);
    renderer->hPrintDC = (W32::HDC)user_data;
           
    extent = &data->extents;

    /* calculate offsets */
    renderer->xoff = - data->extents.left;
    renderer->yoff = - data->extents.top;
    renderer->scale = scale / 25.4; /* don't know why this is required ... */

    /* initialize placeable header */
    /* bounding box in twips 1/1440 of an inch */
    renderer->pmh.Key = 0x9AC6CDD7;
    renderer->pmh.Handle = 0;
    renderer->pmh.Left   = 0;
    renderer->pmh.Top    = 0;
    renderer->pmh.Right  = (gint16)(SC(data->extents.right - data->extents.left) * 25.4);
    renderer->pmh.Bottom = (gint16)(SC(data->extents.bottom - data->extents.top) * 25.4);
    renderer->pmh.Inch   = 1440 * 10;
    renderer->pmh.Reserved = 0;

    guint16 *ptr;
    renderer->pmh.Checksum = 0;
    for (ptr = (guint16 *)&renderer->pmh; ptr < (guint16 *)&(renderer->pmh.Checksum); ptr++)
        renderer->pmh.Checksum ^= *ptr;

#ifdef SAVE_EMF
    /* write the placeable header */
    fwrite(&renderer->pmh, 1, 22 /* NOT: sizeof(PLACEABLEMETAHEADER) */, file->file);
#endif
    
    /* bounding box in device units */
    bbox.left = 0;
    bbox.top = 0;
    bbox.right = (int)(SC(data->extents.right - data->extents.left) * 25.4);
    bbox.bottom = (int)(SC(data->extents.bottom - data->extents.top) * 25.4);

    /* initialize drawing */
    W32::SetBkMode(renderer->hFileDC, TRANSPARENT);
    W32::SetMapMode(renderer->hFileDC, MM_TEXT);
    W32::IntersectClipRect(renderer->hFileDC, 
                           bbox.left, bbox.top,
                           bbox.right, bbox.bottom);

    renderer->scale *= 0.95; /* just a little smaller ... */

    /* write extents */
    DIAG_NOTE(renderer, "export_data extents %f,%f -> %f,%f\n", 
              extent->left, extent->top, extent->right, extent->bottom);

    data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);

    g_object_unref(renderer);
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
