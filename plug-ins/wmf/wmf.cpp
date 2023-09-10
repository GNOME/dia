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

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <errno.h>
#include <glib/gstdio.h>

#include "geometry.h"
#include "diarenderer.h"
#include "filter.h"
#include "plug-ins.h"
#include "dia_image.h"
#include "object.h" // ObjectOps::draw

#include "paginate_gdiprint.h"

#if defined HAVE_WINDOWS_H || defined G_OS_WIN32
#ifdef _MSC_VER
#ifndef _DLL
#error "fwrite will fail with wrong crt"
#endif
#endif
  namespace W32 {
   // at least Rectangle conflicts ...
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
  }
#undef STRICT
typedef W32::HDC HDC;
typedef W32::HFONT HFONT;
typedef W32::LOGFONT LOGFONT;
typedef W32::LOGFONTA LOGFONTA;
typedef W32::LOGFONTW LOGFONTW;

#include <pango/pangowin32.h>

#elif defined(HAVE_LIBEMF)
/* We have to define STRICT to make libemf/64 work. Otherwise there is
wmf.cpp:1383:40: error: cast from 'void*' to 'W32::HDC' loses precision
 */
#define STRICT
  namespace W32 {
#  include <libEMF/emf.h>
  }
#undef STRICT
#else
#  include "wmf_gdi.h"
#  define DIRECT_WMF
#endif

#ifdef G_OS_WIN32
/* force linking with gdi32 */
#pragma comment( lib, "gdi32" )
#endif

// #define DIRECT_WMF

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

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


GType wmf_renderer_get_type (void) G_GNUC_CONST;

typedef struct _WmfRenderer WmfRenderer;
typedef struct _WmfRendererClass WmfRendererClass;

struct _WmfRenderer
{
  DiaRenderer parent_instance;

  DiaFont *font;
  double font_height;

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

  int nDashLen; /* the scaled dash length */
  gboolean platform_is_nt; /* advanced line styles supported */
  gboolean target_emf; /* write enhanced metafile */
  W32::RECT margins;

  gboolean use_pango;
  PangoContext* pango_context;

  DiaContext* ctx;
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
#if defined(G_OS_WIN32) || defined(HAVE_LIBEMF)
	if ((renderer->platform_is_nt && renderer->hPrintDC) || renderer->target_emf) {
          W32::LOGBRUSH logbrush;
	  W32::DWORD    dashes[6];
	  int num_dashes = 0;
	  int dashlen = renderer->nDashLen;
	  int dotlen  = renderer->nDashLen / 10;

	  logbrush.lbStyle = BS_SOLID;
	  logbrush.lbColor = rgb;
	  logbrush.lbHatch = 0;

          switch (renderer->fnPenStyle & PS_STYLE_MASK) {
	  case PS_SOLID :
	    break;
	  case PS_DASH :
	    num_dashes = 2;
	    dashes[0] = dashes[1] = dashlen;
	    break;
	  case PS_DASHDOT :
	    num_dashes = 4;
	    dashes[1] = dashes[3] = MAX((dashlen - dotlen) / 2, 1);
	    dashes[0] = dashlen;
	    dashes[2] = dotlen;
	    break;
	  case PS_DASHDOTDOT :
	    num_dashes = 6;
	    dashes[0] = dashlen;
	    dashes[1] = dashes[3] = dashes[5] = MAX((dashlen - 2 * dotlen)/3, 1);
	    dashes[2] = dashes[4] = dotlen;
	    break;
	  case PS_DOT :
	    num_dashes = 2;
	    dashes[0] = dashes[1] = dotlen;
	    break;
	  default :
	    g_assert_not_reached ();
	  }

	  renderer->hPen = W32::ExtCreatePen (
	    (renderer->fnPenStyle & ~(PS_STYLE_MASK)) | (PS_GEOMETRIC | (num_dashes > 0 ? PS_USERSTYLE : 0)),
	    renderer->nLineWidth,
	    &logbrush, num_dashes, num_dashes > 0 ? dashes : 0);
	}
	else
#endif /* G_OS_WIN32 */
	{
	  renderer->hPen = W32::CreatePen(renderer->fnPenStyle,
					  renderer->nLineWidth,
					  rgb);
	}
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

#ifndef HAVE_LIBEMF
#  define DIAG_NOTE _nada
static void _nada(WmfRenderer*, const char*, ...) { }
#else
#  define DIAG_NOTE my_log

static void
my_log(WmfRenderer* renderer, const char* format, ...)
{
    gchar *string;
    va_list args;

    g_return_if_fail (format != NULL);

    va_start (args, format);
    string = g_strdup_vprintf (format, args);
    va_end (args);

    //fprintf(renderer->file, string);
    g_print("%s", string);

    g_free(string);
}
#endif

/*
 * renderer interface implementation
 */
static void
begin_render(DiaRenderer *self, const DiaRectangle *)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    DIAG_NOTE(renderer, "begin_render\n");

