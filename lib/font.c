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

DiaFont*
dia_font_new_from_legacy_name(const char* name)
{
        /* do NOT translate anything here !!! */
    
    Style style = STYLE_NORMAL;
    DiaFont* retval;
    char* family;
    const char* dash = strchr(name,'-');
    
    if (!dash) return dia_font_new(name,style,1.0);

    family  = g_strndup(name,dash-name);
    if (0 == strncmp(dash+1,"BoldOblique",12)) {
        style = STYLE_BOLD_ITALIC; 
    } else if (0 == strncmp(dash+1,"Bold",5)) {
        style = STYLE_BOLD;
    } if (0 == strncmp(dash+1,"Oblique",8)) {
        style = STYLE_ITALIC;
    }
    retval = dia_font_new(family,style,1.0);
    g_free(family);
    return retval;
}

static G_CONST_RETURN char*
dia_font_style_to_legacy_name(Style style) {
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
dia_font_get_legacy_name(const DiaFont* font) {
    if (font->legacy_name) return font->legacy_name;
    
    ((DiaFont*)font)->legacy_name =
        g_strconcat(dia_font_get_family(font),
                    dia_font_style_to_legacy_name(dia_font_get_style(font)),
                    NULL);
    return font->legacy_name;
}

/* Conversion between our style and pango style/weight */
int dia_font_pango_style_weight_to_dia(int style, int weight) {
	 style + (3*(weight-200)/100)) + 1;
  return style + (3*(weight-200)/100) + 1;
}

void dia_font_dia_style_to_pango(int style, PangoStyle *pango_style, PangoWeight *pango_weight) {
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

static PangoStretch
get_prev_stretch(PangoStretch stretch) {
    switch(stretch) {
        case PANGO_STRETCH_ULTRA_CONDENSED:
            return PANGO_STRETCH_ULTRA_CONDENSED;
        case PANGO_STRETCH_EXTRA_CONDENSED:
            return PANGO_STRETCH_ULTRA_CONDENSED;
        case PANGO_STRETCH_CONDENSED:
            return PANGO_STRETCH_EXTRA_CONDENSED;
        case PANGO_STRETCH_SEMI_CONDENSED:
            return PANGO_STRETCH_CONDENSED;
        case PANGO_STRETCH_NORMAL:
            return PANGO_STRETCH_SEMI_CONDENSED;
        case PANGO_STRETCH_SEMI_EXPANDED:
            return PANGO_STRETCH_NORMAL;
        case PANGO_STRETCH_EXPANDED:
            return PANGO_STRETCH_SEMI_EXPANDED;
        case PANGO_STRETCH_EXTRA_EXPANDED:
            return PANGO_STRETCH_EXPANDED;
        case PANGO_STRETCH_ULTRA_EXPANDED: /* fall-through */
        default:
            return PANGO_STRETCH_EXTRA_EXPANDED;
    }
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
    
    scaling = zoom_factor / global_size_one;
    if (fabs(1.0 - scaling) < 1E-7) {
        return dia_font_build_layout(string,font,height);
    }

    nozoom_width = dia_font_string_width(string,font,height);
    target_zoomed_width = nozoom_width * scaling;

        /* First try: no tweaks. */
    if (dia_font_string_width(string,font,
                             height * scaling) <= target_zoomed_width) {
        return dia_font_build_layout(string,font,height*scaling);
    }

    altered_font = dia_font_copy(font);

        /* Second try. Using the "condense horizontally" strategy. */
    
    for (stretch = pango_font_description_get_stretch(altered_font->pfd);
         stretch != PANGO_STRETCH_ULTRA_CONDENSED;
         stretch = get_prev_stretch(stretch)) {
        
        pango_font_description_set_stretch(altered_font->pfd,stretch);
        if (dia_font_string_width(string,altered_font,
                                 height * scaling) <= target_zoomed_width) {
             layout = dia_font_build_layout(string,altered_font,height*scaling);
             dia_font_unref(altered_font);
             return layout;
        }
    }

    pango_font_description_set_stretch(
        altered_font->pfd,
        pango_font_description_get_stretch(font->pfd));
    
        /* Third try. Using the "reduce overall size" strategy. */
    for (altered_scaling = scaling;
         altered_scaling > (scaling / 2);
         altered_scaling *= 0.98 ) {
        
        if (dia_font_string_width(string,altered_font,
                                 height * altered_scaling) <=
            target_zoomed_width) {

            layout = dia_font_build_layout(string,altered_font,
                                          height*altered_scaling);
            dia_font_unref(altered_font);
            return layout;
        }
    }

        /* Everything have failed. Returning non-tweaked variant. */
    g_warning("Failed to appropriately tweak zoomed font.");
    dia_font_unref(altered_font);
    return dia_font_build_layout(string,font,height*scaling);    
}
