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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strlen */

#include <pango/pango.h>

#include "font.h"

static PangoContext* pango_context = NULL;
real global_size_one = 20.0;

void
dia_font_init(PangoContext* pcontext)
{
    pango_context = pcontext;
}

    /* dia centimetres to pango device units */
static gint
dcm_to_pdu(real dcm) { return dcm * global_size_one * PANGO_SCALE; }
    /* pango device units to dia centimetres */
static real
pdu_to_dcm(gint pdu) { return (real)pdu / (global_size_one * PANGO_SCALE); }

static void dia_font_class_init(DiaFontClass* class);
static void dia_font_finalize(GObject* object);
static void dia_font_init_instance(DiaFont*);

GType
dia_font_get_type (void)
{
    static GType object_type = 0;

    if (!object_type) {
        static const GTypeInfo object_info =
            {
                sizeof (DiaFontClass),
                (GBaseInitFunc) NULL,
                (GBaseFinalizeFunc) NULL,
                (GClassInitFunc) dia_font_class_init, /* class_init */
                NULL,           /* class_finalize */
                NULL,           /* class_data */
                sizeof (DiaFont),
                0,              /* n_preallocs */
                (GInstanceInitFunc)dia_font_init_instance
            };
        object_type = g_type_register_static (G_TYPE_OBJECT,
                                              "DiaFont",
                                              &object_info, 0);
    }  
    return object_type;
}
static gpointer parent_class;

static void
dia_font_class_init(DiaFontClass* klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);
  object_class->finalize = dia_font_finalize;
}

static void 
dia_font_init_instance(DiaFont* font)
{
  GObject *gobject = G_OBJECT(font); 
}

/* Fills everything *BUT* family. */
static DiaFont*
dia_font_fill_descriptor(PangoFontDescription* pfd, Style style, real height)
{
    DiaFont* retval;
    PangoStyle pstyle;
    PangoWeight pweight;
    
    pango_font_description_set_variant(pfd,PANGO_VARIANT_NORMAL);
    pango_font_description_set_stretch(pfd,PANGO_STRETCH_NORMAL);
    pango_font_description_set_size(pfd, dcm_to_pdu(height) );

    dia_font_dia_style_to_pango(style, &pstyle, &pweight);
    pango_font_description_set_style(pfd, pstyle);
    pango_font_description_set_weight(pfd, pweight);
    
    retval = DIA_FONT(g_type_create_instance(dia_font_get_type()));
    retval->pfd = pfd;
    dia_font_ref(retval);
    return retval;
}

DiaFont*
dia_font_new(const char *family, Style style, real height)
{
    PangoFontDescription* pfd = pango_font_description_new();
    pango_font_description_set_family(pfd,family);
    return dia_font_fill_descriptor(pfd,style,height);
}

DiaFont*
dia_font_new_from_style(DiaFontStyle style, real height)
{
  DiaFont* retval;
  /* in the future we could establish Dia's own default font
   * matching to be as (font-)system independent as possible.
   * For now fall back to Pangos configuration --hb
   */
  PangoFontDescription* pfd = pango_font_description_new();
  switch (DIA_FONT_STYLE_GET_FAMILY(style)) {
  case DIA_FONT_SANS :
    pango_font_description_set_family(pfd, "sans");
    break;
  case DIA_FONT_SERIF :
    pango_font_description_set_family(pfd, "serif");
    break;
  case DIA_FONT_MONOSPACE :
    pango_font_description_set_family(pfd, "monospace");
    break;
  default :
    /* FIXME: does Pango allow font_descs without a name */
    ;
  }
  switch (DIA_FONT_STYLE_GET_WEIGHT(style)) {
  case DIA_FONT_ULTRALIGHT :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_ULTRALIGHT);
    break;
  case DIA_FONT_LIGHT :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_LIGHT);
    break;
  case DIA_FONT_MEDIUM : /* Pango doesn't have this */
  case DIA_FONT_WEIGHT_NORMAL :
    pango_font_description_set_weight(pfd, PANGO_WEIGHT_NORMAL);
    break;
  case DIA_FONT_DEMIBOLD : /* Pango doesn't have this */
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
  switch (DIA_FONT_STYLE_GET_OBLIQUITY(style)) {
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
  pango_font_description_set_size(pfd, dcm_to_pdu(height) );

  retval = DIA_FONT(g_type_create_instance(dia_font_get_type()));
  retval->pfd = pfd;
  dia_font_ref(retval);
  return retval;
}