    /* FIXME: still not sure if the renderer output should be platform dependent */
    if (renderer->platform_is_nt)
      renderer->fnPenStyle = PS_GEOMETRIC;

    /* make unfilled the default */
    W32::SelectObject(renderer->hFileDC,
	                W32::GetStockObject (HOLLOW_BRUSH) );
}

static void
end_render(DiaRenderer *self)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HENHMETAFILE hEmf;
#if !defined DIRECT_WMF && defined G_OS_WIN32 /* the later offers both */
    W32::UINT nSize;
    W32::BYTE* pData = NULL;
    FILE* f;
#endif
    DIAG_NOTE(renderer, "end_render\n");
    hEmf = W32::CloseEnhMetaFile(renderer->hFileDC);

#if !defined DIRECT_WMF && defined G_OS_WIN32 /* the later offers both */
    /* Don't do it when printing */
    if (renderer->sFileName && strlen (renderer->sFileName)) {

        W32::HDC hdc = W32::GetDC(NULL);

        f = g_fopen(renderer->sFileName, "wb");

	if (renderer->target_emf) {
	  nSize = W32::GetEnhMetaFileBits (hEmf, 0, NULL);
	  pData = g_new(W32::BYTE, nSize);
	  nSize = W32::GetEnhMetaFileBits (hEmf, nSize, pData);
	} else {
	  /* write the placeable header */
	  fwrite(&renderer->pmh, 1, 22 /* NOT: sizeof(PLACEABLEMETAHEADER) */, f);
	  /* get size */
	  nSize = W32::GetWinMetaFileBits(hEmf, 0, NULL, MM_ANISOTROPIC, hdc);
	  pData = g_new(W32::BYTE, nSize);
	  /* get data */
	  nSize = W32::GetWinMetaFileBits(hEmf, nSize, pData, MM_ANISOTROPIC, hdc);
	}

	/* write file */
	if (fwrite(pData,1,nSize,f) != nSize)
	  dia_context_add_message_with_errno(renderer->ctx, errno, _("Couldn't write file %s\n%s"),
					     dia_context_get_filename (renderer->ctx));
	fclose(f);

	g_free(pData);

        W32::ReleaseDC(NULL, hdc);
    } else {
        W32::PlayEnhMetaFile (renderer->hPrintDC, hEmf, &renderer->margins);
    }
#endif
    g_free(renderer->sFileName);

    if (hEmf)
	W32::DeleteEnhMetaFile(hEmf);
}

static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  if (RENDER_HOLES == cap)
    return TRUE;
  else if (RENDER_ALPHA == cap)
    return TRUE;
  else if (RENDER_AFFINE == cap)
    return TRUE;
  return FALSE;
}

static gpointer parent_class = NULL;

#if defined(G_OS_WIN32)
static void
draw_object (DiaRenderer *self, DiaObject *object, DiaMatrix *matrix)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::XFORM xFormPrev = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    int mode = W32::GetGraphicsMode (renderer->hFileDC);
    gboolean fallback = FALSE;

    if (matrix) {
        if (   W32::SetGraphicsMode (renderer->hFileDC, GM_ADVANCED)
            && W32::GetWorldTransform (renderer->hFileDC, &xFormPrev)) {

            W32::XFORM xForm;

            xForm.eM11 = matrix->xx;
            xForm.eM12 = matrix->yx;
            xForm.eM21 = matrix->xy;
            xForm.eM22 = matrix->yy;
            xForm.eDx = matrix->x0;
            xForm.eDy = matrix->y0;
            if (!W32::SetWorldTransform (renderer->hFileDC, &xForm))
                fallback = TRUE;
        }
    }

    if (fallback)
        DIA_RENDERER_CLASS (parent_class)->draw_object (self, object, matrix);
    else
        object->ops->draw(object, DIA_RENDERER (renderer));

    if (matrix)
        W32::SetWorldTransform (renderer->hFileDC, &xFormPrev);

    W32::SetGraphicsMode (renderer->hFileDC, mode);
}
#endif

static void
set_linewidth(DiaRenderer *self, real linewidth)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    DIAG_NOTE(renderer, "set_linewidth %f\n", linewidth);

    renderer->nLineWidth = SC(linewidth);
}


