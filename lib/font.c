/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * Font code completely reworked for the Pango conversion
 * Copyright (C) 2002 Cyrille Chepelov
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
#include <stdlib.h>
#include <string.h> /* strlen */
#include <time.h>

#include <pango/pango.h>
#undef PANGO_DISABLE_DEPRECATED /* pango_ft_get_context */
#include <gdk/gdk.h>
#include <gtk/gtk.h> /* just for gtk_get_default_language() */
#ifdef GDK_WINDOWING_WIN32
/* avoid namespace clashes caused by inclusion of windows.h */
#define WIN32_LEAN_AND_MEAN
#include <pango/pangowin32.h>
#endif
#include "font.h"
#include "message.h"
#include "textline.h"

static PangoContext *pango_context = NULL;

/**
 * DiaFont:
 *
 * Opaque type to represent a font
 */
struct _DiaFont {
  GObject parent_instance;

  PangoFontDescription *pfd;
  /* mutable */ char *legacy_name;

  /* there is a difference between Pango's font size and Dia's font height */
  /* Calculated  font_size is to make 'font_height = ascent + descent */
  /* The font_height is used as default line height, there used to be a hard-coded size = 0.7 * height  */
  /* before using pango_set_absolute_size() to overcome font size differences between renderers */
  double height;
  /* Need to load a font to query it's metrics */
  PangoFont *loaded;
  PangoFontMetrics *metrics;
};

G_DEFINE_TYPE (DiaFont, dia_font, G_TYPE_OBJECT)


/* This is the global factor that says what zoom factor is 100%.  It's
 * normally 20.0 (and likely to stay that way).  It is defined by how many
 * pixels one cm is represented as.
 */
static double global_zoom_factor = 20.0;


/*! DEPRECATED: there should be only one "measure context" */
static void
dia_font_push_context (PangoContext *pcontext)
{
  g_set_object (&pango_context, pcontext);
  pango_context_set_language (pango_context, gtk_get_default_language ());
}


/**
 * dia_font_get_context:
 *
 * Retrieve the current context (used for the font widget)
 */
PangoContext *
dia_font_get_context (void)
{
  if (pango_context == NULL) {
/* Maybe this one with pangocairo
     dia_font_push_context (pango_cairo_font_map_create_context (pango_cairo_font_map_get_default()));
 * but it gives:
     (lt-dia:30476): Pango-CRITICAL **: pango_renderer_draw_layout: assertion `PANGO_IS_RENDERER (renderer)' failed
 */
    if (gdk_display_get_default ())
      dia_font_push_context(gdk_pango_context_get());
    else {
#ifdef GDK_WINDOWING_WIN32
      dia_font_push_context(pango_win32_get_context ());
#elif defined(GDK_WINDOWING_QUARTZ) || defined(GDK_WINDOWING_X11)
      dia_font_push_context (pango_font_map_create_context (pango_cairo_font_map_get_default ()));
#else
      g_warning ("dia_font_get_context() : not font context w/o display. Crashing soon.");
#endif
    }
  }
  return pango_context;
}


/* dia centimetres to pango device units */
static int
dcm_to_pdu (double dcm)
{
  return dcm * global_zoom_factor * PANGO_SCALE;
}


/* pango device units to dia centimetres */
static double
pdu_to_dcm (int pdu)
{
  return (double) pdu / (global_zoom_factor * PANGO_SCALE);
}


static void
dia_font_finalize (GObject *object)
{
  DiaFont *font = DIA_FONT (object);

  g_clear_pointer (&font->pfd, pango_font_description_free);
  g_clear_pointer (&font->metrics, pango_font_metrics_unref);

  g_clear_object (&font->loaded);

  G_OBJECT_CLASS (dia_font_parent_class)->finalize (object);
}


static void
dia_font_class_init (DiaFontClass* klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = dia_font_finalize;
}


static void
dia_font_init (DiaFont *font)
{
}


static void
dia_pfd_set_height(PangoFontDescription *pfd, double height)
{
  /* ONLY place for the magic factor! */
  g_return_if_fail (height > 0.0);
  pango_font_description_set_absolute_size (pfd, dcm_to_pdu (height) * 0.8);
}


/*!
 * In Dia a font is usually referred to by it's (line-) height, not it's size.
 *
 * This methods "calculates" the latter from the former. This used to be some magic factor of 0.7 which did not
 * solve the resolution dependencance of the former calculation. In fact there is new magic factor now because
 * really calculating the font size from the height would involve two font loads which seem to be two expensive.
 */
