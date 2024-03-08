/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * wpg-import.c -- WordPerfect Graphics import plug-in for Dia
 * Copyright (C) 2014 Hans Breuer <hans@breuer.org>
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

#include <glib.h>
#include <glib/gstdio.h>

#include "diaimportrenderer.h"
#include "wpg_defs.h"
#include "dia_image.h"
#include "diacontext.h"
#include "message.h" /* just dia_log_message() */
#include "dia-layer.h"
#include "font.h"

typedef struct _WpgImportRenderer  WpgImportRenderer;

/*!
 * \defgroup WpgImport WordPerfect Graphics Import
 * \ingroup ImportFilters
 * \brief WordPerfect (5.1) Graphics Import
 *
 * The WordPerfect Graphics import is limited to the last WPG format with
 * open documentation. As such it is mostly useful for historical graphics;)
 */

/*!
 * \brief Renderer for WordPerfect Graphics import
 * \ingroup WpgImport
 * \extends _DiaImportRenderer
 */
struct _WpgImportRenderer {
  DiaImportRenderer parent_instance;

  WPGStartData Box;
  WPGFillAttr  FillAttr;
  WPGLineAttr  LineAttr;
  WPGColorRGB* pPal;

  Color stroke;
  Color fill;
  Color text_color;
  DiaAlignment text_align;
};

typedef struct _WpgImportRendererClass WpgImportRendererClass;
struct _WpgImportRendererClass {
  DiaRendererClass parent_class;
};

static GType wpg_import_renderer_get_type (void);

G_DEFINE_TYPE(WpgImportRenderer, wpg_import_renderer, DIA_TYPE_IMPORT_RENDERER);

#define WPG_TYPE_IMPORT_RENDERER (wpg_import_renderer_get_type ())
#define WPG_IMPORT_RENDERER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WPG_TYPE_IMPORT_RENDERER, WpgImportRenderer))

static void
wpg_import_renderer_class_init (WpgImportRendererClass *klass)
{
  /* anything to initialize? */
}
static void
wpg_import_renderer_init (WpgImportRenderer *self)
{
  /* anything to initialize? */
  self->LineAttr.Type = WPG_LA_SOLID;
  self->FillAttr.Type = WPG_FA_HOLLOW;
}

static Point *
_make_points (WpgImportRenderer *ren,
	      WPGPoint          *pts,
	      int                num)
{
  int    i;
  int    ofs = ren->Box.Height;
  Point *points = g_new (Point, num);

  for (i = 0; i < num; ++i) {
    points[i].x = pts[i].x / WPU_PER_DCM;
    points[i].y = (ofs - pts[i].y) / WPU_PER_DCM;
  }
  return points;
}
static Point *
_make_rect (WpgImportRenderer *ren,
	    WPGPoint          *pts)
{
  /* WPG has position and size, Dia wants top-left and bottom-right */
  int       ofs = ren->Box.Height;
  Point    *points = g_new (Point, 2);
  WPGPoint *pos = pts;
  WPGPoint *wh = pts+1;
  /* also it bottom vs. top position */
  points[0].x = pos->x / WPU_PER_DCM;
  points[1].y = (ofs - pos->y) / WPU_PER_DCM;
  points[1].x = points[0].x + (wh->x / WPU_PER_DCM);
  points[0].y = points[1].y - (wh->y / WPU_PER_DCM);
  return points;
}

static void
_do_ellipse (WpgImportRenderer *ren, WPGEllipse* pEll)
{
  int    h = ren->Box.Height;
  Point center;
  center.x = pEll->x / WPU_PER_DCM;
  center.y = (h - pEll->y) / WPU_PER_DCM;

  if (fabs(pEll->EndAngle - pEll->StartAngle) < 360) {
    /* WPG arcs are counter-clockwise so ensure that end is bigger than start */
    real arcEnd = pEll->EndAngle;
    if (arcEnd < pEll->StartAngle)
      arcEnd += 360;
    if (ren->LineAttr.Type != WPG_LA_NONE)
      dia_renderer_draw_arc (DIA_RENDERER (ren),
                             &center,
                             2 * pEll->rx / WPU_PER_DCM,
                             2 * pEll->ry / WPU_PER_DCM,
                             pEll->StartAngle,
                             arcEnd,
                             &ren->stroke);
    if (ren->FillAttr.Type != WPG_FA_HOLLOW)
      dia_renderer_fill_arc (DIA_RENDERER (ren),
                             &center,
                             2 * pEll->rx / WPU_PER_DCM,
                             2 * pEll->ry / WPU_PER_DCM,
                             pEll->StartAngle,
                             arcEnd,
                             &ren->fill);
  } else {
    dia_renderer_draw_ellipse (DIA_RENDERER (ren),
                               &center,
                               2 * pEll->rx / WPU_PER_DCM,
                               2 * pEll->ry / WPU_PER_DCM,
                               (ren->FillAttr.Type != WPG_FA_HOLLOW) ? &ren->fill : NULL,
                               (ren->LineAttr.Type != WPG_LA_NONE) ? &ren->stroke : NULL);
  }
}