static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  WmfRenderer *renderer = WMF_RENDERER (self);

  DIAG_NOTE (renderer, "set_linecaps %d\n", mode);

  // all the advanced line rendering is unsupported on win9x
  if (!renderer->platform_is_nt) {
    return;
  }

  renderer->fnPenStyle &= ~(PS_ENDCAP_MASK);
  switch (mode) {
    case DIA_LINE_CAPS_BUTT:
      renderer->fnPenStyle |= PS_ENDCAP_FLAT;
      break;
    case DIA_LINE_CAPS_ROUND:
      renderer->fnPenStyle |= PS_ENDCAP_ROUND;
      break;
    case DIA_LINE_CAPS_PROJECTING:
      renderer->fnPenStyle |= PS_ENDCAP_SQUARE;
      break;
    default:
      g_warning ("WmfRenderer : Unsupported fill mode specified!\n");
  }
}


static void
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  WmfRenderer *renderer = WMF_RENDERER (self);

  DIAG_NOTE (renderer, "set_join %d\n", mode);

  if (!renderer->platform_is_nt) {
    return;
  }

  renderer->fnPenStyle &= ~(PS_JOIN_MASK);

  switch (mode) {
    case DIA_LINE_JOIN_MITER:
      renderer->fnPenStyle |= PS_JOIN_MITER;
      break;
    case DIA_LINE_JOIN_ROUND:
      renderer->fnPenStyle |= PS_JOIN_ROUND;
      break;
    case DIA_LINE_JOIN_BEVEL:
      renderer->fnPenStyle |= PS_JOIN_BEVEL;
      break;
    default:
      g_warning ("WmfRenderer : Unsupported fill mode specified!\n");
  }
}


static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  WmfRenderer *renderer = WMF_RENDERER (self);

  DIAG_NOTE (renderer, "set_linestyle %d\n", mode);

  /* dot = 10% of len */
  renderer->nDashLen = SC (dash_length);
  /* line type */
  renderer->fnPenStyle &= ~(PS_STYLE_MASK);

  switch (mode) {
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
      renderer->fnPenStyle |= PS_SOLID;
      break;
    case DIA_LINE_STYLE_DASHED:
      renderer->fnPenStyle |= PS_DASH;
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      renderer->fnPenStyle |= PS_DASHDOT;
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      renderer->fnPenStyle |= PS_DASHDOTDOT;
      break;
    case DIA_LINE_STYLE_DOTTED:
      renderer->fnPenStyle |= PS_DOT;
      break;
    default:
      g_warning ("WmfRenderer : Unsupported fill mode specified!\n");
  }

  if (renderer->platform_is_nt) {
    return;
  }

  /* Non-solid linestyles are only displayed if width <= 1.
    * Better implementation will require custom linestyles
    * not available on win9x ...
    */
  switch (mode) {
    case DIA_LINE_STYLE_DASHED:
    case DIA_LINE_STYLE_DASH_DOT:
    case DIA_LINE_STYLE_DASH_DOT_DOT:
    case DIA_LINE_STYLE_DOTTED:
      renderer->nLineWidth = MIN (renderer->nLineWidth, 1);
    case DIA_LINE_STYLE_SOLID:
    case DIA_LINE_STYLE_DEFAULT:
      break;
  }
}


static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  WmfRenderer *renderer = WMF_RENDERER (self);

  DIAG_NOTE (renderer, "set_fillstyle %d\n", mode);

  switch (mode) {
    case DIA_FILL_STYLE_SOLID:
      break;
    default:
      g_warning ("WmfRenderer : Unsupported fill mode specified!\n");
  }
}