static void
_dia_font_adjust_size (DiaFont *font, double height, gboolean recalc_alwways)
{

  if (font->height != height || !font->metrics || recalc_alwways) {
    PangoFont *loaded;

    dia_pfd_set_height (font->pfd, height);
    /* need to load a font to get it's metrics */
    loaded = font->loaded;
    font->loaded = pango_context_load_font (dia_font_get_context (), font->pfd);

    g_clear_object (&loaded);

    g_clear_pointer (&font->metrics, pango_font_metrics_unref);

    /* caching metrics */
    font->metrics = pango_font_get_metrics (font->loaded, NULL);
    font->height = height;
  }
}


/**
 * dia_font_new:
 *
 * Get a font matching family,style,height.
 */
DiaFont *
dia_font_new (const char *family, DiaFontStyle style, double height)
{
  DiaFont* font = dia_font_new_from_style(style, height);
  gboolean changed;

  changed = family != NULL && g_strcmp0 (pango_font_description_get_family (font->pfd), family) != 0;
  pango_font_description_set_family(font->pfd, family);

  if (changed) {
    _dia_font_adjust_size (font, font->height, TRUE);
  }

  return font;
}


static void
dia_pfd_set_family (PangoFontDescription* pfd, DiaFontFamily fam)
{
  switch (fam) {
    case DIA_FONT_SANS :
      pango_font_description_set_family (pfd, "sans");
      break;
    case DIA_FONT_SERIF :
      pango_font_description_set_family (pfd, "serif");
      break;
    case DIA_FONT_MONOSPACE :
      pango_font_description_set_family (pfd, "monospace");
      break;
    case DIA_FONT_FAMILY_ANY:
    default :
      /* Pango does _not_ allow fonts without a name (or at least they are not useful) */
      pango_font_description_set_family (pfd, "sans");
      break;
  }
}


static void
dia_pfd_set_weight (PangoFontDescription* pfd, DiaFontWeight fw)
{
  switch (fw) {
  case DIA_FONT_ULTRALIGHT :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_ULTRALIGHT);
    break;
  case DIA_FONT_LIGHT :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_LIGHT);
    break;
  case DIA_FONT_WEIGHT_NORMAL :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_NORMAL);
    break;
  case DIA_FONT_MEDIUM : /* Pango doesn't have this, but
                            'intermediate values are possible' */
    pango_font_description_set_weight(pfd, 500);
    break;
  case DIA_FONT_DEMIBOLD : /* Pango doesn't have this, ... */
    pango_font_description_set_weight(pfd, 600);
    break;
  case DIA_FONT_BOLD :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_BOLD);
    break;
  case DIA_FONT_ULTRABOLD :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_ULTRABOLD);
    break;
  case DIA_FONT_HEAVY :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_HEAVY);
    break;
  default :
    g_assert_not_reached();
  }
}


static void
dia_pfd_set_slant (PangoFontDescription* pfd, DiaFontSlant fo)
{
  switch (fo) {
  case DIA_FONT_NORMAL :
    pango_font_description_set_style(pfd,PANGO_STYLE_NORMAL);
    break;
  case DIA_FONT_OBLIQUE :
    pango_font_description_set_style(pfd,PANGO_STYLE_OBLIQUE);
    break;
  case DIA_FONT_ITALIC :
    pango_font_description_set_style(pfd,PANGO_STYLE_ITALIC);
    break;
  default :
    g_assert_not_reached();
  }
}


/**
 * dia_font_new_from_style:
 *
 * Get a font matching style. This is the preferred method to
 * create default fonts within objects.
 */
DiaFont *
dia_font_new_from_style (DiaFontStyle style, double height)
{
  DiaFont *retval;
  /* in the future we could establish Dia's own default font
   * matching to be as (font-)system independent as possible.
   * For now fall back to Pangos configuration --hb
   */
  PangoFontDescription *pfd = pango_font_description_new ();
  dia_pfd_set_family (pfd, DIA_FONT_STYLE_GET_FAMILY (style));
  dia_pfd_set_weight (pfd, DIA_FONT_STYLE_GET_WEIGHT (style));
  dia_pfd_set_slant (pfd, DIA_FONT_STYLE_GET_SLANT (style));
  dia_pfd_set_height (pfd, height);

  retval = g_object_new (DIA_TYPE_FONT, NULL);
  retval->pfd = pfd;
  _dia_font_adjust_size (retval, height, FALSE);
  retval->legacy_name = NULL;

  return retval;
}


DiaFont *
dia_font_copy (DiaFont *font)
{
  if (!font) {
    return NULL;
  }

  return dia_font_new (dia_font_get_family (font),
                       dia_font_get_style (font),
                       dia_font_get_height (font));
}


/**
 * dia_font_get_style:
 *
 * Retrieves the style of the font
 */