static void
_do_polygon (WpgImportRenderer *ren, Point *points, int iNum)
{
  g_return_if_fail (iNum > 2);
  if (ren->LineAttr.Type == WPG_LA_NONE && ren->FillAttr.Type == WPG_FA_HOLLOW)
    return; /* nothing to do */
  dia_renderer_draw_polygon (DIA_RENDERER (ren),
                             points,
                             iNum,
                             (ren->FillAttr.Type != WPG_FA_HOLLOW) ? &ren->fill : NULL,
                             (ren->LineAttr.Type != WPG_LA_NONE) ? &ren->stroke : NULL);
}

static void
_do_rect (WpgImportRenderer *ren, Point *points)
{
  if (ren->LineAttr.Type != WPG_LA_NONE || ren->FillAttr.Type != WPG_FA_HOLLOW) {
    dia_renderer_draw_rect (DIA_RENDERER (ren),
                            &points[0],
                            &points[1],
                            (ren->FillAttr.Type != WPG_FA_HOLLOW) ? &ren->fill : NULL,
                            (ren->LineAttr.Type != WPG_LA_NONE) ? &ren->stroke : NULL);
  }
}

static void
_do_bezier (WpgImportRenderer *ren, WPGPoint *pts, int iNum)
{
  int num_points = (iNum + 2) / 3;
  int i;
  int ofs = ren->Box.Height;
  BezPoint *bps;

  g_return_if_fail (num_points > 1);

  bps = g_alloca (num_points * sizeof(BezPoint));

  bps[0].type = BEZ_MOVE_TO;
  bps[0].p1.x = pts[0].x / WPU_PER_DCM;
  bps[0].p1.y = (ofs - pts[0].y) / WPU_PER_DCM;
  for (i = 1; i < num_points; ++i) {
    bps[i].type = BEZ_CURVE_TO;
    /* although declared as unsigned (WORD) these values have to be treaded
     * as signed, because they can move in both directions.
     */
    bps[i].p1.x =        (gint16)pts[i*3-2].x  / WPU_PER_DCM;
    bps[i].p1.y = (ofs - (gint16)pts[i*3-2].y) / WPU_PER_DCM;
    bps[i].p2.x =        (gint16)pts[i*3-1].x  / WPU_PER_DCM;
    bps[i].p2.y = (ofs - (gint16)pts[i*3-1].y) / WPU_PER_DCM;
    bps[i].p3.x =        (gint16)pts[i*3  ].x  / WPU_PER_DCM;
    bps[i].p3.y = (ofs - (gint16)pts[i*3  ].y) / WPU_PER_DCM;
  }
  /* XXX: should we fold this calls into one? What's closing a WPG PolyCurve? */
  if (ren->LineAttr.Type != WPG_LA_NONE) {
    dia_renderer_draw_bezier (DIA_RENDERER (ren),
                              bps,
                              num_points,
                              &ren->stroke);
  }
  if (ren->FillAttr.Type != WPG_FA_HOLLOW) {
    dia_renderer_draw_beziergon (DIA_RENDERER (ren),
                                 bps,
                                 num_points,
                                 &ren->fill,
                                 NULL);
  }
}

/*
 * Import (under construction)
 */