static void
set_font (DiaRenderer *self, DiaFont *font, real height)
{
  WmfRenderer *renderer = WMF_RENDERER (self);

  W32::LPCTSTR sFace;
  W32::DWORD dwItalic = 0;
  W32::DWORD dwWeight = FW_DONTCARE;
  DiaFontStyle style = dia_font_get_style (font);
  real font_size = dia_font_get_size (font) * (height / dia_font_get_height (font));

  g_clear_object (&renderer->font);
  renderer->font = DIA_FONT (g_object_ref (font));
  renderer->font_height = height;

  DIAG_NOTE (renderer, "set_font %s %f\n",
             dia_font_get_family (font), height);
  if (renderer->hFont) {
    W32::DeleteObject (renderer->hFont);
    renderer->hFont = NULL;
  }
  if (renderer->pango_context) {
    g_object_unref (renderer->pango_context);
    renderer->pango_context = NULL;
  }

  if (renderer->use_pango) {
#ifdef __PANGOWIN32_H__ /* with the pangowin32 backend there is a better way */
	if (!renderer->pango_context)
	    renderer->pango_context = pango_win32_get_context ();

	PangoFont* pf = pango_context_load_font (renderer->pango_context, dia_font_get_description (font));
	if (pf)
	{
	    W32::LOGFONT* lf = pango_win32_font_logfont (pf);
	    /* .93 : sligthly smaller looks much better */
	    lf->lfHeight = -SC(height*.93);
	    lf->lfHeight = -SC(font_size);
	    renderer->hFont = (W32::HFONT)W32::CreateFontIndirect (lf);
	    g_free (lf);
	    g_object_unref (pf);
	}
	else
	{
	    gchar *desc = pango_font_description_to_string (dia_font_get_description (font));
	    dia_context_add_message(renderer->ctx, _("Cannot render unknown font:\n%s"), desc);
	    g_free (desc);
	}
#else
    g_assert_not_reached();
#endif
  } else {
    sFace = dia_font_get_family (font);
    dwItalic = DIA_FONT_STYLE_GET_SLANT (style) != DIA_FONT_NORMAL;

    /* although there is a known algorithm avoid it for cleanness */
    switch (DIA_FONT_STYLE_GET_WEIGHT (style)) {
      case DIA_FONT_ULTRALIGHT    : dwWeight = FW_ULTRALIGHT; break;
      case DIA_FONT_LIGHT         : dwWeight = FW_LIGHT; break;
      case DIA_FONT_MEDIUM        : dwWeight = FW_MEDIUM; break;
      case DIA_FONT_DEMIBOLD      : dwWeight = FW_DEMIBOLD; break;
      case DIA_FONT_BOLD          : dwWeight = FW_BOLD; break;
      case DIA_FONT_ULTRABOLD     : dwWeight = FW_ULTRABOLD; break;
      case DIA_FONT_HEAVY         : dwWeight = FW_HEAVY; break;
      default : dwWeight = FW_NORMAL; break;
    }
    //Hack to get BYTE out of namespace W32
#       ifndef BYTE
#       define BYTE unsigned char
#       endif

  renderer->hFont = (W32::HFONT) W32::CreateFont (
        - SC (font_size),     // logical height of font
        0,                    // logical average character width
        0,                    // angle of escapement
        0,                    // base-line orientation angle
        dwWeight,             // font weight
        dwItalic,             // italic attribute flag
        0,                    // underline attribute flag
        0,                    // strikeout attribute flag
        DEFAULT_CHARSET,      // character set identifier
        OUT_TT_PRECIS,        // output precision
        CLIP_DEFAULT_PRECIS,  // clipping precision
        PROOF_QUALITY,        // output quality
        DEFAULT_PITCH,        // pitch and family
        sFace);               // pointer to typeface name string
  }
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
	     Color *fill, Color *stroke)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    W32::HPEN    hPen;
    W32::POINT*  pts;
    int          i;
    W32::HBRUSH  hBrush = 0;
    W32::COLORREF rgb = fill ? W32COLOR(fill) : 0;

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

    if (stroke)
      hPen = UsePen(renderer, stroke);
    if (fill) {
      hBrush = W32::CreateSolidBrush(rgb);
      W32::SelectObject(renderer->hFileDC, hBrush);
    }

    W32::Polygon(renderer->hFileDC, pts, num_points);

    if (stroke)
      DonePen(renderer, hPen);
    if (fill) {
      W32::SelectObject(renderer->hFileDC,
                        W32::GetStockObject(HOLLOW_BRUSH) );
      W32::DeleteObject(hBrush);
    }
    g_free(pts);
}

static void
draw_rect(DiaRenderer *self,
	  Point *ul_corner, Point *lr_corner,
	  Color *fill, Color *stroke)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    W32::HPEN hPen;

    DIAG_NOTE(renderer, "draw_rect %f,%f -> %f,%f\n",
              ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y);

    if (fill) {
	W32::HGDIOBJ hBrush;
	W32::COLORREF rgb = W32COLOR(fill);
	hBrush = W32::CreateSolidBrush(rgb);
	W32::SelectObject(renderer->hFileDC, hBrush);
	W32::Rectangle (renderer->hFileDC,
			SCX(ul_corner->x), SCY(ul_corner->y),
			SCX(lr_corner->x), SCY(lr_corner->y));
	W32::SelectObject (renderer->hFileDC,
			   W32::GetStockObject (HOLLOW_BRUSH) );
	W32::DeleteObject(hBrush);
    }
    if (stroke) {
	hPen = UsePen(renderer, stroke);

	W32::Rectangle (renderer->hFileDC,
			SCX(ul_corner->x), SCY(ul_corner->y),
			SCX(lr_corner->x), SCY(lr_corner->y));

	DonePen(renderer, hPen);
    }
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

    if (angle1 > angle2) {
	/* make it counter-clockwise by swapping start/end */
	real tmp = angle1;
	angle1 = angle2;
	angle2 = tmp;
    }
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