DiaFontStyle
dia_font_get_style (DiaFont *font)
{
  guint style;

  static int weight_map[] = {
    DIA_FONT_ULTRALIGHT, DIA_FONT_LIGHT,
    DIA_FONT_WEIGHT_NORMAL, /* intentionaly ==0 */
    DIA_FONT_MEDIUM, DIA_FONT_DEMIBOLD, /* not yet in Pango */
    DIA_FONT_BOLD, DIA_FONT_ULTRABOLD, DIA_FONT_HEAVY
  };

  PangoStyle pango_style = pango_font_description_get_style(font->pfd);
  PangoWeight pango_weight = pango_font_description_get_weight(font->pfd);

  g_return_val_if_fail (PANGO_WEIGHT_ULTRALIGHT <= pango_weight &&
                        pango_weight <= PANGO_WEIGHT_HEAVY,
                        DIA_FONT_NORMAL);
  g_return_val_if_fail (PANGO_WEIGHT_ULTRALIGHT == 200, DIA_FONT_NORMAL);
  g_return_val_if_fail (PANGO_WEIGHT_NORMAL == 400, DIA_FONT_NORMAL);
  g_return_val_if_fail (PANGO_WEIGHT_BOLD == 700, DIA_FONT_NORMAL);

  style = weight_map[(pango_weight - PANGO_WEIGHT_ULTRALIGHT) / 100];
  style |= (pango_style << 2);

  return style;
}


/**
 * dia_font_get_family:
 *
 * Retrieves the family of the font. Caller must NOT free.
 */
const char *
dia_font_get_family (DiaFont *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  return pango_font_description_get_family(font->pfd);
}


/**
 * dia_font_get_description:
 *
 * Acessor for the PangoFontDescription
 */
const PangoFontDescription *
dia_font_get_description (DiaFont *font)
{
  g_return_val_if_fail (font != NULL, NULL);

  return font->pfd;
}


/**
 * dia_font_get_height:
 *
 * Retrieves the height of the font
 */
double
dia_font_get_height (DiaFont *font)
{
  g_return_val_if_fail (font != NULL, 0.0);

  return font->height;
}


/**
 * dia_font_get_size:
 *
 * Delivers the size of the font
 */
double
dia_font_get_size (DiaFont *font)
{
  g_return_val_if_fail (font != NULL, 0.0);

  if (!pango_font_description_get_size_is_absolute (font->pfd)) {
    g_warning ("dia_font_get_size() : no absolute size");
  }

  return pdu_to_dcm (pango_font_description_get_size (font->pfd));
}


/**
 * dia_font_set_height:
 *
 * Change the height inside a font record.
 */
void
dia_font_set_height (DiaFont* font, double height)
{
  _dia_font_adjust_size (font, height, FALSE);
}


/**
 * dia_font_get_psfontname:
 *
 * FIXME: what do we do with this, actually ?
 *
 * Name lives for as long as the DiaFont lives.
 */
const char *
dia_font_get_psfontname (DiaFont *font)
{
  /* This hack corrects a couple fonts that were misnamed in
   * earlier versions of Dia.   See bug #477079.
   */
  const char *fontname = dia_font_get_legacy_name (font);

  if (!fontname) {
    return NULL;
  }

  if (g_strcmp0 (fontname, "NewCenturySchoolbook-Roman") == 0) {
    return "NewCenturySchlbk-Roman";
  } else if (g_strcmp0 (fontname, "NewCenturySchoolbook-Italic") == 0) {
    return "NewCenturySchlbk-Italic";
  } else if (g_strcmp0 (fontname, "NewCenturySchoolbook-Bold") == 0) {
    return "NewCenturySchlbk-Bold";
  } else if (g_strcmp0 (fontname, "NewCenturySchoolbook-BoldItalic") == 0) {
    return "NewCenturySchlbk-BoldItalic";
  }

  return fontname;
}


/**
 * dia_font_set_any_family:
 *
 * Changes the family of an existing font to any family
 *
 * The name is system configuration dependent, but font files are portable nowadays.
 */
void
dia_font_set_any_family (DiaFont *font, const char *family)
{
  gboolean changed;

  g_return_if_fail (font != NULL);

  changed = g_strcmp0 (pango_font_description_get_family (font->pfd), family) != 0;
  pango_font_description_set_family (font->pfd, family);
  if (changed) {
    /* force recalculation on name change */
    _dia_font_adjust_size (font, font->height, TRUE);
  }

  g_clear_pointer (&font->legacy_name, g_free);
}


/**
 * dia_font_set_family:
 *
 * Changes the family of an existing font to one of the three standard families
 */
void
dia_font_set_family (DiaFont *font, DiaFontFamily family)
{
  g_return_if_fail (font != NULL);

  dia_pfd_set_family (font->pfd,family);

  g_clear_pointer (&font->legacy_name, g_free);
}


/**
 * dia_font_set_weight:
 *
 * Changes the weight of an existing font
 */