DiaFont*
dia_font_new_from_static(const char *family, Style style, real height)
{
    PangoFontDescription* pfd = pango_font_description_new();
    pango_font_description_set_family_static(pfd,family);
    return dia_font_fill_descriptor(pfd,style,height);
}

DiaFont* dia_font_copy(const DiaFont* font)
{
    if (!font) return NULL;
    return dia_font_new(dia_font_get_family(font),
                            dia_font_get_style(font),
                            dia_font_get_height(font));
}

void
dia_font_finalize(GObject* object)
{
    DiaFont* font;
    font = DIA_FONT(object);
    
    if (font->pfd) pango_font_description_free(font->pfd);
}

DiaFont* dia_font_ref(DiaFont* font)
{
    g_object_ref(G_OBJECT(font));
    return font;
}

void dia_font_unref(DiaFont* font)
{ 
    g_object_unref(G_OBJECT(font));
}

Style dia_font_get_style(const DiaFont* font)
{
    PangoStyle pstyle = pango_font_description_get_style(font->pfd);
    PangoWeight pweight = pango_font_description_get_weight(font->pfd);
    
    return dia_font_pango_style_weight_to_dia(pstyle, pweight);
}

G_CONST_RETURN char*
dia_font_get_family(const DiaFont* font)
{
    return pango_font_description_get_family(font->pfd);
}

real
dia_font_get_height(const DiaFont* font)
{
    return pdu_to_dcm(pango_font_description_get_size(font->pfd));
}

void
dia_font_set_height(DiaFont* font, real height)
{
    pango_font_description_set_size(font->pfd, dcm_to_pdu(height));    
}


G_CONST_RETURN char*
dia_font_get_psfontname(const DiaFont *font)
{
        /* FIXME: this will very likely not work ! */
    return dia_font_get_legacy_name(font);
}

static G_CONST_RETURN char*
dia_font_style_to_legacy_name(Style style) 
{
    switch(style) {
        case STYLE_BOLD_ITALIC:
            return "-BoldOblique";
        case STYLE_BOLD:
            return "-Bold";
        case STYLE_ITALIC:
            return "-Oblique";
        case STYLE_NORMAL: /* fall-through */
        default:
            return "";
    }
}

G_CONST_RETURN char*
dia_font_get_legacy_name(const DiaFont* font) 
{
    if (font->legacy_name) return font->legacy_name;
    
    ((DiaFont*)font)->legacy_name =
        g_strconcat(dia_font_get_family(font),
                    dia_font_style_to_legacy_name(dia_font_get_style(font)),
                    NULL);
    return font->legacy_name;
}

/* Conversion between our style and pango style/weight */
int 
dia_font_pango_style_weight_to_dia(int style, int weight)
{
  return style + (3*(weight-200)/100) + 1;
}

void 
dia_font_dia_style_to_pango(int          style, 
                            PangoStyle  *pango_style, 
                            PangoWeight *pango_weight) 
{
  if (style == 0 || style > STYLE_HEAVY_ITALIC) {
    *pango_style = PANGO_STYLE_NORMAL;
    *pango_weight = PANGO_WEIGHT_NORMAL;
    return;
  }
  *pango_style = (style-1)%3;
  *pango_weight = (style-1)%3*100+200;
}


/* ************************************************************************ */
/* Non-scaled versions of the utility routines                              */
/* ************************************************************************ */