#ifndef HAVE_LIBEMF
static void
fill_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *colour)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HPEN    hPen;
    W32::HGDIOBJ hBrush;
    W32::POINT ptStart, ptEnd;
    W32::COLORREF rgb = W32COLOR(colour);

    DIAG_NOTE(renderer, "fill_arc %fx%f <%f,<%f @%f,%f\n",
              width, height, angle1, angle2, center->x, center->y);

    if (angle1 > angle2) {
	/* make it counter-clockwise by swapping start/end */
	real tmp = angle1;
	angle1 = angle2;
	angle2 = tmp;
    }
    /* calculate start and end points of arc */
    ptStart.x = SCX(center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle1));
    ptStart.y = SCY(center->y - (height / 2.0) * sin((M_PI / 180.0) * angle1));
    ptEnd.x = SCX(center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle2));
    ptEnd.y = SCY(center->y - (height / 2.0) * sin((M_PI / 180.0) * angle2));

    hPen = UsePen(renderer, NULL); /* no border */
    hBrush = W32::CreateSolidBrush(rgb);

    W32::SelectObject(renderer->hFileDC, hBrush);

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
#endif

static void
draw_ellipse(DiaRenderer *self,
	     Point *center,
	     real width, real height,
	     Color *fill, Color *stroke)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HPEN hPen;
    W32::HGDIOBJ hBrush;

    DIAG_NOTE(renderer, "draw_ellipse %fx%f @ %f,%f\n",
              width, height, center->x, center->y);

    if (fill) {
	W32::COLORREF rgb = W32COLOR(fill);
	hBrush = W32::CreateSolidBrush(rgb);
	W32::SelectObject(renderer->hFileDC, hBrush);
    }
    if (stroke)
	hPen = UsePen(renderer, stroke);

    W32::Ellipse(renderer->hFileDC,
                 SCX(center->x - width / 2), /* bbox corners */
                 SCY(center->y - height / 2),
                 SCX(center->x + width / 2),
                 SCY(center->y + height / 2));

    if (stroke)
	DonePen(renderer, hPen);
    if (fill) {
	W32::SelectObject(renderer->hFileDC,
			  W32::GetStockObject (HOLLOW_BRUSH) );
	W32::DeleteObject(hBrush);
    }
}

#ifndef DIRECT_WMF
static void
_bezier (DiaRenderer *self,
	 BezPoint *points,
	 int       numpoints,
	 Color    *colour,
	 gboolean  fill,
	 gboolean  closed)
{
    WmfRenderer *renderer = WMF_RENDERER (self);
    W32::HGDIOBJ hBrush /*, hBrOld */;
    W32::HPEN hPen;
    W32::COLORREF rgb = W32COLOR(colour);

    DIAG_NOTE(renderer, "_bezier n:%d %fx%f ...\n",
              numpoints, points->p1.x, points->p1.y);

    if (fill) {
      hBrush = W32::CreateSolidBrush(rgb);
      /*hBrOld =*/ W32::SelectObject(renderer->hFileDC, hBrush);
    } else {
      hPen = UsePen(renderer, colour);
    }

    W32::BeginPath (renderer->hFileDC);

    for (int i = 0; i < numpoints; ++i) {
        switch (points[i].type) {
	case BezPoint::BEZ_MOVE_TO :
	    W32::MoveToEx (renderer->hFileDC, SCX(points[i].p1.x), SCY(points[i].p1.y), NULL);
	    break;
	case BezPoint::BEZ_LINE_TO :
	    W32::LineTo (renderer->hFileDC, SCX(points[i].p1.x), SCY(points[i].p1.y));
	    break;
	case BezPoint::BEZ_CURVE_TO :
	  {
	    W32::POINT pts[3] = {
	      {SCX(points[i].p1.x), SCY(points[i].p1.y)},
	      {SCX(points[i].p2.x), SCY(points[i].p2.y)},
	      {SCX(points[i].p3.x), SCY(points[i].p3.y)}
	    };
	    W32::PolyBezierTo (renderer->hFileDC, pts, 3);
	  }
	  break;
	}
    }
    if (closed)
        W32::CloseFigure (renderer->hFileDC);
    W32::EndPath (renderer->hFileDC);
    if (fill) {
        W32::FillPath (renderer->hFileDC);
        W32::SelectObject(renderer->hFileDC,
                          W32::GetStockObject (HOLLOW_BRUSH) );
        W32::DeleteObject(hBrush);
    } else {
        W32::StrokePath (renderer->hFileDC);
        DonePen(renderer, hPen);
    }
}
#endif