void
dia_font_set_weight (DiaFont *font, DiaFontWeight weight)
{
  DiaFontWeight old_weight = DIA_FONT_STYLE_GET_WEIGHT (dia_font_get_style (font));

  g_return_if_fail (font != NULL);

  dia_pfd_set_weight(font->pfd,weight);

  if (old_weight != weight) {
    _dia_font_adjust_size (font, font->height, TRUE);
  }
}


/**
 * dia_font_set_slant:
 *
 * Changes the slant of an existing font
 */
void
dia_font_set_slant (DiaFont *font, DiaFontSlant slant)
{
  DiaFontSlant old_slant = DIA_FONT_STYLE_GET_SLANT(dia_font_get_style(font));
  g_return_if_fail(font != NULL);
  dia_pfd_set_slant(font->pfd,slant);
  if (slant != old_slant)
    _dia_font_adjust_size (font, font->height, TRUE);
}


typedef struct _WeightName {
  DiaFontWeight fw;
  const char *name;
} WeightName;
static const WeightName weight_names[] = {
  { DIA_FONT_ULTRALIGHT, "200"},
  { DIA_FONT_LIGHT,"300"},
  { DIA_FONT_WEIGHT_NORMAL,"normal"},
  { DIA_FONT_WEIGHT_NORMAL,"400"},
  { DIA_FONT_MEDIUM, "500"},
  { DIA_FONT_DEMIBOLD, "600"},
  { DIA_FONT_BOLD, "700"},
  { DIA_FONT_ULTRABOLD, "800"},
  { DIA_FONT_HEAVY, "900"},
  { 0, NULL}
};


/**
 * dia_font_get_weight_string:
 *
 * returns a static string suitable for SVG
 */
const char *
dia_font_get_weight_string (DiaFont *font)
{
  const WeightName *p;
  DiaFontWeight fw = DIA_FONT_STYLE_GET_WEIGHT (dia_font_get_style (font));

  for (p = weight_names; p->name != NULL; ++p) {
    if (p->fw == fw) {
      return p->name;
    }
  }

  return "normal";
}


/**
 * dia_font_set_weight_from_string:
 *
 * uses an SVG style string
 */
void
dia_font_set_weight_from_string (DiaFont *font, const char *weight)
{
  DiaFontWeight fw = DIA_FONT_WEIGHT_NORMAL;
  const WeightName *p;

  for (p = weight_names; p->name != NULL; ++p) {
    if (0 == strncmp (weight, p->name, 8)) {
      fw = p->fw;
      break;
    }
  }

  dia_font_set_weight (font, fw);
}


typedef struct _SlantName {
  DiaFontSlant fo;
  const char *name;
} SlantName;
static const SlantName slant_names[] = {
  { DIA_FONT_NORMAL, "normal" },
  { DIA_FONT_OBLIQUE, "oblique" },
  { DIA_FONT_ITALIC, "italic" },
  { 0, NULL}
};


/**
 * dia_font_get_slant_string:
 *
 * returns a static string suitable for SVG
 */
const char *
dia_font_get_slant_string (DiaFont *font)
{
  const SlantName *p;
  DiaFontSlant fo = DIA_FONT_STYLE_GET_SLANT (dia_font_get_style (font));

  for (p = slant_names; p->name != NULL; ++p) {
    if (p->fo == fo) {
      return p->name;
    }
  }

  return "normal";
}


/**
 * dia_font_set_slant_from_string:
 *
 * uses an SVG style string
 */
void
dia_font_set_slant_from_string (DiaFont* font, const char *obli)
{
  DiaFontSlant fo = DIA_FONT_NORMAL;
  const SlantName *p;

  for (p = slant_names; p->name != NULL; ++p) {
    if (0 == strncmp (obli, p->name, 8)) {
      fo = p->fo;
      break;
    }
  }

  dia_font_set_slant (font, fo);
}


/* ************************************************************************ */
/* Non-scaled versions of the utility routines                              */
/* ************************************************************************ */

/**
 * dia_font_string_width:
 *
 * Get the width of the string with the given font in cm
 */
double
dia_font_string_width (const char *string, DiaFont *font, double height)
{
  double result = 0;

  if (string && *string) {
    TextLine *text_line = text_line_new (string, font, height);
    result = text_line_get_width (text_line);
    text_line_destroy (text_line);
  }

  return result;
}


/**
 * dia_font_ascent:
 *
 * Get the ascent of this string in cm (a positive number).
 */
double
dia_font_ascent (const char *string, DiaFont *font, double height)
{
  if (font->metrics) {
    double ascent = pdu_to_dcm (pango_font_metrics_get_ascent (font->metrics));
    return ascent * (height / font->height);
  } else {
    /* previous, _expensive_ but string specific way */
    TextLine *text_line = text_line_new (string, font, height);
    double result = text_line_get_ascent (text_line);
    text_line_destroy (text_line);
    return result;
  }
}