real
dia_font_string_width(const char* string, DiaFont *font, real height)
{
    return dia_font_scaled_string_width(string,font,height,global_size_one);
}

real
dia_font_ascent(const char* string, DiaFont* font, real height)
{
    return dia_font_scaled_ascent(string,font,height,global_size_one);
}

real dia_font_descent(const char* string, DiaFont* font, real height)
{
    return dia_font_scaled_descent(string,font,height,global_size_one);
}

PangoLayout*
dia_font_build_layout(const char* string, DiaFont* font, real height)
{
    PangoLayout* layout;
    PangoAttrList* list;
    PangoAttribute* attr;
    guint length;

    dia_font_set_height(font,height);
    layout = pango_layout_new(pango_context);

    length = string ? strlen(string) : 0;
    pango_layout_set_text(layout,string,length);
        
    list = pango_attr_list_new();

    attr = pango_attr_font_desc_new(font->pfd);
    attr->start_index = 0;
    attr->end_index = length;
    pango_attr_list_insert(list,attr); /* eats attr */
    
    pango_layout_set_attributes(layout,list);
    pango_attr_list_unref(list);

    pango_layout_set_indent(layout,0);
    pango_layout_set_justify(layout,FALSE);
    pango_layout_set_alignment(layout,PANGO_ALIGN_LEFT);
    
    return layout;
}

/* ************************************************************************ */
/* scaled versions of the utility routines                                  */
/* ************************************************************************ */

void
dia_font_set_nominal_zoom_factor(real size_one)
{ global_size_one = size_one; }



real
dia_font_scaled_string_width(const char* string, DiaFont *font, real height,
                            real zoom_factor)
{
    int lw,lh;
    real result;
    PangoLayout* layout = dia_font_scaled_build_layout(string, font,
                                                      height, zoom_factor);
    pango_layout_get_size(layout,&lw,&lh);
    g_object_unref(G_OBJECT(layout));
    
    result = pdu_to_dcm(lw);
    return result;
}

static gboolean
dia_font_vertical_extents(const char* string, DiaFont* font,
                          real height, real zoom_factor,
                          guint line_no,
                          real* top, real* baseline, real* bottom)
{
    PangoRectangle ink_rect,logical_rect;
    PangoLayout* layout;
    PangoLayoutIter* iter;
    guint i;

    layout = dia_font_scaled_build_layout(string, font,
                                          height, zoom_factor);
    iter = pango_layout_get_iter(layout);
    for (i = 0; i < line_no; ++i) {
       if (!pango_layout_iter_next_line(iter)) {
           pango_layout_iter_free(iter);
           g_object_unref(G_OBJECT(layout));
           return FALSE;
       }
    }
    
    pango_layout_iter_get_line_extents(iter,&ink_rect,&logical_rect);
    *top = pdu_to_dcm(logical_rect.y);
    *bottom = pdu_to_dcm(logical_rect.y + logical_rect.height);
    *baseline = pdu_to_dcm(pango_layout_iter_get_baseline(iter));

    pango_layout_iter_free(iter);
    g_object_unref(G_OBJECT(layout));

    return TRUE;
}
    

real
dia_font_scaled_ascent(const char* string, DiaFont* font, real height,
                      real zoom_factor)
{
    real top,bline,bottom;

    dia_font_vertical_extents(string,font,height,zoom_factor,
                              0,&top,&bline,&bottom);

    return bline-top;
}

real dia_font_scaled_descent(const char* string, DiaFont* font,
                            real height, real zoom_factor)
{
    real top,bline,bottom;

    dia_font_vertical_extents(string,font,height,zoom_factor,
                              0,&top,&bline,&bottom);

    return bottom-bline;
}

#define TWEAK_STRING_WIDTH