static void
draw_bezier(DiaRenderer *self,
	    BezPoint *points,
	    int numpoints,
	    Color *colour)
{
#ifndef DIRECT_WMF
    _bezier(self, points, numpoints, colour, FALSE, FALSE);
#else
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
#endif
}

#ifndef DIRECT_WMF
/* not defined in compatibility layer */
static void
draw_beziergon (DiaRenderer *self,
		BezPoint *points, /* Last point must be same as first point */
		int numpoints,
		Color *fill,
		Color *stroke)
{
    if (fill)
	_bezier(self, points, numpoints, fill, TRUE, TRUE);
    if (stroke)
	_bezier(self, points, numpoints, stroke, FALSE, TRUE);
}
#endif


static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *colour)
{
  WmfRenderer *renderer = WMF_RENDERER (self);
  int len;
  W32::HGDIOBJ hOld;
  W32::COLORREF rgb = W32COLOR (colour);

  DIAG_NOTE (renderer, "draw_string %f,%f %s\n", pos->x, pos->y, text);

  W32::SetTextColor (renderer->hFileDC, rgb);

  switch (alignment) {
    case DIA_ALIGN_LEFT:
      W32::SetTextAlign (renderer->hFileDC, TA_LEFT+TA_BASELINE);
      break;
    case DIA_ALIGN_CENTRE:
      W32::SetTextAlign (renderer->hFileDC, TA_CENTER+TA_BASELINE);
      break;
    case DIA_ALIGN_RIGHT:
      W32::SetTextAlign (renderer->hFileDC, TA_RIGHT+TA_BASELINE);
      break;
    }

    /* work out size of first chunk of text */
    len = strlen (text);
    if (!len) {
      return; /* nothing to do */
    }

    hOld = W32::SelectObject (renderer->hFileDC, renderer->hFont);
    {
        // one way to go, but see below ...
        char* scp = NULL;
        /* convert from utf8 to active codepage */
        static char codepage[10];
#ifndef HAVE_LIBEMF
        sprintf (codepage, "CP%d", W32::GetACP ());
#else
        /* GetACP() not available in libEMF */
        sprintf (codepage, "CP1252");
#endif

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
        else // converson failed, write unicode
        {
            long wclen = 0;
            gunichar2* swc = g_utf8_to_utf16 (text, -1, NULL, &wclen, NULL);
            W32::TextOutW (renderer->hFileDC,
                           SCX(pos->x), SCY(pos->y),
                           reinterpret_cast<W32::LPCWSTR>(swc), wclen);
            g_free (swc);
        }
    }

    W32::SelectObject(renderer->hFileDC, hOld);
}

#ifndef HAVE_LIBEMF
static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage *image)
{
#ifdef DIRECT_WMF
    /* not yet supported in compatibility mode */
#else
    WmfRenderer *renderer = WMF_RENDERER (self);
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
#endif /* DIRECT_WMF */
}

static void
draw_rounded_rect (DiaRenderer *self,
	           Point *ul_corner, Point *lr_corner,
	           Color *fill, Color *stroke, real radius)
{
    WmfRenderer *renderer = WMF_RENDERER (self);

    DIAG_NOTE(renderer, "draw_rounded_rect %f,%f -> %f,%f %f\n",
              ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y, radius);

    if (fill) {
	W32::COLORREF rgb = W32COLOR(fill);
	W32::HGDIOBJ hBrush = W32::CreateSolidBrush(rgb);

	W32::SelectObject(renderer->hFileDC, hBrush);

	W32::RoundRect (renderer->hFileDC,
			SCX(ul_corner->x), SCY(ul_corner->y),
			SCX(lr_corner->x), SCY(lr_corner->y),
			SC(radius*2), SC(radius*2));

	W32::SelectObject (renderer->hFileDC,
			   W32::GetStockObject (HOLLOW_BRUSH) );
	W32::DeleteObject(hBrush);
    }
    if (stroke) {
	W32::HPEN hPen = UsePen (renderer, stroke);
	W32::RoundRect (renderer->hFileDC,
			SCX(ul_corner->x), SCY(ul_corner->y),
			SCX(lr_corner->x), SCY(lr_corner->y),
			SC(radius*2), SC(radius*2));
	DonePen(renderer, hPen);
    }
}
#endif