/**
 * dia_font_descent:
 *
 * Get the descent of the font in cm (a positive number)
 */
double
dia_font_descent (const char *string, DiaFont *font, double height)
{
  if (font->metrics) {
    double descent = pdu_to_dcm (pango_font_metrics_get_descent (font->metrics));
    return descent * (height / font->height);
  } else {
    /* previous, _expensive_ but string specific way */
    TextLine *text_line = text_line_new (string, font, height);
    double result = text_line_get_descent (text_line);
    text_line_destroy (text_line);
    return result;
  }
}


/**
 * dia_font_build_layout:
 *
 * prepares a layout of the text, in font 'font'.
 */
PangoLayout *
dia_font_build_layout (const char *string, DiaFont *font, double height)
{
  PangoLayout *layout;
  PangoAttrList *list;
  PangoAttribute *attr;
  guint length;
  PangoFontDescription *pfd;
  double factor;

  layout = pango_layout_new (dia_font_get_context ());

  length = string ? strlen (string) : 0;
  pango_layout_set_text (layout, string, length);

  list = pango_attr_list_new ();

  pfd = pango_font_description_copy (font->pfd);
  /* account for difference between size and height as well as between font height and given one */
  factor = dia_font_get_size (font) / dia_font_get_height (font);
  pango_font_description_set_absolute_size (pfd, dcm_to_pdu (height) * factor);
  attr = pango_attr_font_desc_new (pfd);
  pango_font_description_free (pfd);

  attr->start_index = 0;
  attr->end_index = length;
  pango_attr_list_insert (list, attr); /* eats attr */

  pango_layout_set_attributes (layout, list);
  pango_attr_list_unref (list);

  pango_layout_set_indent (layout,0);
  pango_layout_set_justify (layout, FALSE);
  pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);

  return layout;
}


/**
 * get_string_offsets:
 * @iter: The #PangoLayoutIter to count characters in.
 * @offsets: The place to return the offsets
 * @n_offsets: The place to return the number of offsets
 *
 * Find the offsets of the individual letters in the iter and place them
 * in an array.
 *
 * This currently assumes only one run per iter, which is all we can input.
 */
static void
get_string_offsets(PangoLayoutIter *iter, double **offsets, int *n_offsets)
{
  int i;
  PangoLayoutLine *line = pango_layout_iter_get_line (iter);
  PangoGlyphItem *item;
  PangoGlyphString *string;

  if(0 == line->length)
  {
    *n_offsets = 0;
    return;
  }
  item = (PangoGlyphItem*)line->runs->data;
  string = item->glyphs;

  *n_offsets = string->num_glyphs;
  *offsets = g_new (double, *n_offsets);

  for (i = 0; i < string->num_glyphs; i++) {
    PangoGlyphGeometry geom = string->glyphs[i].geometry;

    (*offsets)[i] = pdu_to_dcm(geom.width) / global_zoom_factor;
  }
}

/*
 * Copy the offsets into a storage object, adjusting for exaggerated size.
 */
static void
get_layout_offsets(PangoLayoutLine *line, PangoLayoutLine **layout_line)
{
  /* Some info not copied. */
  GSList *layout_runs = NULL;
  GSList *runs = line->runs;

  *layout_line = g_new0(PangoLayoutLine, 1);

  /* A LayoutLine contains a GSList runs of PangoGlyphItems.
   * Each PangoGlyphItem contains an array glyphs of PangoGlyphString.
   * This array is run->item->num_chars long?
   * Each PangoGlyphString contains an array glyphs of PangoGlyphInfo.
   * This array is run->glyphs[i].num_glyphs long.
   */
  for (; runs != NULL; runs = g_slist_next(runs)) {
    PangoGlyphItem *run = (PangoGlyphItem *) runs->data;
    PangoGlyphItem *layout_run = g_new0(PangoGlyphItem, 1);
    int j;
    /* Make single pointer */
    PangoGlyphString *glyph_string = run->glyphs;
    PangoGlyphString *layout_glyph_string;

    layout_run->glyphs = g_new0(PangoGlyphString, 1);
    layout_glyph_string = layout_run->glyphs;

    layout_glyph_string->num_glyphs = glyph_string->num_glyphs;
    layout_glyph_string->glyphs =
      g_new0(PangoGlyphInfo, glyph_string->num_glyphs);
    for (j = 0; j < layout_glyph_string->num_glyphs; j++) {
      PangoGlyphInfo *info = &glyph_string->glyphs[j];
      PangoGlyphInfo *layout_info = &layout_glyph_string->glyphs[j];
      layout_info->geometry.width = info->geometry.width;
      layout_info->geometry.x_offset = info->geometry.x_offset;
      layout_info->geometry.y_offset = info->geometry.y_offset;
    }
    layout_runs = g_slist_append(layout_runs, layout_run);
  }
  (*layout_line)->runs = layout_runs;
}


