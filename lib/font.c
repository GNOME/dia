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
static real global_size_one = 20.0;

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

DiaFont*
dia_font_new(const char *family, DiaFontStyle style, real height)
{
  DiaFont* retval = dia_font_new_from_style(style, height);

  pango_font_description_set_family(retval->pfd,family);
  return retval;
}

static void
dia_pfd_set_family(PangoFontDescription* pfd, DiaFontFamily fam) {
  switch (fam) {
  case DIA_FONT_SANS :
    pango_font_description_set_family(pfd, "Sans");
    break;
  case DIA_FONT_SERIF :
    pango_font_description_set_family(pfd, "Serif");
    break;
  case DIA_FONT_MONOSPACE :
    pango_font_description_set_family(pfd, "Monospace");
    break;
  default :
          /* Pango does allow fonts without a name */
      ;
  }
}

static void
dia_pfd_set_weight(PangoFontDescription* pfd, DiaFontWeight fw) {
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
dia_pfd_set_slant(PangoFontDescription* pfd, DiaFontSlant fo) {
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

static void dia_pfd_set_size(PangoFontDescription* pfd, real height)
{ /* inline candidate... */
  pango_font_description_set_size(pfd, dcm_to_pdu(height) );
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
  dia_pfd_set_family(pfd,DIA_FONT_STYLE_GET_FAMILY(style));
  dia_pfd_set_weight(pfd,DIA_FONT_STYLE_GET_WEIGHT(style));
  dia_pfd_set_slant(pfd,DIA_FONT_STYLE_GET_SLANT(style));
  dia_pfd_set_size(pfd,height);
  
  retval = DIA_FONT(g_type_create_instance(dia_font_get_type()));
  retval->pfd = pfd;
  dia_font_ref(retval);
  return retval;
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

DiaFontStyle 
dia_font_get_style(const DiaFont* font)
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

  g_assert(PANGO_WEIGHT_ULTRALIGHT <= pango_weight && pango_weight <= PANGO_WEIGHT_HEAVY); 
  g_assert(PANGO_WEIGHT_ULTRALIGHT == 200);
  g_assert(PANGO_WEIGHT_NORMAL == 400);
  g_assert(PANGO_WEIGHT_BOLD == 700);

  style  = weight_map[(pango_weight - PANGO_WEIGHT_ULTRALIGHT) / 100] << 4;
  style |= (pango_style << 2);

  return style;
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

G_CONST_RETURN char*
dia_font_get_legacy_name(const DiaFont *font)
{
  /* FIXME: this is broken ! New fonts don't have this ! */    
  if (font->legacy_name)
    return font->legacy_name;

  return "Courier";
}

void dia_font_set_any_family(DiaFont* font, const char* family)
{
  g_assert(font != NULL);
  pango_font_description_set_family(font->pfd, family);
  if (font->legacy_name) {
      g_free(font->legacy_name);
      font->legacy_name = NULL;
  }
}

void dia_font_set_family(DiaFont* font, DiaFontFamily family)
{
  g_assert(font != NULL);
  dia_pfd_set_family(font->pfd,family);  
  if (font->legacy_name) {
      g_free(font->legacy_name);
      font->legacy_name = NULL;
  }
}

void dia_font_set_weight(DiaFont* font, DiaFontWeight weight)
{
  g_assert(font != NULL);    
  dia_pfd_set_weight(font->pfd,weight);
}

void dia_font_set_slant(DiaFont* font, DiaFontSlant slant)
{
  g_assert(font != NULL);    
  dia_pfd_set_slant(font->pfd,slant);
}


struct weight_name { DiaFontWeight fw; const char *name; };    
static const struct weight_name weight_names[] = {
    {DIA_FONT_ULTRALIGHT, "200"},
    {DIA_FONT_LIGHT,"300"},
    {DIA_FONT_WEIGHT_NORMAL,"normal"},
    {DIA_FONT_WEIGHT_NORMAL,"400"},
    {DIA_FONT_MEDIUM, "500"},
    {DIA_FONT_DEMIBOLD, "600"},
    {DIA_FONT_BOLD, "700"},
    {DIA_FONT_ULTRABOLD, "800"},
    {DIA_FONT_HEAVY, "900"},
    {0,NULL}};

G_CONST_RETURN char *dia_font_get_weight_string(const DiaFont* font)
{
    const struct weight_name* p;
    DiaFontWeight fw = DIA_FONT_STYLE_GET_WEIGHT(dia_font_get_style(font));
    
    for (p = weight_names; p->name != NULL; ++p) {
        if (p->fw == fw) return p->name;
    }
    return "normal";
}

void dia_font_set_weight_from_string(DiaFont* font, const char* weight) {
    DiaFontWeight fw = DIA_FONT_WEIGHT_NORMAL;
    const struct weight_name* p;

    for (p = weight_names; p->name != NULL; ++p) {
        if (0 == strncmp(weight,p->name,8)) {
            fw = p->fw;
            break;
        }
    }

    dia_font_set_weight(font,fw);
}


struct slant_name { DiaFontSlant fo; const char *name; };    
static const struct slant_name slant_names[] = {
    { DIA_FONT_NORMAL, "normal"},
    { DIA_FONT_OBLIQUE, "oblique"},
    { DIA_FONT_ITALIC, "italic"},
    { 0, NULL} };

G_CONST_RETURN char *
dia_font_get_slant_string(const DiaFont* font)
{
    const struct slant_name* p;
    DiaFontSlant fo =
    DIA_FONT_STYLE_GET_SLANT(dia_font_get_style(font));
    
    for (p = slant_names; p->name != NULL; ++p) {
        if (p->fo == fo) return p->name;
    }
    return "normal";
}

void dia_font_set_slant_from_string(DiaFont* font, const char* obli) {
    DiaFontSlant fo = DIA_FONT_NORMAL;
    const struct slant_name* p;

    DiaFontStyle old_style;
    DiaFontSlant old_fo;
    old_style = dia_font_get_style(font);
    old_fo = DIA_FONT_STYLE_GET_SLANT(old_style);
    
    for (p = slant_names; p->name != NULL; ++p) {
        if (0 == strncmp(obli,p->name,8)) {
            fo = p->fo;
            break;
        }
    }
    dia_font_set_slant(font,fo);
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
    /* Scale the result back for the zoom factor */
    result /= (zoom_factor/global_size_one);
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
  if (string[0] == '\0') {
    /* This hack won't work for fonts that don't cover ASCII */
    dia_font_vertical_extents("XjgM149",font,height,zoom_factor,
			      0,&top,&bline,&bottom);
  } else {
    dia_font_vertical_extents(string,font,height,zoom_factor,
			      0,&top,&bline,&bottom);
  }
  return (bline-top)/(zoom_factor/global_size_one);
}

real dia_font_scaled_descent(const char* string, DiaFont* font,
                            real height, real zoom_factor)
{
  real top,bline,bottom;

  if (string[0] == '\0') {
    /* This hack won't work for fonts that don't cover ASCII */
    dia_font_vertical_extents("XjgM149",font,height,zoom_factor,
			      0,&top,&bline,&bottom);
  } else {
    dia_font_vertical_extents(string,font,height,zoom_factor,
                              0,&top,&bline,&bottom);
  }
  return (bottom-bline)/(zoom_factor/global_size_one);
}

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
    real_width = dia_font_string_width(string,font, height * scaling);
    if (real_width <= target_zoomed_width) {
        return dia_font_build_layout(string,font,height*scaling);
    }

    altered_font = dia_font_copy(font);

        /* Last try. Using the "reduce overall size" strategy. */
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
    retval->legacy_name = NULL;
  }
  
  return retval;
}