/* GObject boiler plate */
static void wmf_renderer_class_init (WmfRendererClass *klass);

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
wmf_renderer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  WmfRenderer *self = WMF_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      set_font (DIA_RENDERER (self),
                DIA_FONT (g_value_get_object (value)),
                self->font_height);
      break;
    case PROP_FONT_HEIGHT:
      set_font (DIA_RENDERER (self),
                self->font,
                g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
wmf_renderer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  WmfRenderer *self = WMF_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, self->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, self->font_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
wmf_renderer_finalize (GObject *object)
{
  WmfRenderer *renderer = WMF_RENDERER (object);

  g_clear_object (&renderer->font);

  if (renderer->hFont)
    W32::DeleteObject(renderer->hFont);
  if (renderer->pango_context)
    g_object_unref (renderer->pango_context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
wmf_renderer_class_init (WmfRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = wmf_renderer_set_property;
  object_class->get_property = wmf_renderer_get_property;
  object_class->finalize = wmf_renderer_finalize;

  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;
#ifdef G_OS_WIN32
  renderer_class->draw_object  = draw_object;
#endif

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_arc     = draw_arc;
#ifndef HAVE_LIBEMF
  renderer_class->fill_arc     = fill_arc;
#endif
  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_string  = draw_string;
#ifndef HAVE_LIBEMF
  renderer_class->draw_image   = draw_image;
#endif
  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;

  renderer_class->draw_bezier   = draw_bezier;
#ifndef DIRECT_WMF
  renderer_class->draw_beziergon = draw_beziergon;
#endif
#ifndef HAVE_LIBEMF
  renderer_class->draw_rounded_rect = draw_rounded_rect;
#endif
  /* other */
  renderer_class->is_capable_to = is_capable_to;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}

/* plug-in export api */
static gboolean
export_data(DiagramData *data, DiaContext *ctx,
	    const gchar *filename, const gchar *diafilename,
	    void* user_data)
{
    WmfRenderer *renderer;
    W32::HDC  file = NULL;
    W32::HDC refDC;
    DiaRectangle *extent;
    // gint len;
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
    scale /= 2; // Without this there can be some smallint overflow, dunno why

    refDC = W32::GetDC(NULL);

    bbox.right = (int)((data->extents.right - data->extents.left) * scale *
        100 * W32::GetDeviceCaps(refDC, HORZSIZE) / W32::GetDeviceCaps(refDC, HORZRES));
    bbox.bottom = (int)((data->extents.bottom - data->extents.top) * scale *
        100 * W32::GetDeviceCaps(refDC, VERTSIZE) / W32::GetDeviceCaps(refDC, VERTRES));

#if defined(HAVE_LIBEMF)
    FILE* ofile = g_fopen (filename, "w");
    if (ofile)
      file = CreateEnhMetaFileWithFILEA (refDC, ofile, &bbox, "Created with Dia/libEMF\0");
      // file now owned by the metafile DC
#else
    file = (W32::HDC)W32::CreateEnhMetaFile(
                    refDC, // handle to a reference device context
#  ifdef DIRECT_WMF
                    filename, // pointer to a filename string
#  else
                    NULL, // in memory
#  endif
                    &bbox, // pointer to a bounding rectangle
                    "Dia\0Diagram\0"); // pointer to an optional description string
#endif
    if (file == NULL) {
	dia_context_add_message_with_errno (ctx, errno, _("Can't open output file %s"),
					    dia_context_get_filename(ctx));
	return FALSE;
    }

    renderer = (WmfRenderer*)g_object_new(WMF_TYPE_RENDERER, NULL);

    renderer->hFileDC = file;
    renderer->sFileName = g_strdup(filename);
    renderer->ctx = ctx;
    if (user_data == (void*)1) {
	renderer->target_emf = TRUE;
	renderer->hPrintDC = 0;
#ifdef __PANGOWIN32_H__
        renderer->use_pango = TRUE;
#else
        renderer->use_pango = FALSE;
#endif
    } else {
        renderer->hPrintDC = (W32::HDC)user_data;
        renderer->use_pango = (user_data != NULL); // always for printing
    }

    DIAG_NOTE(renderer, "Saving %s:%s\n", renderer->target_emf ? "EMF" : "WMF", filename);

    /* printing is platform dependent */
#ifdef HAVE_LIBEMF
    renderer->platform_is_nt = TRUE;
#else
    renderer->platform_is_nt = (W32::GetVersion () < 0x80000000);
#endif
    extent = &data->extents;

    /* calculate offsets */
    if (!renderer->hPrintDC) {
	renderer->xoff = - data->extents.left;
	renderer->yoff = - data->extents.top;
	renderer->scale = scale;
    } else {
        int  ppc = (int)(W32::GetDeviceCaps (renderer->hPrintDC, PHYSICALWIDTH)
	            / ( data->paper.lmargin + data->paper.width + data->paper.rmargin));
	/* respect margins */
	renderer->margins.left   = (int)(ppc * data->paper.lmargin - W32::GetDeviceCaps (renderer->hPrintDC, PHYSICALOFFSETX));
	renderer->margins.top    = (int)(ppc * data->paper.tmargin - W32::GetDeviceCaps (renderer->hPrintDC, PHYSICALOFFSETY));
	renderer->margins.right  = (int)(W32::GetDeviceCaps (renderer->hPrintDC, PHYSICALWIDTH) - ppc * data->paper.rmargin);
	renderer->margins.bottom = (int)(W32::GetDeviceCaps (renderer->hPrintDC, PHYSICALHEIGHT) - ppc * data->paper.bmargin);

	renderer->xoff = - data->extents.left;
	renderer->yoff = - data->extents.top;
	renderer->scale = scale;
    }
    /* initialize placeable header */
    /* bounding box in twips 1/1440 of an inch */
    renderer->pmh.Key = 0x9AC6CDD7;
    renderer->pmh.Handle = 0;
    renderer->pmh.Left   = 0;
    renderer->pmh.Top    = 0;
    renderer->pmh.Right  = (gint16)SC(data->extents.right - data->extents.left);
    renderer->pmh.Bottom = (gint16)SC(data->extents.bottom - data->extents.top);
    renderer->pmh.Inch   = 1440 * 10;
    renderer->pmh.Reserved = 0;

    guint16 *ptr;
    renderer->pmh.Checksum = 0;
    for (ptr = (guint16 *)&renderer->pmh; ptr < (guint16 *)&(renderer->pmh.Checksum); ptr++)
        renderer->pmh.Checksum ^= *ptr;

#ifdef DIRECT_WMF
    /* write the placeable header */
    fwrite(&renderer->pmh, 1, 22 /* NOT: sizeof(PLACEABLEMETAHEADER) */, file->file);
#endif

    /* bounding box in device units */
    bbox.left = 0;
    bbox.top = 0;
    bbox.right = (int)SC(data->extents.right - data->extents.left);
    bbox.bottom = (int)SC(data->extents.bottom - data->extents.top);

    /* initialize drawing */
    W32::SetBkMode(renderer->hFileDC, TRANSPARENT);
    W32::SetMapMode(renderer->hFileDC, MM_TEXT);
#ifndef HAVE_LIBEMF
    W32::IntersectClipRect(renderer->hFileDC,
                           bbox.left, bbox.top,
                           bbox.right, bbox.bottom);
#endif

    /* write extents */
    DIAG_NOTE(renderer, "export_data extents %f,%f -> %f,%f\n",
              extent->left, extent->top, extent->right, extent->bottom);

    data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);

    g_object_unref(renderer);

    W32::ReleaseDC (NULL, refDC);

    return TRUE;
}

#ifdef G_OS_WIN32
static const gchar *wmf_extensions[] = { "wmf", NULL };
static DiaExportFilter wmf_export_filter = {
    N_("Windows Metafile"),
    wmf_extensions,
    export_data,
    NULL, /* user data */
    "wmf"
};
#endif

static const gchar *emf_extensions[] = { "emf", NULL };
static DiaExportFilter emf_export_filter = {
    N_("Enhanced Metafile"),
    emf_extensions,
    export_data,
    (void*)1, /* user data: urgh, reused as bool and pointer */
    "emf"
};


#ifdef G_OS_WIN32
static DiaObjectChange *
print_callback (DiagramData *data,
                const char  *filename,
                guint        flags,
                void        *user_data)
{
  /* Todo: get the context from caller */
  DiaContext *ctx = dia_context_new ("PrintGDI");

  if (!data)
    dia_context_add_message (ctx, _("Nothing to print"));
  else
    diagram_print_gdi (data, filename, ctx);

  dia_context_release (ctx);

  return NULL;
}

static DiaCallbackFilter cb_gdi_print = {
    "FilePrintGDI",
    N_("Print (GDI) ..."),
    "/InvisibleMenu/File/FilePrint",
    print_callback,
    NULL
};
#endif

/* --- dia plug-in interface --- */
extern "C" {
DIA_PLUGIN_CHECK_INIT
}

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
    if (!dia_plugin_info_init(info, "WMF",
                              _("WMF export filter"),
                              NULL, NULL))
        return DIA_PLUGIN_INIT_ERROR;

#ifdef G_OS_WIN32
    /*
     * On non Windows platforms this plug-in currently is only
     * useful at compile/develoment time. The output is broken
     * when processed by wmf_gdi.cpp ...
     */
    filter_register_export(&wmf_export_filter);
    filter_register_export(&emf_export_filter);

    filter_register_callback (&cb_gdi_print);
#elif defined(HAVE_LIBEMF)
    /* not sure if libEMF really saves EMF ;) */
    filter_register_export(&emf_export_filter);
#endif

    return DIA_PLUGIN_INIT_OK;
}