/**
 * dia_font_get_sizes:
 * @string: text to measure
 * @font: the font to use
 * @height: the font height to use
 * @width: (out): width of @string
 * @ascent: (out): width of @ascent
 * @descent: (out): width of @descent
 * @n_offsets: (out): number of @layout_offsets
 * @layout_offsets: (out): offsets of @string
 *
 * Get size information for the given string, font and height.
 *
 * Returns: an array of offsets of the individual glyphs in the layout.
 *
 * Since: dawn-of-time
 */
double *
dia_font_get_sizes (const char       *string,
                    DiaFont          *font,
                    double            height,
                    double           *width,
                    double           *ascent,
                    double           *descent,
                    int              *n_offsets,
                    PangoLayoutLine **layout_offsets)
{
  PangoLayout* layout;
  PangoLayoutIter* iter;
  double top, bline, bottom;
  const char *non_empty_string;
  PangoRectangle ink_rect,logical_rect;
  double *offsets = NULL; /* avoid: 'offsets' may be used uninitialized in this function */

  /* We need some reasonable ascent/descent values even for empty strings. */
  if (string == NULL || string[0] == '\0') {
    non_empty_string = "XjgM149";
  } else {
    non_empty_string = string;
  }
  layout = dia_font_build_layout(non_empty_string, font, height * global_zoom_factor);

  /* Only one line here ? */
  iter = pango_layout_get_iter(layout);

  pango_layout_iter_get_line_extents(iter, &ink_rect, &logical_rect);

  top = pdu_to_dcm(logical_rect.y) / global_zoom_factor;
  bottom = pdu_to_dcm(logical_rect.y + logical_rect.height) / global_zoom_factor;
  bline = pdu_to_dcm(pango_layout_iter_get_baseline(iter)) / global_zoom_factor;

  get_string_offsets(iter, &offsets, n_offsets);
  get_layout_offsets(pango_layout_get_line(layout, 0), layout_offsets);

  /* FIXME: the above assumption of 'one line' is wrong. At least calculate the overall width correctly
   * to avoid text overflowing its box, like in bug #482585 */
  while (pango_layout_iter_next_line (iter)) {
    PangoRectangle more_ink_rect, more_logical_rect;

    pango_layout_iter_get_line_extents(iter, &more_ink_rect, &more_logical_rect);
    if (more_logical_rect.width > logical_rect.width)
      logical_rect.width = more_logical_rect.width;
    /* also calculate for the ink rect (true space needed for drawing the glyphs) */
    if (more_ink_rect.width > ink_rect.width)
      ink_rect.width = more_ink_rect.width;
  }

  pango_layout_iter_free(iter);
  g_clear_object (&layout);

  *ascent = bline-top;
  *descent = bottom-bline;
  if (non_empty_string != string) {
    *width = 0.0;
  } else {
    /* take the bigger rectangle to avoid cutting of any part of the string */
    /* also respect that Dia is assuming to start from zero */
    int full_width = ink_rect.width + ink_rect.x;
    *width = pdu_to_dcm(logical_rect.width > full_width ? logical_rect.width : full_width) / global_zoom_factor;
  }
  return offsets;
}


/*
 * Compatibility with older files out of pre Pango Time.
 * Make old files look as similar as possible
 * List should be kept alphabetically sorted by oldname, in case of
 * duplicates the one with the preferred newname comes first.
 *
 * FIXME: DIA_FONT_FAMILY_ANY in the list below does mean noone knows better
 *
 * The PostScript names can be found on page 139 of
 * http://partners.adobe.com/public/developer/en/ps/PS3010and3011.Supplement.pdf
 *
 * Note that these are not strictly the Adobe names, as a few were used
 * incorrectly and are kept for backwards compatibility.  The latin-1
 * postscript renderer mangles these.
 *
 * Some additional fairly standard fonts for asian scripts are also added.
 */