static void
import_object(DiaRenderer* self, DiagramData *dia,
              WPG_Type type, int iSize, guchar* pData)
{
  WpgImportRenderer *renderer = WPG_IMPORT_RENDERER(self);
  WPGPoint* pts = NULL;
  gint16* pInt16 = NULL;
  int    iNum = 0;
  gint32 iPre51 = 0;
  Point *points = NULL;

  switch (type) {
    case WPG_LINE:
      iNum = 2;
      pts = (WPGPoint*)pData;
      points = _make_points (renderer, pts, iNum);
      dia_renderer_draw_line (self, &points[0], &points[1], &renderer->stroke);
      break;
    case WPG_POLYLINE:
      pInt16 = (gint16*)pData;
      iNum = pInt16[0];
      pts = (WPGPoint*)(pData + sizeof(gint16));
      points = _make_points (renderer, pts, iNum);
      dia_renderer_draw_polyline (self, points, iNum, &renderer->stroke);
      break;
    case WPG_RECTANGLE:
      points = _make_rect (renderer, (WPGPoint*)pData);
      _do_rect (renderer, points);
      break;
    case WPG_POLYGON:
      pInt16 = (gint16*)pData;
      iNum = pInt16[0];
      pts = (WPGPoint*)(pData + sizeof(gint16));
      points = _make_points (renderer, pts, iNum);
      _do_polygon (renderer, points, iNum);
      break;
    case WPG_ELLIPSE:
      {
        WPGEllipse* pEll = (WPGEllipse*)pData;
        _do_ellipse (renderer, pEll);
      }
      break;
    case WPG_POLYCURVE:
      iPre51 = *((gint32*)pData);
      pInt16 = (gint16*)pData;
      iNum = pInt16[2];
      pts = (WPGPoint*)(pData + 3*sizeof(gint16));
      dia_log_message ("WPG POLYCURVE Num pts %d Pre51 %d", iNum, iPre51);
      _do_bezier (renderer, pts, iNum);
      break;
    case WPG_FILLATTR:
    case WPG_LINEATTR:
    case WPG_COLORMAP:
    case WPG_MARKERATTR:
    case WPG_POLYMARKER:
    case WPG_TEXTSTYLE:
    case WPG_START:
    case WPG_END:
    case WPG_OUTPUTATTR:
    case WPG_STARTFIGURE:
    case WPG_STARTCHART:
    case WPG_PLANPERFECT:
    case WPG_STARTWPG2:
    case WPG_POSTSCRIPT1:
    case WPG_POSTSCRIPT2:
      /* these are no objects, silence GCC */
      break;
    case WPG_BITMAP1:
    case WPG_TEXT:
    case WPG_BITMAP2:
      /* these objects are handled directly below, silence GCC */
      break;
    case WPG_GRAPHICSTEXT2:
    case WPG_GRAPHICSTEXT3:
      /* these objects actually might get implemented some day, silence GCC */
      break;
    default:
      g_warning ("Unknown type %i", type);
      break;
  } /* switch */

  g_clear_pointer (&points, g_free);

  DIAG_NOTE (g_message ("Type %d Num pts %d Size %d", type, iNum, iSize));
}

static void
_make_stroke (WpgImportRenderer *ren)
{
  WPGColorRGB c;

  g_return_if_fail (ren->pPal != NULL);
  c = ren->pPal[ren->LineAttr.Color];
  ren->stroke.red = c.r / 255.0;
  ren->stroke.green = c.g / 255.0;
  ren->stroke.blue = c.b / 255.0;
  ren->stroke.alpha = 1.0;
  switch (ren->LineAttr.Type) {
    case WPG_LA_SOLID:
      dia_renderer_set_linestyle (DIA_RENDERER (ren),
                                  DIA_LINE_STYLE_SOLID,
                                  0.0);
      break;
    case WPG_LA_MEDIUMDASH:
      dia_renderer_set_linestyle (DIA_RENDERER (ren),
                                  DIA_LINE_STYLE_DASHED,
                                  0.66);
      break;
    case WPG_LA_SHORTDASH:
      dia_renderer_set_linestyle (DIA_RENDERER (ren),
                                  DIA_LINE_STYLE_DASHED,
                                  0.33);
      break;
    case WPG_LA_DASHDOT:
      dia_renderer_set_linestyle (DIA_RENDERER (ren),
                                  DIA_LINE_STYLE_DASH_DOT,
                                  1.0);
      break;
    case WPG_LA_DASHDOTDOT:
      dia_renderer_set_linestyle (DIA_RENDERER (ren),
                                  DIA_LINE_STYLE_DASH_DOT_DOT,
                                  1.0);
      break;
    case WPG_LA_DOTS:
      dia_renderer_set_linestyle (DIA_RENDERER (ren),
                                  DIA_LINE_STYLE_DOTTED,
                                  1.0);
      break;
    default:
      g_warning ("Unknown type %i", ren->LineAttr.Type);
      break;
  }

  dia_renderer_set_linewidth (DIA_RENDERER (ren),
                              ren->LineAttr.Width / WPU_PER_DCM);
}