PangoLayout*
dia_font_scaled_build_layout(const char* string, DiaFont* font,
                            real height, real zoom_factor)
{
    DiaFont* altered_font;
    real scaling;
    real nozoom_width;
    real target_zoomed_width;
    PangoStretch stretch;
    PangoLayout* layout;
    real altered_scaling;
    real real_width;

    scaling = zoom_factor / global_size_one;
    if (fabs(1.0 - scaling) < 1E-7) {
        return dia_font_build_layout(string,font,height);
    }

    nozoom_width = dia_font_string_width(string,font,height);
    target_zoomed_width = nozoom_width * scaling;

        /* First try: no tweaks. */
#ifdef TWEAK_STRING_WIDTH
    real_width = dia_font_string_width(string,font,
					    height * scaling);
        if (real_width <= target_zoomed_width) {
#endif   
        return dia_font_build_layout(string,font,height*scaling);
#ifdef TWEAK_STRING_WIDTH
	}

    altered_font = dia_font_copy(font);

        /* Third try. Using the "reduce overall size" strategy. */
    for (altered_scaling = scaling;
         altered_scaling > (scaling / 2);
         altered_scaling *= (target_zoomed_width/real_width>0.98?0.98:
			     target_zoomed_width/real_width) ) {
      real_width = dia_font_string_width(string,font,
					 height * altered_scaling);

        if (real_width <= target_zoomed_width) {
            layout = dia_font_build_layout(string,altered_font,
                                          height*altered_scaling);
            dia_font_unref(altered_font);
            return layout;
        }
    }

    /* Everything has failed. Returning non-tweaked variant. */
    g_warning("Failed to appropriately tweak zoomed font.");
    dia_font_unref(altered_font);
    return dia_font_build_layout(string,font,height*scaling);    
#endif
}

/**
 * Compatibility with older files out of pre Pango Time. 
 * Make old files look as similar as possible
 *
 * FIXME: DIA_FONT_FAMILY_ANY in the list below does mean noone knows better
 */