static struct _legacy_font {
  char         *oldname;
  char         *newname;
  DiaFontStyle  style;   /* the DIA_FONT_FAMILY() is used as falback only */
} legacy_fonts[] = {
  { "AvantGarde-Book",        "AvantGarde", DIA_FONT_SERIF },
  { "AvantGarde-BookOblique", "AvantGarde", DIA_FONT_SERIF | DIA_FONT_OBLIQUE },
  { "AvantGarde-Demi",        "AvantGarde", DIA_FONT_SERIF | DIA_FONT_DEMIBOLD },
  { "AvantGarde-DemiOblique", "AvantGarde", DIA_FONT_SERIF | DIA_FONT_OBLIQUE | DIA_FONT_DEMIBOLD },
  { "Batang", "Batang", DIA_FONT_FAMILY_ANY },
  { "Bookman-Demi",        "Bookman Old Style", DIA_FONT_SERIF | DIA_FONT_DEMIBOLD },
  { "Bookman-DemiItalic",  "Bookman Old Style", DIA_FONT_SERIF | DIA_FONT_DEMIBOLD | DIA_FONT_ITALIC },
  { "Bookman-Light",       "Bookman Old Style", DIA_FONT_SERIF | DIA_FONT_LIGHT },
  { "Bookman-LightItalic", "Bookman Old Style", DIA_FONT_SERIF | DIA_FONT_LIGHT | DIA_FONT_ITALIC },
  { "BousungEG-Light-GB", "BousungEG-Light-GB", DIA_FONT_FAMILY_ANY },
  { "Courier",             "monospace", DIA_FONT_MONOSPACE },
  { "Courier",             "Courier New", DIA_FONT_MONOSPACE },
  { "Courier-Bold",        "monospace", DIA_FONT_MONOSPACE | DIA_FONT_BOLD },
  { "Courier-Bold",        "Courier New", DIA_FONT_MONOSPACE | DIA_FONT_BOLD },
  { "Courier-BoldOblique", "monospace", DIA_FONT_MONOSPACE | DIA_FONT_ITALIC | DIA_FONT_BOLD },
  { "Courier-BoldOblique", "Courier New", DIA_FONT_MONOSPACE | DIA_FONT_ITALIC | DIA_FONT_BOLD },
  { "Courier-Oblique",     "monospace", DIA_FONT_MONOSPACE | DIA_FONT_ITALIC},
  { "Courier-Oblique",     "Courier New", DIA_FONT_MONOSPACE | DIA_FONT_ITALIC },
  { "Dotum", "Dotum", DIA_FONT_FAMILY_ANY },
  { "GBZenKai-Medium", "GBZenKai-Medium", DIA_FONT_FAMILY_ANY },
  { "GothicBBB-Medium", "GothicBBB-Medium", DIA_FONT_FAMILY_ANY },
  { "Gulim", "Gulim", DIA_FONT_FAMILY_ANY },
  { "Headline", "Headline", DIA_FONT_FAMILY_ANY },
  { "Helvetica",             "sans", DIA_FONT_SANS },
  { "Helvetica",             "Arial", DIA_FONT_SANS },
  { "Helvetica-Bold",        "sans", DIA_FONT_SANS | DIA_FONT_BOLD },
  { "Helvetica-Bold",        "Arial", DIA_FONT_SANS | DIA_FONT_BOLD },
  { "Helvetica-BoldOblique", "sans", DIA_FONT_SANS | DIA_FONT_BOLD | DIA_FONT_ITALIC },
  { "Helvetica-BoldOblique", "Arial", DIA_FONT_SANS | DIA_FONT_BOLD | DIA_FONT_ITALIC },
  { "Helvetica-Narrow",             "Arial Narrow", DIA_FONT_SANS | DIA_FONT_MEDIUM },
  { "Helvetica-Narrow-Bold",        "Arial Narrow", DIA_FONT_SANS | DIA_FONT_DEMIBOLD },
  { "Helvetica-Narrow-BoldOblique", "Arial Narrow", DIA_FONT_SANS | DIA_FONT_MEDIUM | DIA_FONT_OBLIQUE },
  { "Helvetica-Narrow-Oblique",     "Arial Narrow", DIA_FONT_SANS | DIA_FONT_MEDIUM | DIA_FONT_OBLIQUE },
  { "Helvetica-Oblique",     "sans", DIA_FONT_SANS | DIA_FONT_ITALIC },
  { "Helvetica-Oblique",     "Arial", DIA_FONT_SANS | DIA_FONT_ITALIC },
  { "MOESung-Medium", "MOESung-Medium", DIA_FONT_FAMILY_ANY },
  { "NewCenturySchoolbook-Bold",       "Century Schoolbook SWA", DIA_FONT_SERIF | DIA_FONT_BOLD },
  { "NewCenturySchoolbook-BoldItalic", "Century Schoolbook SWA", DIA_FONT_SERIF | DIA_FONT_BOLD | DIA_FONT_ITALIC },
  { "NewCenturySchoolbook-Italic",     "Century Schoolbook SWA", DIA_FONT_SERIF | DIA_FONT_ITALIC },
  { "NewCenturySchoolbook-Roman",      "Century Schoolbook SWA", DIA_FONT_SERIF },
  { "Palatino-Bold",       "Palatino", DIA_FONT_FAMILY_ANY | DIA_FONT_BOLD },
  { "Palatino-BoldItalic", "Palatino", DIA_FONT_FAMILY_ANY | DIA_FONT_BOLD | DIA_FONT_ITALIC },
  { "Palatino-Italic",     "Palatino", DIA_FONT_FAMILY_ANY | DIA_FONT_ITALIC },
  { "Palatino-Roman",      "Palatino", DIA_FONT_FAMILY_ANY },
  { "Ryumin-Light",      "Ryumin", DIA_FONT_FAMILY_ANY | DIA_FONT_LIGHT },
  { "ShanHeiSun-Light",  "ShanHeiSun", DIA_FONT_FAMILY_ANY | DIA_FONT_LIGHT },
  { "Song-Medium",       "Song-Medium", DIA_FONT_FAMILY_ANY | DIA_FONT_MEDIUM },
  { "Symbol",           "Symbol", DIA_FONT_SANS | DIA_FONT_MEDIUM },
  { "Times-Bold",       "serif", DIA_FONT_SERIF | DIA_FONT_BOLD },
  { "Times-Bold",       "Times New Roman", DIA_FONT_SERIF | DIA_FONT_BOLD },
  { "Times-BoldItalic", "serif", DIA_FONT_SERIF | DIA_FONT_ITALIC | DIA_FONT_BOLD },
  { "Times-BoldItalic", "Times New Roman", DIA_FONT_SERIF | DIA_FONT_ITALIC | DIA_FONT_BOLD },
  { "Times-Italic",     "serif", DIA_FONT_SERIF | DIA_FONT_ITALIC },
  { "Times-Italic",     "Times New Roman", DIA_FONT_SERIF | DIA_FONT_ITALIC },
  { "Times-Roman",      "serif", DIA_FONT_SERIF },
  { "Times-Roman",      "Times New Roman", DIA_FONT_SERIF },
  { "ZapfChancery-MediumItalic", "Zapf Calligraphic 801 SWA", DIA_FONT_SERIF | DIA_FONT_MEDIUM },
  { "ZapfDingbats", "Zapf Calligraphic 801 SWA", DIA_FONT_SERIF },
  { "ZenKai-Medium", "ZenKai", DIA_FONT_FAMILY_ANY | DIA_FONT_MEDIUM },
};