static void
_make_fill (WpgImportRenderer *ren)
{
  WPGColorRGB c;

  g_return_if_fail (ren->pPal != NULL);
  c = ren->pPal[ren->FillAttr.Color];
  ren->fill.red = c.r / 255.0;
  ren->fill.green = c.g / 255.0;
  ren->fill.blue = c.b / 255.0;
  ren->fill.alpha = 1.0;
}

static gboolean
_render_bmp (WpgImportRenderer *ren, WPGBitmap2 *bmp, FILE *f, int len)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, /* no alpha */
				      8, /* bits per sample: nothing else */
				      bmp->Width, bmp->Height);
  guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
  gboolean bRet = TRUE;
#define PUT_PIXEL(pix) \
  { WPGColorRGB rgb = ren->pPal[pix]; \
    pixels[0] = rgb.r; pixels[1] = rgb.g; pixels[2] = rgb.b; \
    pixels[3] = 0xff; pixels+=4; }
#define REPEAT_LINE(sth) \
  { memcpy (pixels + 4 * bmp->Width, pixels, 4 * bmp->Width); pixels+=(4 * bmp->Width); }
  if (pixbuf) {
    DiaImage *image;
    int    h = ren->Box.Height;
    Point pt = { bmp->Left / WPU_PER_DCM, (h - bmp->Top) / WPU_PER_DCM };
    if (bmp->Depth != 8)
      g_warning ("WPGBitmap2 unsupported depth %d", bmp->Depth);
    while (len > 0) {
      guint8 b, rc, i;
      if (1 == fread(&b, sizeof(guint8), 1, f)) {
	if (b & 0x80) {
	  rc = b & 0x7F;
	  if (rc != 0) { /* Read the next BYTE and repeat it RunCount times */
	    bRet &= (1 == fread(&b, sizeof(guint8), 1, f)); len--;
	    for (i = 0; i < rc; ++i)
	      PUT_PIXEL (b)
	  } else { /* Repeat the value FFh RunCount times */
	    bRet &= (1 == fread (&rc, sizeof(guint8), 1, f)); len--;
	    for (i = 0; i < rc; ++i)
	      PUT_PIXEL (0xFF)
	  }
	} else {
	  rc = b & 0x7F;
	  if (rc != 0) { /* The next RunCount BYTEs are read literally */
	    for (i = 0; i < rc; ++i) {
	      bRet &= (1 == fread (&b, sizeof(guint8), 1, f)); len--;
	      PUT_PIXEL (b)
	    }
	  } else { /* Repeat the previous scan line RunCount times */
	    bRet &= (1 == fread (&rc, sizeof(guint8), 1, f)); len--;
	    for (i = 0; i < rc; ++i)
	      REPEAT_LINE (0xFF);
	  }
	}
      }
      len--;
    }
    image = dia_image_new_from_pixbuf (pixbuf);
    if (bmp->Right - bmp->Left == 0 || bmp->Bottom - bmp->Top == 0) {
      dia_renderer_draw_image (DIA_RENDERER (ren),
                               &pt,
                               bmp->Xdpi / 2.54,
                               bmp->Ydpi / 2.54,
                               image);
    } else { /* real WPGBitmap2 */
      dia_renderer_draw_image (DIA_RENDERER (ren),
                               &pt,
                               (bmp->Right - bmp->Left) / WPU_PER_DCM,
                               (bmp->Bottom - bmp->Top) / WPU_PER_DCM,
                               image);
    }

    g_clear_object (&pixbuf);
    g_clear_object (&image);

    return bRet;
  }
#undef PUT_PIXEL
#undef REPEAT_LINE
  return FALSE;
}

static void
_do_textstyle (WpgImportRenderer *ren, WPGTextStyle *ts)
{
  WPGColorRGB c;

  /* change some text properties directly, store the res for draw_string */
  real height;
  DiaFont *font;

  g_return_if_fail(sizeof(WPGTextStyle) == 22);

  c = ren->pPal[ts->Color];
  ren->text_color.red = c.r / 255.0;
  ren->text_color.green = c.g / 255.0;
  ren->text_color.blue = c.b / 255.0;
  ren->text_color.alpha = 1.0;

  ren->text_align = ts->XAlign == 0 ?
                      DIA_ALIGN_LEFT : ts->XAlign == 1 ?
                        DIA_ALIGN_CENTRE : DIA_ALIGN_RIGHT;
  /* select font */
  height = ts->Height / WPU_PER_DCM;
  if (ts->Font == 0x0DF0)
    font = dia_font_new_from_style (DIA_FONT_MONOSPACE, height);
  else if (ts->Font == 0x1950)
    font = dia_font_new_from_style (DIA_FONT_SERIF, height);
  else if (ts->Font == 0x1150)
    font = dia_font_new_from_style (DIA_FONT_SANS, height);
  else /* any is not advices */
    font = dia_font_new_from_style (DIA_FONT_SANS, height);

  dia_renderer_set_font (DIA_RENDERER (ren), font, height);

  g_clear_object (&font);
}