static struct _legacy_font {
  gchar*       oldname;
  gchar*       newname;
  DiaFontStyle style;   /* the DIA_FONT_FAMILY() is used as falback only */
} legacy_fonts[] = {
  /* these _MUST_ be sorted alphabetical */
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
  { "Courier",             "Courier New", DIA_FONT_MONOSPACE },
  { "Courier-Bold",        "Courier New", DIA_FONT_MONOSPACE | DIA_FONT_BOLD },
  { "Courier-BoldOblique", "Courier New", DIA_FONT_MONOSPACE | DIA_FONT_OBLIQUE | DIA_FONT_BOLD },
  { "Courier-Oblique",     "Courier New", DIA_FONT_MONOSPACE | DIA_FONT_OBLIQUE },
  { "Dotum", "Dotum", DIA_FONT_FAMILY_ANY },
  { "GBZenKai-Medium", "GBZenKai-Medium", DIA_FONT_FAMILY_ANY }, 
  { "GothicBBB-Medium", "GothicBBB-Medium", DIA_FONT_FAMILY_ANY },
  { "Gulim", "Gulim", DIA_FONT_FAMILY_ANY }, 
  { "Headline", "Headline", DIA_FONT_FAMILY_ANY },
  { "Helvetica",             "Arial", DIA_FONT_SANS },
  { "Helvetica-Bold",        "Arial", DIA_FONT_SANS | DIA_FONT_BOLD },
  { "Helvetica-BoldOblique", "Arial", DIA_FONT_SANS | DIA_FONT_BOLD | DIA_FONT_OBLIQUE },
  { "Helvetica-Narrow",             "Arial Narrow", DIA_FONT_SANS | DIA_FONT_MEDIUM },
  { "Helvetica-Narrow-Bold",        "Arial Narrow", DIA_FONT_SANS | DIA_FONT_DEMIBOLD },
  { "Helvetica-Narrow-BoldOblique", "Arial Narrow", DIA_FONT_SANS | DIA_FONT_MEDIUM | DIA_FONT_OBLIQUE },
  { "Helvetica-Narrow-Oblique",     "Arial Narrow", DIA_FONT_SANS | DIA_FONT_MEDIUM | DIA_FONT_OBLIQUE },
  { "Helvetica-Oblique",     "Arial", DIA_FONT_SANS | DIA_FONT_OBLIQUE },
  { "MOESung-Medium", "MOESung-Medium", DIA_FONT_FAMILY_ANY },
  { "NewCenturySchoolbook-Bold",       "Century Schoolbook SWA", DIA_FONT_SERIF | DIA_FONT_BOLD },
  { "NewCenturySchoolbook-BoldItalic", "Century Schoolbook SWA", DIA_FONT_SERIF | DIA_FONT_BOLD | DIA_FONT_ITALIC },
  { "NewCenturySchoolbook-Italic",     "Century Schoolbook SWA", DIA_FONT_SERIF | DIA_FONT_ITALIC },
  { "NewCenturySchoolbook-Roman",      "Century Schoolbook SWA", DIA_FONT_SERIF },
  { "Palatino-Bold",       "Palatino", DIA_FONT_FAMILY_ANY | DIA_FONT_BOLD }, 
  { "Palatino-BoldItalic", "Palatino", DIA_FONT_FAMILY_ANY | DIA_FONT_BOLD | DIA_FONT_ITALIC }, 
  { "Palatino-Italic",     "Palatino", DIA_FONT_FAMILY_ANY | DIA_FONT_ITALIC }, 
  { "Palatino-Roman",      "Palatino", DIA_FONT_FAMILY_ANY },
  { "Ryumin-Light", "Ryumin", DIA_FONT_FAMILY_ANY | DIA_FONT_LIGHT },
  { "ShanHeiSun-Light", "ShanHeiSun", DIA_FONT_FAMILY_ANY | DIA_FONT_LIGHT },
  { "Song-Medium", "Song-Medium", DIA_FONT_FAMILY_ANY | DIA_FONT_MEDIUM },
  { "Symbol", "Symbol", DIA_FONT_SANS | DIA_FONT_MEDIUM },
  { "Times-Bold",       "Times New Roman", DIA_FONT_SERIF | DIA_FONT_BOLD },
  { "Times-BoldItalic", "Times New Roman", DIA_FONT_SERIF | DIA_FONT_ITALIC | DIA_FONT_BOLD },
  { "Times-Italic",     "Times New Roman", DIA_FONT_SERIF | DIA_FONT_ITALIC },
  { "Times-Roman",      "Times New Roman", DIA_FONT_SERIF }, 
  { "ZapfChancery-MediumItalic", "Zapf Calligraphic 801 SWA", DIA_FONT_SERIF | DIA_FONT_MEDIUM },
  { "ZapfDingbats", "Zapf Calligraphic 801 SWA", DIA_FONT_SERIF },
  { "ZenKai-Medium", "ZenKai", DIA_FONT_FAMILY_ANY | DIA_FONT_MEDIUM },
};


static int
fonts_compare (const void *key, const void *elem)
{
  char* a = (char*)key;
  char* b = ((struct _legacy_font*)elem)->oldname;
  return strcmp (a, b);
}

/**
 * Given a legacy name as stored until Dia-0.90 construct
 * a new DiaFont which is as similar as possible
 */
DiaFont*
dia_font_new_from_legacy_name(const char* name)
{
  /* do NOT translate anything here !!! */
  DiaFont* retval;
  struct _legacy_font* found;
  real height = 1.0;

  found = bsearch (name, legacy_fonts,
		   G_N_ELEMENTS(legacy_fonts), sizeof (struct _legacy_font),
		   fonts_compare);
  if (found) {
    retval = dia_font_new (found->newname, found->style, height);
    retval->legacy_name = found->oldname;
  } else {      
    /* We tried our best, let Pango complain */
    retval = dia_font_new (name, DIA_FONT_WEIGHT_NORMAL, height);
  }
  
  return retval;
}