/**
 * dia_font_new_from_legacy_name:
 * @name: the legacy name
 *
 * Given a legacy name as stored until Dia-0.90 construct
 * a new DiaFont which is as similar as possible
 *
 * Since: dawn-of-time
 */
DiaFont *
dia_font_new_from_legacy_name (const char* name)
{
  /* do NOT translate anything here !!! */
  DiaFont *retval = NULL;
  struct _legacy_font* found = NULL;
  double height = 1.0;
  int i;

  for (i = 0; i < G_N_ELEMENTS (legacy_fonts); i++) {
    if (!g_strcmp0 (name, legacy_fonts[i].oldname)) {
      found = &legacy_fonts[i];
      break;
    }
  }
  if (found) {
    retval = dia_font_new (found->newname, found->style, height);
    retval->legacy_name = found->oldname;
  } else {
    /* We tried our best, let Pango complain */
    retval = dia_font_new (name, DIA_FONT_WEIGHT_NORMAL, height);
    retval->legacy_name = NULL;
  }

  return retval;
}


/**
 * dia_font_get_legacy_name:
 *
 * Get a simple font name from a font.
 *
 * Name will be valid for the duration of the DiaFont* lifetime.
 */
const char *
dia_font_get_legacy_name (DiaFont *font)
{
  const char* matched_name = NULL;
  const char* family;
  DiaFontStyle style;
  int i;

  /* if we have loaded it from an old file, use the old name */
  if (font->legacy_name) {
    return font->legacy_name;
  }

  family = dia_font_get_family (font);
  style = dia_font_get_style (font);
  for (i = 0; i < G_N_ELEMENTS (legacy_fonts); i++) {
    if (0 == g_ascii_strcasecmp (legacy_fonts[i].newname, family)) {
      /* match weight and slant */
      DiaFontStyle st = legacy_fonts[i].style;
      if ((DIA_FONT_STYLE_GET_SLANT (style) | DIA_FONT_STYLE_GET_WEIGHT (style))
          == (DIA_FONT_STYLE_GET_SLANT (st) | DIA_FONT_STYLE_GET_WEIGHT (st))) {
        return legacy_fonts[i].oldname; /* exact match */
      } else if (0 == (DIA_FONT_STYLE_GET_SLANT (st) | DIA_FONT_STYLE_GET_WEIGHT (st))) {
        matched_name = legacy_fonts[i].oldname;
        /* 'unmodified' font, continue matching */
      }
    }
  }

  return matched_name ? matched_name : "Courier";
}