static void
_render_text (WpgImportRenderer *ren, WPGPoint *pos, gchar *text)
{
  Point pt;
  pt.x = pos->x / WPU_PER_DCM;
  pt.y = (ren->Box.Height - pos->y ) / WPU_PER_DCM;

  dia_renderer_draw_string (DIA_RENDERER (ren),
                            text,
                            &pt,
                            ren->text_align,
                            &ren->text_color);
}

gboolean
import_data (const gchar *filename, DiagramData *dia, DiaContext *ctx, void* user_data)
{
  FILE* f;
  gboolean bRet = TRUE;
  WPGHead8 rh;

  f = g_fopen(filename, "rb");

  if (NULL == f) {
    dia_context_add_message(ctx, _("Couldn't open: '%s' for reading.\n"), filename);
    bRet = FALSE;
  }

  /* check header */
  if (bRet) {
    WPGFileHead fhead;
    bRet = ( (1 == fread(&fhead, sizeof(WPGFileHead), 1, f))
            && fhead.fid[0] == 255 && fhead.fid[1] == 'W'
            && fhead.fid[2] == 'P' && fhead.fid[3] == 'C'
            && (1 == fhead.MajorVersion) && (0 == fhead.MinorVersion));
    if (!bRet)
      dia_context_add_message(ctx, _("File: %s type/version unsupported.\n"), filename);
  }

  if (bRet) {
    int iSize;
    gint16 i16, iNum16;
    guint8 i8;
    WpgImportRenderer *ren = g_object_new (WPG_TYPE_IMPORT_RENDERER, NULL);

    ren->pPal = g_new0(WPGColorRGB, 256);

    DIAG_NOTE(g_message("Parsing: %s ", filename));

    do {
      if (1 == fread(&rh, sizeof(WPGHead8), 1, f)) {
        if (rh.Size < 0xFF)
          iSize = rh.Size;
        else {
          bRet = (1 == fread(&i16, sizeof(guint16), 1, f));
          if (0x8000 & i16) {
            DIAG_NOTE(g_print("Large Object: hi:lo %04X", (int)i16));
            iSize = (0x7FFF & i16) << 16;
            /* Reading large objects involves major uglyness. Instead of getting
             * one size, as implied by "Encyclopedia of Graphics File Formats",
             * it would require putting together small chunks of data to one large
             * object. The criteria when to stop isn't absolutely clear.
             */
            bRet = (1 == fread(&i16, sizeof(guint16), 1, f));
            DIAG_NOTE(g_print("Large Object: %d\n", (int)i16));
            iSize += (unsigned short)i16;
#if 0
            /* Ignore this large objec part */
            fseek(f, iSize, SEEK_CUR);
            continue;
#endif
          }
          else
            iSize = i16;
        }
      } else
        iSize = 0;

      /* DIAG_NOTE(g_message("Type %d Size %d", rh.Type, iSize)); */
      if (iSize > 0) {
        switch (rh.Type) {
        case WPG_FILLATTR:
          bRet = (1 == fread(&ren->FillAttr, sizeof(WPGFillAttr), 1, f));
	  _make_fill (ren);
          break;
        case WPG_LINEATTR:
          bRet = (1 == fread(&ren->LineAttr, sizeof(WPGLineAttr), 1, f));
	  _make_stroke (ren);
          break;
        case WPG_START:
          bRet = (1 == fread(&ren->Box, sizeof(WPGStartData), 1, f));
          break;
        case WPG_STARTWPG2:
          /* not sure if this is the right thing to do */
          bRet &= (1 == fread(&i8, sizeof(gint8), 1, f));
          bRet &= (1 == fread(&i16, sizeof(gint16), 1, f));
          DIAG_NOTE(g_message("Ignoring tag WPG_STARTWPG2, Size %d\n Type? %d Size? %d",
                    iSize, (int)i8, i16));
          fseek(f, iSize - 3, SEEK_CUR);
          break;
        case WPG_LINE:
        case WPG_POLYLINE:
        case WPG_RECTANGLE:
        case WPG_POLYGON:
        case WPG_POLYCURVE:
        case WPG_ELLIPSE:
          {
            guchar* pData;

            pData = g_new(guchar, iSize);
            bRet = (iSize == (int)fread(pData, 1, iSize, f));
            import_object(DIA_RENDERER(ren), dia, rh.Type, iSize, pData);
            g_clear_pointer (&pData, g_free);
          }
          break;
        case WPG_COLORMAP:
          bRet &= (1 == fread(&i16, sizeof(gint16), 1, f));
          bRet &= (1 == fread(&iNum16, sizeof(gint16), 1, f));
          if (iNum16 != (gint16)((iSize - 2) / sizeof(WPGColorRGB)))
            g_warning("WpgImporter : colormap size/header mismatch.");
          if (i16 >= 0 && i16 <= iSize) {
            bRet &= (iNum16 == (int)fread(&ren->pPal[i16], sizeof(WPGColorRGB), iNum16, f));
          }
          else {
            g_warning("WpgImporter : start color map index out of range.");
            bRet &= (iNum16 == (int)fread(ren->pPal, sizeof(WPGColorRGB), iNum16, f));
          }
          break;
	case WPG_BITMAP1:
          {
            WPGBitmap1 bmp;
            WPGBitmap2 bmp2 = { 0, };

            bRet &= (1 == fread(&bmp, sizeof(WPGBitmap1), 1, f));
	    /* transfer data to bmp2 struct */
	    bmp2.Width = bmp.Width;
	    bmp2.Height = bmp.Height;
	    bmp2.Depth = bmp.BitsPerPixel; /* really? */
	    bmp2.Xdpi = bmp.Xdpi;
	    bmp2.Ydpi = bmp.Ydpi;

	    if (!_render_bmp (ren, &bmp2, f, iSize - sizeof(WPGBitmap1)))
	      fseek(f, iSize - sizeof(WPGBitmap2), SEEK_CUR);
          }
	  break;
        case WPG_BITMAP2:
          {
            WPGBitmap2 bmp;

            bRet &= (1 == fread(&bmp, sizeof(WPGBitmap2), 1, f));
	    if (!_render_bmp (ren, &bmp, f, iSize - sizeof(WPGBitmap2)))
	      fseek(f, iSize - sizeof(WPGBitmap2), SEEK_CUR);
          }
          break;
	case WPG_TEXT:
	  {
	    guint16 len;
	    WPGPoint pos;
	    gchar *text;

	    bRet &= (1 == fread(&len, sizeof(guint16), 1, f));
	    bRet &= (1 == fread(&pos, sizeof(WPGPoint), 1, f));
	    text = g_alloca (len+1);
	    bRet &= (len == fread(text, 1, len, f));
	    text[len] = 0;
	    if (len > 0)
	      _render_text (ren, &pos, text);
	  }
	  break;
	case WPG_TEXTSTYLE:
	  {
	    WPGTextStyle ts;
	    bRet &= (1 == fread(&ts, sizeof(WPGTextStyle), 1, f));
	    _do_textstyle (ren, &ts);
	  }
	  break;
        default:
          fseek(f, iSize, SEEK_CUR);
          dia_context_add_message (ctx, _("Unknown WPG type %d size %d."),
				   (int)rh.Type, iSize);
        } /* switch */
      } else { /* if iSize */
        if (WPG_END != rh.Type) {
          dia_context_add_message (ctx, _("Size 0 on WPG type %d, expecting WPG_END\n"),
				   (int)rh.Type);
	  bRet = FALSE;
	}
      }
    }
    while ((iSize > 0) && (bRet));

    if (!bRet) {
      dia_context_add_message (ctx,
                               _("Unexpected end of file. WPG type %d, size %d.\n"),
                               rh.Type,
                               iSize);
    }

    g_clear_pointer (&ren->pPal, g_free);

    /* transfer to diagram data */
    {
      DiaObject *objs = dia_import_renderer_get_objects (DIA_RENDERER(ren));
      if (objs) {
        dia_layer_add_object (dia_diagram_data_get_active_layer (dia), objs);
      } else {
        dia_context_add_message (ctx, _("Empty WPG file?"));
        bRet = FALSE;
      }
    }
    g_clear_object (&ren);
  } /* bRet */

  if (f) {
    fclose (f);
  }

  return bRet;
}
